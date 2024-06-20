// REST Motion
// Features:
// 1. T-Display
//  github.com/Xinyuan-LilyGO/TTGO-T-Display
// https://dl.espressif.com/dl/package_esp32_index.json
//  esp32 -> ESP32 Dev Module

//  04.01.2023
//  Posts at regular intervals.
//  Future Revisions:
//    Decouple checking sensor & posting interval.
//    Dual docs


#include <WiFi.h>
#include <FS.h>
#include <HTTPClient.h>
#include "time.h"


#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();
//https://randomnerdtutorials.com/esp32-date-time-ntp-client-server-arduino/
//https://www.survivingwithandroid.com/esp32-rest-api-esp32-api-server/
//https://www.mischianti.org/2020/05/24/rest-server-on-esp8266-and-esp32-get-and-json-formatter-part-2/
#include <Arduino.h>
// #include <WiFi.h>
// #include <FS.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
// #include <Adafruit_Sensor.h>
// #include <Adafruit_BME680.h>
// #include "time.h"
const char *ntpServer = "pool.ntp.org";
//const long  gmtOffset_sec = 3600 * 4;
const long gmtOffset_sec = 3600 * -8;
const int daylightOffset_sec = 3600;

#define MAX_ELEMENTS 418  // 428 -> Crash
#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define SEALEVELPRESSURE_HPA (1013.25)
#define BUTTON1PIN 35
#define BUTTON2PIN 0

// Adafruit_BME680 bme; // I2C
const char *SSID = "IOT";
const char *PWD = "saturday";
const int red_pin = 5;
const int green_pin = 18;
const int blue_pin = 19;
// Setting PWM frequency, channels and bit resolution
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
float temperature;
float humidity;
float pressure;
void setup_task();
void getMenu();
void getData();
void resetData();
void getTemperature();
void getPressure();
void getHumidity();
void getSettings();
void setup_routing();
void read_sensor_data(void *parameter);
void read_motion(char *device, int PIN);
String get_time();


void IRAM_ATTR toggleButton1() {
  Serial.println("Button 1 Pressed!");
  // tft.fillScreen(TFT_BLACK);
  // tft.setCursor(0, 30);
  // tft.drawLine(0, 35, 250, 35, TFT_BLUE);
  // tft.setFreeFont(&Orbitron_Light_24);
  // tft.setTextColor(TFT_GREEN, TFT_BLACK);
  // tft.print("Button 1");
  // tft.setCursor(0, 60);
  // tft.print("Pressed!");
}

void IRAM_ATTR toggleButton2() {
  Serial.println("Button 2 Pressed!");
  // tft.fillScreen(TFT_BLACK);
  // tft.setCursor(0, 30);
  // tft.drawLine(0, 35, 250, 35, TFT_BLUE);
  // tft.setFreeFont(&Orbitron_Light_24);
  // tft.setTextColor(TFT_GREEN, TFT_BLACK);
  // tft.print("Button 2");
  // tft.setCursor(0, 60);
  // tft.print("Pressed!");
}

int ENFORCER = 27;  // PIN=27  Seco-Larm E-931-S35RRQ Enforcer Indoor/Outdoor Wall Mounted
// void IRAM_ATTR interrupt_motion_detected() {
//   Serial.println("Interrupt - Motion Detected");
//   // read_motion( "/enforcer", ENFORCER );
//   read_motion( "/motion", ENFORCER );
// }


unsigned long start_time = 0;
unsigned long last_time = 0;
bool motion;
int motion_count = 0;
int motion_count_post = 0;
int motion_threshold = 1;
unsigned long interval = 5000;  //  Allowed time to pass with no motion.
int BUTTON_1 = 35;
int BUTTON_2 = 0;

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLUE);
  pinMode(BUTTON1PIN, INPUT);
  pinMode(BUTTON2PIN, INPUT);
  attachInterrupt(BUTTON1PIN, toggleButton1, FALLING);
  attachInterrupt(BUTTON2PIN, toggleButton2, FALLING);
  // if (ENFORCER) {
  //   pinMode(ENFORCER, INPUT_PULLUP);  // Enforcer works beautifully with a INPUT_PULLUP
  //                                     // attachInterrupt(ENFORCER, interrupt_motion_detected, FALLING );
  // }

  Serial.begin(115200);
  // while (!Serial);
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(SSID, PWD);
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(5000);
  //   }

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


  // Check internet access
  if (checkInternetAccess()) {
    Serial.println("Internet access verified");
  } else {
    Serial.println("No internet access");
    while (true); // Stop here if no internet access
  }



  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Wait for time to be set
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 10000)) {  // Increase timeout to 10 seconds
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "Time set: %A, %B %d %Y %H:%M:%S");


  xTaskCreate(read_sensor_data, "Read sensor data", 2000, NULL, 2, NULL);
  xTaskCreate(post_sensor_data, "Post sensor data", 2000, NULL, 1, NULL);
  setup_routing();
}

void setup_routing() {
  server.on("/", getMenu);
  server.on("/poll", get_poll);
  server.on("/motion", get_motion);
  server.on("/reset", resetData);
  server.on(F("/set"), HTTP_GET, getSettings);
  server.begin();
}


bool checkInternetAccess() {
  HTTPClient http;
  http.begin("http://google.com");  // URL for checking internet access
  int httpCode = http.GET();
  http.end();
  return httpCode > 0;
}



void read_sensors() {
  read_motion("/motion", ENFORCER);
}

