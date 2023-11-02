// AWS Certificates, etc.
#include "secrets.h"

// API for sending MQTT message to AWS IoT Core
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include "WiFi.h"
#include "esp_camera.h"

#include "HTTPClient.h"

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

int i = 0;

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}

void connectAWS() {
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  client.setServer(AWS_IOT_ENDPOINT, 8883);
  Serial.println("Began client");
  client.setKeepAlive(30);
  Serial.println("Setting Buffer Size to 40032: " + String(client.setBufferSize(40032)));

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
  //  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
  //Serial.println(client.state());
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
}

void publishMessage() {
  camera_fb_t *fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  char *input = (char *)fb->buf;

  //AWS Lambda Actions need base64 encoded data, jsonified
  char output[base64_enc_len(3)];
  String image;
  for (int i = 0; i < fb->len; i++) {
    base64_encode(output, (input++), 3);
    if (i % 3 == 0) image += String(output);
  }
  esp_camera_fb_return(fb);

  char timestamp[12];
  itoa(millis(), timestamp, 10);
  char jsonStart[30] = "{\"file\":\"";
  strcat(jsonStart, timestamp);
  strcat(jsonStart, "\",\"image\":\"");
  Serial.println(jsonStart);
  char jsonEnd[] = "\"}";
  Serial.println(jsonEnd);

  if (!client.connected()) {
    Serial.println("Connecting to AWS IOT");

    while (!client.connect(THINGNAME)) {
      Serial.print(".");
      delay(100);
    }

    if (!client.connected()) {
      Serial.println("AWS IoT Timeout!");
      return;
    }
  }



  uint32_t fbLen = image.length();
  String str = "";
  String imageFile = "";
  String chunk = "";
  for (uint32_t n = 0; n < fbLen; n = n + 40000) {

    if (n + 40000 < fbLen) {
      str = image.substring(n, n + 40000);
    } else if (fbLen % 40000 > 0) {
      uint32_t remain = fbLen % 40000;
      str = image.substring(n, n + remain);
    }
    imageFile = String(jsonStart) + str + String(jsonEnd);
    //Serial.println(String(imageFile.length()));
    //Serial.println(imageFile.substring(0,40));
    //Serial.println(imageFile.substring(imageFile.length()-40,imageFile.length()));
    //Serial.println(client.publish(AWS_IOT_PUBLISH_TOPIC, (uint8_t*)imageFile.c_str(),imageFile.length(), false));
    size_t chunkLen = imageFile.length();
    boolean res = client.beginPublish(AWS_IOT_PUBLISH_TOPIC, chunkLen, false);
    Serial.println("publish began " + String(res));

    for (size_t z = 0; z < chunkLen; z = z + 2048) {
      if (z + 2048 < chunkLen) {
        chunk = imageFile.substring(z, z + 2048);
        client.write((uint8_t *)chunk.c_str(), 2048);
      } else if (chunkLen % 2048 > 0) {
        size_t remainder = chunkLen % 2048;
        chunk = imageFile.substring(z, z + remainder);
        client.write((uint8_t *)chunk.c_str(), remainder);
      }
      Serial.println(String(z));
    }

    Serial.println("publish written " + String(res));

    res = client.endPublish();
    Serial.println("publish ended " + String(res));
    Serial.println(String(n));
  }
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  setupCamera();
  connectWifi();
  connectAWS();
  Serial.println("out of connection phase");
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("looping now");
  publishMessage();
  delay(30000);
  client.loop();
}
