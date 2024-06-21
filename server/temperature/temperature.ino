#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <TFT_eSPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

// Wi-Fi credentials
const char* ssid = "IOT";
const char* password = "password";
const char* serverIP = "http://192.168.1.100:5000/data";  // Optional server endpoint

TFT_eSPI tft = TFT_eSPI();
WebServer server(80);
Adafruit_BME680 bme; // I2C

void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);

  connectToWiFi();
  if (!bme.begin()) {
    tftPrint("Could not find a valid BME680 sensor, check wiring!");
    while (1);
  }

  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms
  
  setupServerEndpoints();
  server.begin();
  tftPrint("Server started");
  showMenu();
}

void loop() {
  server.handleClient();
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  tftPrint("Connecting to Wi-Fi...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    tftPrint(".");
  }

  tftPrint("Connected! IP Address: " + WiFi.localIP().toString());
}

void tftPrint(String message) {
  tft.fillScreen(TFT_BLACK);
  tft.drawString(message, 10, 10, 2);
  Serial.println(message);
}

void readSensorData(float& temperature, float& humidity, float& gas, float& altitude) {
  unsigned long endTime = bme.beginReading();
  delay(500); // Wait for reading to complete

  if (bme.endReading() > 0) {
    temperature = bme.temperature;
    humidity = bme.humidity;
    gas = bme.gas_resistance / 1000.0; // Convert to kOhms
    altitude = bme.readAltitude(1013.25); // Assuming sea level pressure
  } else {
    tftPrint("BME680 reading failed");
  }
}

void handleRoot() {
  String html = "<html><body><h1>ESP32 Sensor Server</h1><ul>";
  html += "<li><a href=\"/temp\">HTML Sensor Data</a></li>";
  html += "<li><a href=\"/sensor_data\">Json Sensor Data</a></li>";
  html += "</ul></body></html>";
  server.send(200, "text/html", html);
}

void handleGetSensorData() {
  float temperature, humidity, gas, altitude;
  readSensorData(temperature, humidity, gas, altitude);

  String json = "{\"temperature\": " + String(temperature) +
                ", \"humidity\": " + String(humidity) +
                ", \"gas\": " + String(gas) +
                ", \"altitude\": " + String(altitude) + "}";

  server.send(200, "application/json", json);
}

void handleGetTemp() {
  float temperature, humidity, gas, altitude;
  readSensorData(temperature, humidity, gas, altitude);

  String html = "<html><body><h1>Sensor Data</h1>";
  html += "<p>Temperature: " + String(temperature) + " &deg;C</p>";
  html += "<p>Humidity: " + String(humidity) + " %</p>";
  html += "<p>Gas: " + String(gas) + " kOhms</p>";
  html += "<p>Altitude: " + String(altitude) + " m</p>";
  html += "<a href=\"/\">Back to Menu</a>";
  html += "</body></html>";
  server.send(200, "text/html", html);

  tftPrint("Temp: " + String(temperature) + " C, Hum: " + String(humidity) + " %, Gas: " + String(gas) + " kOhms, Alt: " + String(altitude) + " m");
}

void setupServerEndpoints() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/sensor_data", HTTP_GET, handleGetSensorData);
  server.on("/temp", HTTP_GET, handleGetTemp);
}

void showMenu() {
  tft.fillScreen(TFT_BLACK);
  tft.drawString("Menu:", 10, 10, 2);
  tft.drawString("1. Temperature & Humidity", 10, 30, 2);
  tft.drawString("Visit /temp endpoint", 10, 50, 2);
}