//  Read Boolean Motion
void read_motion(char *device, int PIN) {
  // Serial.print("motion_count = ");
  if (motion_count > 0) {
    int lapse = millis() - last_time;
    if (lapse > interval) {
      add_json_object_2("motion", motion_count, "i");

      motion_count = 0;
      // t.fill_screen( TFT_BLACK );
      Serial.println("read_motion: motion_count = 0");
    }
  }

  motion = digitalRead(PIN);
  if (motion) {
    if (motion_count == 0) {
      start_time = millis();
      // t.fill_screen( TFT_BLACK );
    }
    last_time = millis();
    //    t.lcd_display( 1, 0, t.fontsize, 100, String( last_time ) );
    motion_count++;
    motion_count_post++;
    String mt = "Motion=" + String(motion_count);
    // t.lcd_display( 1, 30, t.fontsize, 0, mt );
    Serial.println(mt);
    // if ( motion_count >= motion_threshold )
    // {
    //     // publish_message( device, String( motion_count ) );
    //     Serial.println("publish_message");
    // }
  }
}


void getMenu() {
  Serial.println("get Menu");
  String html = "<html><body>";
  html += "<h1>Motion Sensor - Enforcer 1</h1><br>";
  html += "Interval = ";
  html += SAMPLE_INTERVAL;
  html += "<br>";
  html += "<a href='http://10.0.0.21/set?interval=10000'>http://10.0.0.21/set?interval=10000</a><br>";
  html += "<a href='http://10.0.0.21/motion'>Motion Data</a><br>";
  html += "<a href='http://10.0.0.21/poll'>Polled Data</a><br>";
  html += "<a href='http://10.0.0.21/reset'>Reset</a><br>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}


void getSettings() {
  Serial.println("getSettings()");
  // Serial.println( server.args() );
  String message = "";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    String name1 = server.argName(i);
    //      String value1 = server.arg ( i );
    if (name1.equals("interval")) {
      SAMPLE_INTERVAL = server.arg(i).toInt();
      Serial.println(SAMPLE_INTERVAL);
    }
  }
  Serial.println(message);
  //    if (server.arg("interval")== "true")
  //    {
  //        response+= ",\"signalStrengh\": \""+String(WiFi.RSSI())+"\"";
  //    }
  DynamicJsonDocument doc1(2048);
  doc1["set"] = "Great Success !";
  // Serialize JSON document
  String json;
  serializeJson(doc1, json);
  server.send(200, "application/json", json);
}



void add_json_object(char *tag, float value, char *unit) {
  int dic_size = jsonDocument_1.size();
  if (dic_size >= MAX_ELEMENTS) {
    Serial.println("Remove 1");
    jsonDocument_1.remove(0);
  }
  JsonObject objt = jsonDocument_1.createNestedObject();
  objt["time"] = get_time();
  objt["type"] = tag;
  objt["value"] = value;
  objt["unit"] = unit;
}

void add_json_object_2(char *tag, float value, char *unit) {
  int dic_size = jsonDocument_2.size();
  if (dic_size >= MAX_ELEMENTS) {
    Serial.println("Remove 1");
    jsonDocument_2.remove(0);
  }
  JsonObject objt = jsonDocument_2.createNestedObject();
  objt["time"] = get_time();
  objt["type"] = tag;
  objt["value"] = value;
  objt["unit"] = unit;
}

void show_time() {
  String t = get_time();
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_BLUE);
  tft.drawString(t, 10, 10, 6);
  Serial.print("time=");
  Serial.println(t);
}

void read_sensor_data(void *parameter) {
  for (;;) {
    String theString = get_time();
    // Serial.print("theString=");
    // Serial.println(theString);
    int theInt = theString.substring(theString.length() - 2, theString.length()).toInt();
    // Serial.print("theInt=");
    // Serial.println(theInt);
    read_sensors();
    vTaskDelay(SAMPLE_INTERVAL / portTICK_PERIOD_MS);
  }
}

void post_sensor_data(void *parameter) {
  for (;;) {
    add_json_object("post", motion_count_post, "i");
    motion_count_post = 0;
    vTaskDelay(POST_INTERVAL);
  }
}

void resetData() {
  jsonDocument_1.clear();
  jsonDocument_2.clear();
  Serial.println("Data reset");
  DynamicJsonDocument doc1(2048);
  doc1["reset"] = "Great Success !";
  String json;
  serializeJson(doc1, json);
  server.send(200, "application/json", json);
}

void get_poll() {
  Serial.println("Get Polled Data");
  serializeJson(jsonDocument_1, buffer);
  Serial.println(buffer);
  // int dic_size_motion = jsonDocument_1.size();
  // int dic_size = doc.size();
  // Serial.println(dic_size_motion);
  // Serial.println(dic_size);

  // for (int i=0; i < dic_size; i++)
  // {
  //   const char* sensor = doc[i]["type"];
  //   Serial.println( sensor );
  // }
  // Serial.println( buffer );
  server.send(200, "application/json", buffer);
}


void get_motion() {
  Serial.println("Get Motion Times");
  StaticJsonDocument<64> filter;
  filter["type"] = "motion";

  int size = jsonDocument_2.size();
  Serial.println(size);
  if (!size) {
    Serial.println("NULL");
    server.send(200, "application/json", "{}");
  }


  if (size) {
    Serial.println("Not NULL");
    serializeJson(jsonDocument_2, buffer);
    server.send(200, "application/json", buffer);
    jsonDocument_2.clear();  //  Reset data for iot client device
  }
}


void loop() {
  server.handleClient();
}


String get_time() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return "Error";
  }
  //  Serial.println(&timeinfo, "print - %A, %B %d %Y %H:%M:%S");
  //https://arduino.stackexchange.com/questions/52676/how-do-you-convert-a-formatted-print-statement-into-a-string-variable
  char timeStringBuff[50];
  // strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
  strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M:%S", &timeinfo);
  String asString(timeStringBuff);
  return asString;
}