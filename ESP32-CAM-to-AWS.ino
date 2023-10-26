
// AWS Certificates, etc.
#include "secrets.h"

// API for sending MQTT message to AWS IoT Core
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include "WiFi.h"
#include "esp_camera.h"

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/pub"

// Camera Model
// CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

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
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
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
  char message[512] = "my message ";
  Serial.println("setting message: " + String(message));
  int fbLen = strlen(message);
  Serial.println("message length: " + String(fbLen));
  boolean res = client.beginPublish(AWS_IOT_PUBLISH_TOPIC, fbLen, false);
  Serial.println("publish began " + String(res));

  res = client.write((byte*)message, fbLen);
  Serial.println("publish written " + String(res));

  res = client.endPublish();
  Serial.println("publish ended " + String(res));

  //   // String str = "";
  //   // for (size_t n=0;n<fbLen;n=n+2048) {
  //   //   if (n+2048<fbLen) {
  //   //     str = imageFile.substring(n, n+2048);
  //   //     client.write((uint8_t*)str.c_str(), 2048);
  //   //   }
  //   //   else if (fbLen%2048>0) {
  //   //     size_t remainder = fbLen%2048;
  //   //     str = imageFile.substring(n, n+remainder);
  //   //     client.write((uint8_t*)str.c_str(), remainder);
  //   //   }
  //   // }  

  // char message[512] = "my message";
  // client.publish(AWS_IOT_PUBLISH_TOPIC, message);
  // Serial.println("published");
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  setupCamera();
  connectWifi();
  connectAWS();
  publishMessage();
}

void loop() {
  // put your main code here, to run repeatedly:

}
