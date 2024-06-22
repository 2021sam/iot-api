/**
 * ESP32 BME680 Sensor Server with TFT Display
 * 
 * This program sets up an ESP32 to read data from a BME680 sensor and 
 * display the temperature, humidity, and gas resistance on a TFT screen. 
 * It also sets up a web server to serve the sensor data in both HTML and JSON formats.
 * 
 * Button 1: Refresh sensor data
 * Button 2: Update display
 * 
 * Wi-Fi credentials:
 *   SSID: IOT
 *   Password: password
 * 
 * Server Endpoints:
 *   - /html : Display sensor data in HTML format
 *   - /json : Display sensor data in JSON format
 * 
 * Hardware Connections:
 *   - Button 1: GPIO 0
 *   - Button 2: GPIO 35
 *   - BME680: I2C
 *   - TFT: SPI
 * 
 * Author: Sam Portillo
 * Date: 2024-06-21
 */

#include <WiFi.h>
#include <WebServer.h>
#include <TFT_eSPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

// Wi-Fi credentials
const char* ssid = "IOT";
const char* password = "password";

TFT_eSPI tft = TFT_eSPI();
WebServer server(80);
Adafruit_BME680 bme; // I2C

// Button pins
const int button1Pin = 0; // GPIO0
const int button2Pin = 35; // GPIO35

volatile bool refreshData = false;
volatile bool refreshDisplay = false;

void IRAM_ATTR handleButton1() {
  Serial.println("Button 1 pressed");
  refreshData = true;
}

void IRAM_ATTR handleButton2() {
  Serial.println("Button 2 pressed");
  refreshDisplay = true;
}

void tftPrint(String message) {
  tft.fillScreen(TFT_BLACK);

  int y = 10; // Initial y-coordinate
  String line;
  while (message.length() > 0) {
    int idx = message.indexOf(' '); // Find the space in the message
    if (idx == -1) {
      line = message; // No space found, take the entire message
      message = ""; // Clear the message
    } else {
      line = message.substring(0, idx + 1); // Get up to the space (including space)
      message = message.substring(idx + 1); // Remove printed part from the message
    }

    tft.drawString(line, 10, y); // Print the line
    y += 30; // Move to the next line (adjust based on your font size)
  }

  Serial.println(message); // Print message to Serial for debugging
  delay(1000);
}

void setup() {
  Serial.begin(115200);

  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);
  attachInterrupt(button1Pin, handleButton1, FALLING);
  attachInterrupt(button2Pin, handleButton2, FALLING);

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2); // Set text size to 2 for increased font size

  connectToWiFi();
  if (!bme.begin()) {
    tftPrint("No BME680 sensor found!");
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

  displaySensorData(); // Initial display of sensor data
}

void loop() {
  server.handleClient();

  if (refreshData) {
    refreshSensorData(); // Refresh sensor data when button 1 is pressed
    refreshData = false;
  }

  if (refreshDisplay) {
    displaySensorData(); // Update display when button 2 is pressed
    refreshDisplay = false;
  }
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  tftPrint("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    tftPrint(".");
  }

  tftPrint("Connected!");
  tftPrint("IP address:");
  tftPrint(WiFi.localIP().toString());
}

void displaySensorData() {
  float temperature, humidity, gas;
  readSensorData(temperature, humidity, gas);

  tft.setTextSize(2);

  // Display temperature
  String tempStr = String(temperature, 1) + "C";
  tftPrint("Temperature: " + tempStr);

  // Display humidity
  String humStr = String(humidity, 1) + "%";
  tftPrint("Humidity: " + humStr);

  // Display gas resistance
  String gasStr = String(gas, 2) + " kOhms";
  tftPrint("Gas: " + gasStr);
}

void readSensorData(float& temperature, float& humidity, float& gas) {
  unsigned long endTime = bme.beginReading();
  delay(500); // Wait for reading to complete

  if (bme.endReading() > 0) {
    temperature = bme.temperature;
    humidity = bme.humidity;
    gas = bme.gas_resistance / 1000.0; // Convert to kOhms
  } else {
    tftPrint("BME680 read failed");
  }
}

void refreshSensorData() {
  float temperature, humidity, gas;
  readSensorData(temperature, humidity, gas);
}

void setupServerEndpoints() {
  server.on("/", HTTP_GET, [](){
    String html = "<html><body>";
    html += "<h1>BME680 Sensor Data</h1>";
    html += "<h2>Endpoints:</h2>";
    html += "<ul>";
    html += "<li><a href=\"/html\">HTML BME680 data</a></li>";
    html += "<li><a href=\"/json\">JSON BME680 data</a></li>";
    html += "</ul>";
    html += "<p><strong>Button Instructions:</strong></p>";
    html += "<ul>";
    html += "<li>Press Button 1 to refresh sensor data.</li>";
    html += "<li>Press Button 2 to update the display.</li>";
    html += "</ul>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });

  server.on("/html", HTTP_GET, [](){
      float temperatureC = bme.temperature;
      float temperatureF = temperatureC * 1.8 + 32.0;
      float humidity = bme.humidity;
      float gas = bme.gas_resistance / 1000.0;

      String html = "<html><body>";
      html += "<h1>BME680 Data (HTML)</h1>";
      html += "<p>Temperature (C): " + String(temperatureC, 1) + "C</p>";
      html += "<p>Temperature (F): " + String(temperatureF, 1) + "F</p>";
      html += "<p>Humidity: " + String(humidity, 1) + "%</p>";
      html += "<p>Gas: " + String(gas, 2) + " kOhms</p>";
      html += "</body></html>";
      server.send(200, "text/html", html);
  });

  server.on("/json", HTTP_GET, [](){
    String json = "{";
    json += "\"temperature\": " + String(bme.temperature, 1) + ",";
    json += "\"humidity\": " + String(bme.humidity, 1) + ",";
    json += "\"gas\": " + String(bme.gas_resistance / 1000.0, 2);
    json += "}";
    server.send(200, "application/json", json);
  });
}
