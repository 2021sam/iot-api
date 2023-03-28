// github.com/Xinyuan-LilyGO/T-Display-S3
// In Arduino Preferences, on the Settings tab, enter the
// https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
// 170 x 320

#include "Arduino.h"
#include "TFT_eSPI.h"
#include "pin_config.h"
TFT_eSPI tft = TFT_eSPI();

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
const char* ssid = "Hunter";
const char* password = "saturday";
//Your Domain name with URL path or IP address with path
const char* serverName = "http://10.0.0.21/motion";
// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
unsigned long timerDelay = 2 * 1000;

String sensorReadings;
int BUTTON_1 = 0;
int BUTTON_2 = 14;
void IRAM_ATTR toggleButton1();
void IRAM_ATTR toggleButton2();
int POLL_INTERVAL = 100;

void setup() {
  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);
  // attachInterrupt( digitalPinToInterrupt( BUTTON_1 ), toggleButton1, FALLING);
  // attachInterrupt( digitalPinToInterrupt( BUTTON_2 ), toggleButton2, FALLING);
  // pinMode(PIN_POWER_ON, OUTPUT);
  // digitalWrite(PIN_POWER_ON, HIGH);

  Serial.begin(115200);
  tft.begin();
  tft.setRotation(3);
  tft.setSwapBytes(true);

  ledcSetup(0, 2000, 8);
  ledcAttachPin(PIN_LCD_BL, 0);
  ledcWrite(0, 255);

  xTaskCreate(read_button_1, "Read Button 1", 2000, NULL, 1, NULL);
  xTaskCreate(read_button_2, "Read Button 2", 2000, NULL, 1, NULL);
}

void checkConnections() {
  if (WiFi.status() != WL_CONNECTED) {
    setup_WiFi();
  }
}

void setup_WiFi() {
  tft.fillScreen(TFT_RED);
  tft.setTextColor(TFT_BLACK, TFT_RED);
  tft.drawString("Joining Network", 50, 10, 4);
  tft.drawString(ssid, 80, 80, 4);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);  //  07/08/2021  WiFi crashed, may have been stuck in a loop.
    delay(5000);                 //  2000 is not enough but 3000 is.
    Serial.println("2");
  }

  if (WiFi.status() == WL_CONNECTED) {
    IPAddress broadCast = WiFi.localIP();
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println(broadCast);
    tft.fillScreen(TFT_GREEN);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("WiFi", 60, 0, 4);
    tft.drawString(IpAddress2String(broadCast), 0, 50, 4);
  }
}

String IpAddress2String(const IPAddress& ipAddress) {
  return String(ipAddress[0]) + String(".") + String(ipAddress[1]) + String(".") + String(ipAddress[2]) + String(".") + String(ipAddress[3]);
}

void read_button_1(void* parameter) {
  for (;;) {
    if (digitalRead(BUTTON_1) == 0) {
      tft.fillScreen(TFT_BLUE);
      tft.setTextColor(TFT_BLUE, TFT_GREEN);
      tft.drawString("Waiting for Motion", 40, 20, 4);
      Serial.println("Button 1 Pressed!");
    }
    vTaskDelay(POLL_INTERVAL / portTICK_PERIOD_MS);
  }
}

void read_button_2(void* parameter) {
  for (;;) {
    if (digitalRead(BUTTON_2) == 0) {
      tft.fillScreen(TFT_BLUE);
      tft.setTextColor(TFT_BLUE, TFT_GREEN);
      tft.drawString("Waiting for Motion", 50, 20, 4);
      Serial.println("Button 2 Pressed!");
    }
    vTaskDelay(POLL_INTERVAL / portTICK_PERIOD_MS);
  }
}

void loop() {
  int lapse = millis() - lastTime;
  Serial.print("lapse = ");
  Serial.println(lapse);
  if (lapse > timerDelay) {
    lastTime = millis();
    checkConnections();

    sensorReadings = httpGETRequest(serverName);
    Serial.println(sensorReadings);
    JSONVar myObject = JSON.parse(sensorReadings);
    Serial.print("json=");
    Serial.println(myObject);

    for (int i = 0; i < myObject.length(); i++) {
      int value_int = myObject[i]["value"];
      String value_s = String(value_int);
      Serial.print("value=");
      Serial.println(value_s);
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.drawString("MOTION  ALERT", 55, 30, 4);
      tft.drawString(value_s, 100, 90, 8);
      Serial.println(myObject[i]["time"]);
      Serial.println(myObject[i]["type"]);
      Serial.println(myObject[i]["value"]);
      Serial.println(myObject[i]["unit"]);
      delay(5000);
    }
  }
  delay(1000);
}

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
  http.begin(client, serverName);
  delay(2000);
  int httpResponseCode = http.GET();
  Serial.print("httpResponseCode=");
  Serial.println(httpResponseCode);
  String payload = "{}";
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
  return payload;
}
