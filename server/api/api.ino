#include <WiFi.h>
#include <FS.h>
#include <HTTPClient.h>
#include "time.h"
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Arduino.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "Adafruit_BME680.h"

TFT_eSPI tft = TFT_eSPI();
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600 * -8;
const int daylightOffset_sec = 3600;

#define MAX_ELEMENTS 418
#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define SEALEVELPRESSURE_HPA (1013.25)
#define BUTTON1PIN 35
#define BUTTON2PIN 0

const char *SSID = "IOT";
const char *PWD = "saturday";
const int red_pin = 5;
const int green_pin = 18;
const int blue_pin = 19;
const int frequency = 5000;
const int redChannel = 0;
const int greenChannel = 1;
const int blueChannel = 2;
const int resolution = 8;
int SAMPLE_INTERVAL = 1 * 100;
int POST_INTERVAL = 60 * 1000;

WebServer server(80);
StaticJsonDocument<10000> jsonDocument_1;
StaticJsonDocument<10000> jsonDocument_2;
char buffer[34000];
bool motion;
int motion_count = 0;
int motion_count_post = 0;
int motion_threshold = 1;
unsigned long start_time = 0;
unsigned long last_time = 0;
unsigned long interval = 5000;

Adafruit_BME680 bme;

void setup_task();
void getMenu();
void getData();
void resetData();
void getSettings();
void setup_routing();
void read_sensor_data(void *parameter);
void read_motion(char *device, int PIN);
void setup_bme();
void loop_bme();
String get_time();
void IRAM_ATTR toggleButton1();
void IRAM_ATTR toggleButton2();
void add_json_object(char *tag, float value, char *unit);
void add_json_object_2(char *tag, float value, char *unit);
void read_sensors();
void get_poll();
void get_motion();
void get_sensor_values();

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLUE);
  pinMode(BUTTON1PIN, INPUT);
  pinMode(BUTTON2PIN, INPUT);
  attachInterrupt(BUTTON1PIN, toggleButton1, FALLING);
  attachInterrupt(BUTTON2PIN, toggleButton2, FALLING);

  Serial.begin(115200);

  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(SSID, PWD);

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 30000) {
      Serial.print(".");
      delay(500);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect to Wi-Fi");
    while (true); // Stop here if unable to connect
  }
  Serial.print("Connected! IP Address: ");
  Serial.println(WiFi.localIP());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 10000)) {  // Increase timeout to 10 seconds
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "Time set: %A, %B %d %Y %H:%M:%S");

  xTaskCreate(read_sensor_data, "Read sensor data", 2000, NULL, 2, NULL);
  xTaskCreate(post_sensor_data, "Post sensor data", 2000, NULL, 1, NULL);
  setup_routing();
  setup_bme();
}

void setup_routing() {
  server.on("/", getMenu);
  server.on("/poll", get_poll);
  server.on("/motion", get_motion);
  server.on("/temp", get_sensor_values);
  server.on("/reset", resetData);
  server.on(F("/set"), HTTP_GET, getSettings);
  server.begin();
}

void get_sensor_values() {
  unsigned long endTime = bme.beginReading();
  if (endTime == 0) {
    Serial.println(F("Failed to begin reading BME680 :("));
    server.send(500, "application/json", "{\"error\": \"Failed to start sensor reading\"}");
    return;
  }
  delay(50);
  if (!bme.endReading()) {
    Serial.println(F("Failed to complete reading BME680 :("));
    server.send(500, "application/json", "{\"error\": \"Failed to complete sensor reading\"}");
    return;
  }

  StaticJsonDocument<256> doc;
  doc["temperature"] = bme.temperature;
  doc["pressure"] = bme.pressure / 100.0;
  doc["humidity"] = bme.humidity;
  doc["gas_resistance"] = bme.gas_resistance / 1000.0;
  doc["approx_altitude"] = bme.readAltitude(SEALEVELPRESSURE_HPA);

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void getMenu() {
  Serial.println("get Menu");
  String html = "<html><body>";
  html += "<h1>Motion Sensor - Enforcer 1</h1><br>";
  html += "Interval = ";
  html += interval;
  html += "<br><a href='/poll'>Start polling</a>";
  html += "<br><a href='/motion'>Get motion data</a>";
  html += "<br><a href='/temp'>Get temperature data</a>";
  html += "<br><a href='/reset'>Reset Data</a>";
  html += "<br><a href='/set'>Set Settings</a>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void resetData() {
  Serial.println("Reset Data");
  motion_count = 0;
  motion_count_post = 0;
  server.send(200, "application/json", "Data Reset");
}

void getSettings() {
  String html = "<html><body>";
  html += "<form action=\"/set\" method=\"POST\">";
  html += "<label for=\"interval\">Set Interval:</label><br>";
  html += "<input type=\"text\" id=\"interval\" name=\"interval\" value=\"";
  html += interval;
  html += "\"><br><br>";
  html += "<input type=\"submit\" value=\"Submit\">";
  html += "</form>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void get_poll() {
  String html = "<html><body>";
  html += "Polling started!";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void get_motion() {
  StaticJsonDocument<256> doc;
  doc["motion"] = motion;
  doc["motion_count"] = motion_count;
  doc["motion_count_post"] = motion_count_post;

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void loop() {
  loop_bme();
  server.handleClient();
}

void read_sensor_data(void *parameter) {
  while (true) {
    if (millis() - last_time > interval) {
      read_sensors();
      last_time = millis();
    }
    delay(10);
  }
}


void post_sensor_data(void *parameter) {
  for (;;) {
    add_json_object("post", motion_count_post, "i");
    motion_count_post = 0;
    vTaskDelay(POST_INTERVAL);
  }
}


void setup_bme() {
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1);
  }

  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms
}

void loop_bme() {
  // Placeholder for any BME680 specific loop actions
}

void read_sensors() {
  unsigned long endTime = bme.beginReading();
  if (endTime == 0) {
    Serial.println(F("Failed to begin reading BME680 :("));
    return;
  }
  delay(50);
  if (!bme.endReading()) {
    Serial.println(F("Failed to complete reading BME680 :("));
    return;
  }

  add_json_object("Temperature", bme.temperature, "C");
  add_json_object("Pressure", bme.pressure / 100.0, "hPa");
  add_json_object("Humidity", bme.humidity, "%");
  add_json_object("Gas Resistance", bme.gas_resistance / 1000.0, "kOhms");
  add_json_object("Approx Altitude", bme.readAltitude(SEALEVELPRESSURE_HPA), "m");
}

void add_json_object(char *tag, float value, char *unit) {
  StaticJsonDocument<256> doc;
  doc["tag"] = tag;
  doc["value"] = value;
  doc["unit"] = unit;

  String json;
  serializeJson(doc, json);
  Serial.println(json);
}

void add_json_object_2(char *tag, float value, char *unit) {
  StaticJsonDocument<256> doc;
  doc["tag"] = tag;
  doc["value"] = value;
  doc["unit"] = unit;

  String json;
  serializeJson(doc, json);
  Serial.println(json);
}

void IRAM_ATTR toggleButton1() {
  motion = !motion;
  motion_count++;
  motion_count_post++;
}

void IRAM_ATTR toggleButton2() {
  // Placeholder for Button2 functionality
}
