// AWS Certificates, etc.
#include "secrets.h"

// API for sending MQTT message to AWS IoT Core
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include "WiFi.h"
#include "esp_camera.h"
#include "camera_init.h"

#include "HTTPClient.h"
#include <NTPClient.h>

//Library for saving to SD card
#include "SD_MMC.h"

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

int count = 0;
char completeUrl[120];

void messageHandler(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  for (count = 0; count < 10; count++) {
    takeAndSave();  
  }
  count = 0;

  //  StaticJsonDocument<200> doc;
  //  deserializeJson(doc, payload);
  //  const char* message = doc["message"];
}

void connectWifi() {
  WiFi.mode(WIFI_STA);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Wifi connected, configuring time");
  // Synchronize the RTC with the NTP server
  timeClient.begin();
}

void initAWS() {
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  client.setServer(AWS_IOT_ENDPOINT, 8883);
  client.setKeepAlive(40);
  // Create a message handler
  client.setCallback(messageHandler);
  connectAWS();
}

void connectAWS() {
  Serial.println("Connecting to AWS IOT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }
  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
  Serial.println(client.state());
}

void setupCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if (psramFound()) {
    Serial.println("psramfound");
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 30;
    //config.jpeg_quality = 48;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    //config.jpeg_quality = 48;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  //Serial.println("Starting SD Card");
  if(!SD_MMC.begin()){
    Serial.println("SD Card Mount Failed");
    return;
  }
  
  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE){
    Serial.println("No SD Card attached");
    return;
  }

}

void takeAndSave() {
  camera_fb_t *fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  // Get current time for filename
  time_t rawTime = timeClient.getEpochTime();

  // Create a tmElements_t struct to extract time components
  char filename[18];
  // Format the time as YYYYMMDDHHMMSS
  snprintf(filename, sizeof(filename), "/%10d%02d.jpg", rawTime, count);

  Serial.println(filename);

  File file = SD_MMC.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
  } else {
    file.write(fb->buf, fb->len);
    Serial.printf("Saved file to: %s\n", filename);
  }
  file.close();
  esp_camera_fb_return(fb);
}

void takeAndUpload() {
  camera_fb_t *fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  // Get current time for filename
  time_t rawTime = timeClient.getEpochTime();

  // Create a tmElements_t struct to extract time components
  char filename[18];
  // Format the time as YYYYMMDDHHMMSS
  snprintf(filename, sizeof(filename), "/%10d%02d.jpg", rawTime, count);

  Serial.println(filename);

  // Create an HTTPClient object
  HTTPClient http;

  // Use snprintf to format the time directly into formattedTime
  snprintf(completeUrl, sizeof(completeUrl), "%s%s", serverUrl, filename);
  Serial.println(completeUrl);

  // Specify the server and endpoint
  http.begin(completeUrl);

  // Set the HTTP method to PUT
  http.addHeader("Content-Type", "image/jpeg");
  int httpResponseCode = http.sendRequest("PUT", (uint8_t *)fb->buf, fb->len);
  http.end();

  // Check for a successful response
  if (httpResponseCode == 200) {
    Serial.println("Image uploaded successfully!");
  } else {
    Serial.print("HTTP error code: ");
    Serial.println(httpResponseCode);
    File file = SD_MMC.open(filename, FILE_WRITE);
    if (!file) {
      Serial.println("Failed to open file for writing");
    } else {
      file.write(fb->buf, fb->len);
      Serial.printf("Saved file to: %s\n", filename);
    }
    file.close();
  }

  // End the HTTP connection

  esp_camera_fb_return(fb);
}

void listFiles() {
  File root = SD_MMC.open("/");
  File file = root.openNextFile();

  while (file) {
    Serial.print("File: ");
    Serial.println(file.name());
    file = root.openNextFile();
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  setupCamera();
  connectWifi();
  initAWS();
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("looping now");
  timeClient.update();
  client.loop();
  takeAndUpload();
  listFiles();
  delay(30000);

  if (!client.connected()) {
    connectAWS();
  } else {
    client.loop();
  }
}
