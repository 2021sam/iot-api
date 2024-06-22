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

// Button pins
const int button1Pin = 0; // GPIO0
const int button2Pin = 35; // GPIO35

volatile bool refreshData = false;
volatile bool refreshDisplay = false;

void IRAM_ATTR handleButton1() {
  refreshData = true;
}

void IRAM_ATTR handleButton2() {
  refreshDisplay = true;
}

void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2); // Set text size to 2 for increased font size

  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);
  attachInterrupt(button1Pin, handleButton1, FALLING);
  attachInterrupt(button2Pin, handleButton2, FALLING);
  
  connectToWiFi();
  if (!bme.begin()) {
    Serial.println("No BME680 sensor found!");
    while (1);
  }

  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms
  
  setupServerEndpoints();
  server.begin();
  Serial.println("Server started");

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
  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("Connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void displaySensorData() {
  float temperature, humidity, gas;
  readSensorData(temperature, humidity, gas);

  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);

  // Display temperature
  String tempStr = String(temperature, 1) + "C";
  tft.drawString("Temp: " + tempStr, 10, 10, 2);

  // Display humidity
  String humStr = String(humidity, 1) + "%";
  tft.drawString("Humidity: " + humStr, 10, 40, 2);

  // Display gas resistance
  String gasStr = String(gas, 2) + " kOhms";
  tft.drawString("Gas: " + gasStr, 10, 70, 2);
}

void readSensorData(float& temperature, float& humidity, float& gas) {
  unsigned long endTime = bme.beginReading();
  delay(500); // Wait for reading to complete

  if (bme.endReading() > 0) {
    temperature = bme.temperature;
    humidity = bme.humidity;
    gas = bme.gas_resistance / 1000.0; // Convert to kOhms
  } else {
    Serial.println("BME680 read failed");
  }
}

void refreshSensorData() {
  float temperature, humidity, gas;
  readSensorData(temperature, humidity, gas);
}

void setupServerEndpoints() {
  server.on("/data", HTTP_GET, [](){
    String html = "<html><body>";
    html += "<h1>BME680 Data</h1>";
    html += "<p>Temperature: " + String(bme.temperature, 1) + "C</p>";
    html += "<p>Humidity: " + String(bme.humidity, 1) + "%</p>";
    html += "<p>Gas: " + String(bme.gas_resistance / 1000.0, 2) + " kOhms</p>";
    html += "<p>Options:</p>";
    html += "<ul>";
    html += "<li><a href=\"/html\">HTML BME680 data</a></li>";
    html += "<li><a href=\"/json\">JSON BME680 data</a></li>";
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

  // server.on("/html", HTTP_GET, [](){
  //   String html = "<html><body>";
  //   html += "<h1>BME680 Data (HTML)</h1>";
  //   html += "<p>Temperature: " + String(bme.temperature, 1) + "C</p>";
  //   html += "<p>Humidity: " + String(bme.humidity, 1) + "%</p>";
  //   html += "<p>Gas: " + String(bme.gas_resistance / 1000.0, 2) + " kOhms</p>";
  //   html += "</body></html>";
  //   server.send(200, "text/html", html);
  // });

  server.on("/json", HTTP_GET, [](){
    String json = "{";
    json += "\"temperature\": " + String(bme.temperature, 1) + ",";
    json += "\"humidity\": " + String(bme.humidity, 1) + ",";
    json += "\"gas\": " + String(bme.gas_resistance / 1000.0, 2);
    json += "}";
    server.send(200, "application/json", json);
  });
}




// #include <WiFi.h>
// #include <WebServer.h>
// #include <HTTPClient.h>
// #include <TFT_eSPI.h>
// #include <Adafruit_Sensor.h>
// #include <Adafruit_BME680.h>

// // Wi-Fi credentials
// const char* ssid = "IOT";
// const char* password = "password";
// const char* serverIP = "http://192.168.1.100:5000/data";  // Optional server endpoint

// TFT_eSPI tft = TFT_eSPI();
// WebServer server(80);
// Adafruit_BME680 bme; // I2C

// // Button pins
// const int button1Pin = 0; // GPIO0
// const int button2Pin = 35; // GPIO35

// volatile bool refreshData = false;
// volatile bool refreshDisplay = false;

// void IRAM_ATTR handleButton1() {
//   refreshData = true;
// }

// void IRAM_ATTR handleButton2() {
//   refreshDisplay = true;
// }

// void setup() {
//   Serial.begin(115200);
//   tft.begin();
//   tft.setRotation(1);
//   tft.fillScreen(TFT_BLACK);
//   tft.setTextColor(TFT_WHITE);
//   tft.setTextSize(2); // Set text size to 2 for menu

//   pinMode(button1Pin, INPUT_PULLUP);
//   pinMode(button2Pin, INPUT_PULLUP);
//   attachInterrupt(button1Pin, handleButton1, FALLING);
//   attachInterrupt(button2Pin, handleButton2, FALLING);
  
//   connectToWiFi();
//   if (!bme.begin()) {
//     tftPrint("No BME680 sensor found!");
//     while (1);
//   }

//   bme.setTemperatureOversampling(BME680_OS_8X);
//   bme.setHumidityOversampling(BME680_OS_2X);
//   bme.setPressureOversampling(BME680_OS_4X);
//   bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
//   bme.setGasHeater(320, 150); // 320*C for 150 ms
  
//   setupServerEndpoints();
//   server.begin();
//   tftPrint("Server started"); // Display "Server started" on TFT screen
//   displayMenu();
// }

// void loop() {
//   server.handleClient();

//   if (refreshData) {
//     refreshSensorData(); // Refresh sensor data when button 1 is pressed
//     refreshData = false;
//   }

//   if (refreshDisplay) {
//     displaySensorData(); // Update display when button 2 is pressed
//     refreshDisplay = false;
//   }
// }

// void connectToWiFi() {
//   WiFi.begin(ssid, password);
//   tftPrint("Connecting to Wi-Fi...");

//   while (WiFi.status() != WL_CONNECTED) {
//     delay(1000);
//     tftPrint(".");
//   }

//   tftPrint("Connected!");
//   tftPrint("IP: " + WiFi.localIP().toString());
// }

// void tftPrint(String message) {
//   tft.fillScreen(TFT_BLACK);

//   int y = 10; // Initial y-coordinate
//   String line;
//   while (message.length() > 0) {
//     int idx = message.indexOf(' '); // Find the space in the message
//     if (idx == -1) {
//       line = message; // No space found, take the entire message
//       message = ""; // Clear the message
//     } else {
//       line = message.substring(0, idx + 1); // Get up to the space (including space)
//       message = message.substring(idx + 1); // Remove printed part from the message
//     }

//     tft.drawString(line, 10, y, 2); // Print the line
//     y += 30; // Move to the next line (adjust based on your font size)
//   }

//   Serial.println(message); // Print message to Serial for debugging
// }

// void displayMenu() {
//   tft.fillScreen(TFT_BLACK);
//   tft.setTextSize(2);

//   tft.drawString("Menu:", 10, 10, 2);
//   tft.drawString("1. HTML BME680 data", 10, 40, 2);
//   tft.drawString("2. JSON BME680 data", 10, 70, 2);
// }

// void readSensorData(float& temperature, float& humidity, float& gas) {
//   unsigned long endTime = bme.beginReading();
//   delay(500); // Wait for reading to complete

//   if (bme.endReading() > 0) {
//     temperature = bme.temperature;
//     humidity = bme.humidity;
//     gas = bme.gas_resistance / 1000.0; // Convert to kOhms
//   } else {
//     tftPrint("BME680 read failed");
//   }
// }

// void refreshSensorData() {
//   float temperature, humidity, gas;
//   readSensorData(temperature, humidity, gas);
// }

// void displaySensorData() {
//   float temperature, humidity, gas;
//   readSensorData(temperature, humidity, gas);

//   tft.fillScreen(TFT_BLACK);
//   tft.setTextSize(2);

//   // Display temperature
//   String tempStr = String(temperature, 1) + "C";
//   tft.drawString("Temp: " + tempStr, 10, 10, 2);

//   // Display humidity
//   String humStr = String(humidity, 1) + "%";
//   tft.drawString("Humidity: " + humStr, 10, 40, 2);

//   // Display gas resistance
//   String gasStr = String(gas, 2) + " kOhms";
//   tft.drawString("Gas: " + gasStr, 10, 70, 2);
// }

// void setupServerEndpoints() {
//   server.on("/html", HTTP_GET, [](){
//     displaySensorData();
//     String html = "<html><body>";
//     html += "<h1>BME680 Data</h1>";
//     html += "<p>Temperature: " + String(bme.temperature, 1) + "C</p>";
//     html += "<p>Humidity: " + String(bme.humidity, 1) + "%</p>";
//     html += "<p>Gas: " + String(bme.gas_resistance / 1000.0, 2) + " kOhms</p>";
//     html += "</body></html>";
//     server.send(200, "text/html", html);
//   });

//   server.on("/json", HTTP_GET, [](){
//     displaySensorData();
//     String json = "{";
//     json += "\"temperature\": " + String(bme.temperature, 1) + ",";
//     json += "\"humidity\": " + String(bme.humidity, 1) + ",";
//     json += "\"gas\": " + String(bme.gas_resistance / 1000.0, 2);
//     json += "}";
//     server.send(200, "application/json", json);
//   });
// }




// #include <WiFi.h>
// #include <WebServer.h>
// #include <HTTPClient.h>
// #include <TFT_eSPI.h>
// #include <Adafruit_Sensor.h>
// #include <Adafruit_BME680.h>

// // Wi-Fi credentials
// const char* ssid = "IOT";
// const char* password = "password";
// const char* serverIP = "http://192.168.1.100:5000/data";  // Optional server endpoint

// TFT_eSPI tft = TFT_eSPI();
// WebServer server(80);
// Adafruit_BME680 bme; // I2C

// // Button pins
// const int button1Pin = 0; // GPIO0
// const int button2Pin = 35; // GPIO35

// volatile bool refreshData = false;
// volatile bool refreshDisplay = false;

// void IRAM_ATTR handleButton1() {
//   refreshData = true;
// }

// void IRAM_ATTR handleButton2() {
//   refreshDisplay = true;
// }

// void setup() {
//   Serial.begin(115200);
//   tft.begin();
//   tft.setRotation(1);
//   tft.fillScreen(TFT_BLACK);
//   tft.setTextColor(TFT_WHITE);
//   tft.setTextSize(3); // Set text size to 3 for increased font size

//   pinMode(button1Pin, INPUT_PULLUP);
//   pinMode(button2Pin, INPUT_PULLUP);
//   attachInterrupt(button1Pin, handleButton1, FALLING);
//   attachInterrupt(button2Pin, handleButton2, FALLING);
  
//   connectToWiFi();
//   if (!bme.begin()) {
//     tftPrint("No BME680 sensor found!");
//     while (1);
//   }

//   bme.setTemperatureOversampling(BME680_OS_8X);
//   bme.setHumidityOversampling(BME680_OS_2X);
//   bme.setPressureOversampling(BME680_OS_4X);
//   bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
//   bme.setGasHeater(320, 150); // 320*C for 150 ms
  
//   setupServerEndpoints();
//   server.begin();
//   tftPrint("Server started"); // Display "Server started" on TFT screen
// }

// void loop() {
//   server.handleClient();

//   if (refreshData) {
//     refreshSensorData(); // Refresh sensor data when button 1 is pressed
//     refreshData = false;
//   }

//   if (refreshDisplay) {
//     displaySensorData(); // Update display when button 2 is pressed
//     refreshDisplay = false;
//   }
// }

// void connectToWiFi() {
//   WiFi.begin(ssid, password);
//   tftPrint("Connecting to Wi-Fi...");

//   while (WiFi.status() != WL_CONNECTED) {
//     delay(1000);
//     tftPrint(".");
//   }

//   tftPrint("Connected!");
//   tftPrint("IP: " + WiFi.localIP().toString());
// }

// void tftPrint(String message) {
//   tft.fillScreen(TFT_BLACK);

//   int y = 10; // Initial y-coordinate
//   String line;
//   while (message.length() > 0) {
//     int idx = message.indexOf(' '); // Find the space in the message
//     if (idx == -1) {
//       line = message; // No space found, take the entire message
//       message = ""; // Clear the message
//     } else {
//       line = message.substring(0, idx + 1); // Get up to the space (including space)
//       message = message.substring(idx + 1); // Remove printed part from the message
//     }

//     tft.drawString(line, 10, y, 2); // Print the line
//     y += 30; // Move to the next line (adjust based on your font size)
//   }

//   Serial.println(message); // Print message to Serial for debugging
// }

// void readSensorData(float& temperature, float& humidity, float& gas) {
//   unsigned long endTime = bme.beginReading();
//   delay(500); // Wait for reading to complete

//   if (bme.endReading() > 0) {
//     temperature = bme.temperature;
//     humidity = bme.humidity;
//     gas = bme.gas_resistance / 1000.0; // Convert to kOhms
//   } else {
//     tftPrint("BME680 read failed");
//   }
// }

// void refreshSensorData() {
//   float temperature, humidity, gas;
//   readSensorData(temperature, humidity, gas);
// }

// void displaySensorData() {
//   float temperature, humidity, gas;
//   readSensorData(temperature, humidity, gas);

//   tft.fillScreen(TFT_BLACK);
//   tft.setTextSize(3);

//   // Display temperature
//   String tempStr = String(temperature, 1) + "C";
//   tft.drawString("Temp: " + tempStr, 10, 10, 2);

//   // Display humidity
//   String humStr = String(humidity, 1) + "%";
//   tft.drawString("Humidity: " + humStr, 10, 50, 2);

//   // Display gas resistance
//   String gasStr = String(gas, 2) + " kOhms";
//   tft.drawString("Gas: " + gasStr, 10, 90, 2);
// }

// void setupServerEndpoints() {
//   server.on("/html", HTTP_GET, [](){
//     displaySensorData();
//     String html = "<html><body>";
//     html += "<h1>BME680 Data</h1>";
//     html += "<p>Temperature: " + String(bme.temperature, 1) + "C</p>";
//     html += "<p>Humidity: " + String(bme.humidity, 1) + "%</p>";
//     html += "<p>Gas: " + String(bme.gas_resistance / 1000.0, 2) + " kOhms</p>";
//     html += "</body></html>";
//     server.send(200, "text/html", html);
//   });

//   server.on("/json", HTTP_GET, [](){
//     displaySensorData();
//     String json = "{";
//     json += "\"temperature\": " + String(bme.temperature, 1) + ",";
//     json += "\"humidity\": " + String(bme.humidity, 1) + ",";
//     json += "\"gas\": " + String(bme.gas_resistance / 1000.0, 2);
//     json += "}";
//     server.send(200, "application/json", json);
//   });
// }




// #include <WiFi.h>
// #include <WebServer.h>
// #include <HTTPClient.h>
// #include <TFT_eSPI.h>
// #include <Adafruit_Sensor.h>
// #include <Adafruit_BME680.h>

// // Wi-Fi credentials
// const char* ssid = "IOT";
// const char* password = "password";
// const char* serverIP = "http://192.168.1.100:5000/data";  // Optional server endpoint

// TFT_eSPI tft = TFT_eSPI();
// WebServer server(80);
// Adafruit_BME680 bme; // I2C

// // Button pins
// const int button1Pin = 0; // GPIO0
// const int button2Pin = 35; // GPIO35

// volatile bool refreshData = false;
// volatile bool refreshDisplay = false;

// void IRAM_ATTR handleButton1() {
//   refreshData = true;
// }

// void IRAM_ATTR handleButton2() {
//   refreshDisplay = true;
// }

// void setup() {
//   Serial.begin(115200);
//   tft.begin();
//   tft.setRotation(1);
//   tft.fillScreen(TFT_BLACK);
//   tft.setTextColor(TFT_WHITE);
//   tft.setTextSize(3); // Set text size to 3 for increased font size

//   pinMode(button1Pin, INPUT_PULLUP);
//   pinMode(button2Pin, INPUT_PULLUP);
//   attachInterrupt(button1Pin, handleButton1, FALLING);
//   attachInterrupt(button2Pin, handleButton2, FALLING);
  
//   connectToWiFi();
//   if (!bme.begin()) {
//     tftPrint("No BME680 sensor found!");
//     while (1);
//   }

//   bme.setTemperatureOversampling(BME680_OS_8X);
//   bme.setHumidityOversampling(BME680_OS_2X);
//   bme.setPressureOversampling(BME680_OS_4X);
//   bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
//   bme.setGasHeater(320, 150); // 320*C for 150 ms
  
//   setupServerEndpoints();
//   server.begin();
//   tftPrint("Server started"); // Display "Server started" on TFT screen
// }

// void loop() {
//   server.handleClient();

//   if (refreshData) {
//     refreshSensorData();
//     refreshData = false;
//   }

//   if (refreshDisplay) {
//     displaySensorData();
//     refreshDisplay = false;
//   }
// }

// void connectToWiFi() {
//   WiFi.begin(ssid, password);
//   tftPrint("Connecting to Wi-Fi...");

//   while (WiFi.status() != WL_CONNECTED) {
//     delay(1000);
//     tftPrint(".");
//   }

//   tftPrint("Connected!");
//   tftPrint("IP: " + WiFi.localIP().toString());
// }

// void tftPrint(String message) {
//   tft.fillScreen(TFT_BLACK);

//   int y = 10; // Initial y-coordinate
//   String line;
//   while (message.length() > 0) {
//     int idx = message.indexOf(' '); // Find the space in the message
//     if (idx == -1) {
//       line = message; // No space found, take the entire message
//       message = ""; // Clear the message
//     } else {
//       line = message.substring(0, idx + 1); // Get up to the space (including space)
//       message = message.substring(idx + 1); // Remove printed part from the message
//     }

//     tft.drawString(line, 10, y, 2); // Print the line
//     y += 30; // Move to the next line (adjust based on your font size)
//   }

//   Serial.println(message); // Print message to Serial for debugging
// }

// void readSensorData(float& temperature, float& humidity, float& gas) {
//   unsigned long endTime = bme.beginReading();
//   delay(500); // Wait for reading to complete

//   if (bme.endReading() > 0) {
//     temperature = bme.temperature;
//     humidity = bme.humidity;
//     gas = bme.gas_resistance / 1000.0; // Convert to kOhms
//   } else {
//     tftPrint("BME680 read failed");
//   }
// }

// void displaySensorData() {
//   float temperature, humidity, gas;
//   readSensorData(temperature, humidity, gas);

//   tft.fillScreen(TFT_BLACK);
//   tft.setTextSize(3);

//   // Display temperature
//   String tempStr = String(temperature, 1) + "C";
//   tft.drawString("Temp: " + tempStr, 10, 10, 2);

//   // Display humidity
//   String humStr = String(humidity, 1) + "%";
//   tft.drawString("Humidity: " + humStr, 10, 50, 2);

//   // Display gas resistance
//   String gasStr = String(gas, 2) + " kOhms";
//   tft.drawString("Gas: " + gasStr, 10, 90, 2);
// }

// void setupServerEndpoints() {
//   server.on("/html", HTTP_GET, [](){
//     displaySensorData();
//     String html = "<html><body>";
//     html += "<h1>BME680 Data</h1>";
//     html += "<p>Temperature: " + String(bme.temperature, 1) + "C</p>";
//     html += "<p>Humidity: " + String(bme.humidity, 1) + "%</p>";
//     html += "<p>Gas: " + String(bme.gas_resistance / 1000.0, 2) + " kOhms</p>";
//     html += "</body></html>";
//     server.send(200, "text/html", html);
//   });

//   server.on("/json", HTTP_GET, [](){
//     displaySensorData();
//     String json = "{";
//     json += "\"temperature\": " + String(bme.temperature, 1) + ",";
//     json += "\"humidity\": " + String(bme.humidity, 1) + ",";
//     json += "\"gas\": " + String(bme.gas_resistance / 1000.0, 2);
//     json += "}";
//     server.send(200, "application/json", json);
//   });
// }





// #include <WiFi.h>
// #include <WebServer.h>
// #include <HTTPClient.h>
// #include <TFT_eSPI.h>
// #include <Adafruit_Sensor.h>
// #include <Adafruit_BME680.h>

// // Wi-Fi credentials
// const char* ssid = "IOT";
// const char* password = "password";
// const char* serverIP = "http://192.168.1.100:5000/data";  // Optional server endpoint

// TFT_eSPI tft = TFT_eSPI();
// WebServer server(80);
// Adafruit_BME680 bme; // I2C

// // Button pins
// const int button1Pin = 0; // GPIO0
// const int button2Pin = 35; // GPIO2

// volatile bool refreshData = false;

// void IRAM_ATTR handleButton1() {
//   refreshData = true;
// }

// void setup() {
//   Serial.begin(115200);
//   tft.begin();
//   tft.setRotation(1);
//   tft.fillScreen(TFT_BLACK);
//   tft.setTextColor(TFT_WHITE);
//   tft.setTextSize(3); // Set text size to 3 for increased font size

//   pinMode(button1Pin, INPUT_PULLUP);
//   pinMode(button2Pin, INPUT_PULLUP);
//   attachInterrupt(button1Pin, handleButton1, FALLING);
  
//   connectToWiFi();
//   if (!bme.begin()) {
//     tftPrint("No BME680 sensor found!");
//     while (1);
//   }

//   bme.setTemperatureOversampling(BME680_OS_8X);
//   bme.setHumidityOversampling(BME680_OS_2X);
//   bme.setPressureOversampling(BME680_OS_4X);
//   bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
//   bme.setGasHeater(320, 150); // 320*C for 150 ms
  
//   setupServerEndpoints();
//   server.begin();
//   tftPrint("Server started");
// }

// void loop() {
//   server.handleClient();

//   if (refreshData) {
//     displaySensorData();
//     refreshData = false;
//   }
// }

// void connectToWiFi() {
//   WiFi.begin(ssid, password);
//   tftPrint("Connecting to Wi-Fi...");

//   while (WiFi.status() != WL_CONNECTED) {
//     delay(1000);
//     tftPrint(".");
//   }

 
//   tftPrint("Connected!");
//   tftPrint("IP: " + WiFi.localIP().toString());
// }

// void tftPrint(String message) {
//   tft.fillScreen(TFT_BLACK);

//   int y = 10; // Initial y-coordinate
//   while (message.length() > 0) {
//     String line = message.substring(0, 11); // Get the first 11 characters
//     message = message.substring(11); // Remove the first 11 characters

//     tft.drawString(line, 10, y, 2); // Print the line
//     y += 30; // Move to the next line (adjust based on your font size)
//   }

//   Serial.println(message);
// }

// void readSensorData(float& temperature, float& humidity, float& gas) {
//   unsigned long endTime = bme.beginReading();
//   delay(500); // Wait for reading to complete

//   if (bme.endReading() > 0) {
//     temperature = bme.temperature;
//     humidity = bme.humidity;
//     gas = bme.gas_resistance / 1000.0; // Convert to kOhms
//   } else {
//     tftPrint("BME680 read failed");
//   }
// }

// void handleRoot() {
//   String html = "<html><body><h1>ESP32 Sensor Server</h1><ul>";
//   html += "<li><a href=\"/html\">HTML BME680 data</a></li>";
//   html += "<li><a href=\"/json\">JSON BME680 data</a></li>";
//   html += "</ul></body></html>";
//   server.send(200, "text/html", html);
// }

// void handleGetSensorData() {
//   float temperature, humidity, gas;
//   readSensorData(temperature, humidity, gas);

//   String json = "{\"temperature\": " + String(temperature) +
//                 ", \"humidity\": " + String(humidity) +
//                 ", \"gas\": " + String(gas) + "}";

//   server.send(200, "application/json", json);
// }

// void handleGetHtml() {
//   float temperature, humidity, gas;
//   readSensorData(temperature, humidity, gas);

//   String html = "<html><body><h1>Sensor Data</h1>";
//   html += "<p>Temperature: " + String(temperature) + " &deg;C</p>";
//   html += "<p>Humidity: " + String(humidity) + " %</p>";
//   html += "<p>Gas: " + String(gas) + " kOhms</p>";
//   html += "<a href=\"/\">Back to Menu</a>";
//   html += "</body></html>";
//   server.send(200, "text/html", html);

//   tftPrint("Temp: " + String(temperature) + "C Hum: " + String(humidity) + "% Gas: " + String(gas) + "kOhms");
// }

// void setupServerEndpoints() {
//   server.on("/", HTTP_GET, handleRoot);
//   server.on("/json", HTTP_GET, handleGetSensorData);
//   server.on("/html", HTTP_GET, handleGetHtml);
// }

// void displaySensorData() {
//   float temperature, humidity, gas;
//   readSensorData(temperature, humidity, gas);

//   tftPrint("Temp: " + String(temperature) + "C Hum: " + String(humidity) + "% Gas: " + String(gas) + "kOhms");
// }





// #include <WiFi.h>
// #include <WebServer.h>
// #include <HTTPClient.h>
// #include <TFT_eSPI.h>
// #include <Adafruit_Sensor.h>
// #include <Adafruit_BME680.h>

// // Wi-Fi credentials
// const char* ssid = "IOT";
// const char* password = "password";
// const char* serverIP = "http://192.168.1.100:5000/data";  // Optional server endpoint

// TFT_eSPI tft = TFT_eSPI();
// WebServer server(80);
// Adafruit_BME680 bme; // I2C

// // Button pins
// const int button1Pin = 0; // GPIO0
// const int button2Pin = 2; // GPIO2

// volatile bool refreshData = false;

// void IRAM_ATTR handleButton1() {
//   refreshData = true;
// }

// void setup() {
//   Serial.begin(115200);
//   tft.begin();
//   tft.setRotation(1);
//   tft.fillScreen(TFT_BLACK);
//   tft.setTextColor(TFT_WHITE);

//   pinMode(button1Pin, INPUT_PULLUP);
//   pinMode(button2Pin, INPUT_PULLUP);
//   attachInterrupt(button1Pin, handleButton1, FALLING);
  
//   connectToWiFi();
//   if (!bme.begin()) {
//     tftPrint("Could not find a valid BME680 sensor, check wiring!");
//     while (1);
//   }

//   bme.setTemperatureOversampling(BME680_OS_8X);
//   bme.setHumidityOversampling(BME680_OS_2X);
//   bme.setPressureOversampling(BME680_OS_4X);
//   bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
//   bme.setGasHeater(320, 150); // 320*C for 150 ms
  
//   setupServerEndpoints();
//   server.begin();
//   tftPrint("Server started");
// }

// void loop() {
//   server.handleClient();

//   if (refreshData) {
//     displaySensorData();
//     refreshData = false;
//   }
// }

// void connectToWiFi() {
//   WiFi.begin(ssid, password);
//   tftPrint("Connecting to Wi-Fi...");

//   while (WiFi.status() != WL_CONNECTED) {
//     delay(1000);
//     tftPrint(".");
//   }

//   tftPrint("Connected! IP Address: " + WiFi.localIP().toString());
// }

// void tftPrint(String message) {
//   tft.fillScreen(TFT_BLACK);
//   tft.setTextSize(3); // Set text size to 3 for increased font size
//   tft.drawString(message, 10, 10); // No need for the fourth parameter
//   Serial.println(message);
// }

// void readSensorData(float& temperature, float& humidity, float& gas) {
//   unsigned long endTime = bme.beginReading();
//   delay(500); // Wait for reading to complete

//   if (bme.endReading() > 0) {
//     temperature = bme.temperature;
//     humidity = bme.humidity;
//     gas = bme.gas_resistance / 1000.0; // Convert to kOhms
//   } else {
//     tftPrint("BME680 reading failed");
//   }
// }

// void handleRoot() {
//   String html = "<html><body><h1>ESP32 Sensor Server</h1><ul>";
//   html += "<li><a href=\"/html\">HTML BME680 data</a></li>";
//   html += "<li><a href=\"/json\">JSON BME680 data</a></li>";
//   html += "</ul></body></html>";
//   server.send(200, "text/html", html);
// }

// void handleGetSensorData() {
//   float temperature, humidity, gas;
//   readSensorData(temperature, humidity, gas);

//   String json = "{\"temperature\": " + String(temperature) +
//                 ", \"humidity\": " + String(humidity) +
//                 ", \"gas\": " + String(gas) + "}";

//   server.send(200, "application/json", json);
// }

// void handleGetHtml() {
//   float temperature, humidity, gas;
//   readSensorData(temperature, humidity, gas);

//   String html = "<html><body><h1>Sensor Data</h1>";
//   html += "<p>Temperature: " + String(temperature) + " &deg;C</p>";
//   html += "<p>Humidity: " + String(humidity) + " %</p>";
//   html += "<p>Gas: " + String(gas) + " kOhms</p>";
//   html += "<a href=\"/\">Back to Menu</a>";
//   html += "</body></html>";
//   server.send(200, "text/html", html);

//   tftPrint("Temp: " + String(temperature) + "C\nHum: " + String(humidity) + "%\nGas: " + String(gas) + "kOhms");
// }

// void setupServerEndpoints() {
//   server.on("/", HTTP_GET, handleRoot);
//   server.on("/json", HTTP_GET, handleGetSensorData);
//   server.on("/html", HTTP_GET, handleGetHtml);
// }

// void displaySensorData() {
//   float temperature, humidity, gas;
//   readSensorData(temperature, humidity, gas);

//   tftPrint("Temp: " + String(temperature) + "C\nHum: " + String(humidity) + "%\nGas: " + String(gas) + "kOhms");
// }









// #include <WiFi.h>
// #include <WebServer.h>
// #include <HTTPClient.h>
// #include <TFT_eSPI.h>
// #include <Adafruit_Sensor.h>
// #include <Adafruit_BME680.h>

// // Wi-Fi credentials
// const char* ssid = "IOT";
// const char* password = "password";
// const char* serverIP = "http://192.168.1.100:5000/data";  // Optional server endpoint

// TFT_eSPI tft = TFT_eSPI();
// WebServer server(80);
// Adafruit_BME680 bme; // I2C

// // Button pins
// const int button1Pin = 0; // GPIO0
// const int button2Pin = 2; // GPIO2

// volatile bool refreshData = false;
// volatile int displayMode = 0;

// void IRAM_ATTR handleButton1() {
//   refreshData = true;
// }

// void IRAM_ATTR handleButton2() {
//   displayMode = (displayMode + 1) % 4; // Cycle through 4 display modes
// }

// void setup() {
//   Serial.begin(115200);
//   tft.begin();
//   tft.setRotation(1);
//   tft.fillScreen(TFT_BLACK);
//   tft.setTextColor(TFT_WHITE);

//   pinMode(button1Pin, INPUT_PULLUP);
//   pinMode(button2Pin, INPUT_PULLUP);
//   attachInterrupt(button1Pin, handleButton1, FALLING);
//   attachInterrupt(button2Pin, handleButton2, FALLING);

//   connectToWiFi();
//   if (!bme.begin()) {
//     tftPrint("Could not find a valid BME680 sensor, check wiring!");
//     while (1);
//   }

//   bme.setTemperatureOversampling(BME680_OS_8X);
//   bme.setHumidityOversampling(BME680_OS_2X);
//   bme.setPressureOversampling(BME680_OS_4X);
//   bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
//   bme.setGasHeater(320, 150); // 320*C for 150 ms
  
//   setupServerEndpoints();
//   server.begin();
//   tftPrint("Server started");
//   showMenu();
// }

// void loop() {
//   server.handleClient();

//   if (refreshData) {
//     displaySensorData();
//     refreshData = false;
//   }
// }

// void connectToWiFi() {
//   WiFi.begin(ssid, password);
//   tftPrint("Connecting to Wi-Fi...");

//   while (WiFi.status() != WL_CONNECTED) {
//     delay(1000);
//     tftPrint(".");
//   }

//   tftPrint("Connected! IP Address: " + WiFi.localIP().toString());
// }

// void tftPrint(String message) {
//   tft.fillScreen(TFT_BLACK);
//   tft.setTextSize(3); // Set text size to 3 for increased font size
//   tft.drawString(message, 10, 10); // No need for the fourth parameter
//   Serial.println(message);
// }

// void readSensorData(float& temperature, float& humidity, float& gas, float& altitude) {
//   unsigned long endTime = bme.beginReading();
//   delay(500); // Wait for reading to complete

//   if (bme.endReading() > 0) {
//     temperature = bme.temperature;
//     humidity = bme.humidity;
//     gas = bme.gas_resistance / 1000.0; // Convert to kOhms
//     altitude = bme.readAltitude(1013.25); // Assuming sea level pressure
//   } else {
//     tftPrint("BME680 reading failed");
//   }
// }

// void handleRoot() {
//   String html = "<html><body><h1>ESP32 Sensor Server</h1><ul>";
//   html += "<li><a href=\"/html\">HTML BME680 data</a></li>";
//   html += "<li><a href=\"/json\">JSON BME680 data</a></li>";
//   html += "</ul></body></html>";
//   server.send(200, "text/html", html);
// }

// void handleGetSensorData() {
//   float temperature, humidity, gas, altitude;
//   readSensorData(temperature, humidity, gas, altitude);

//   String json = "{\"temperature\": " + String(temperature) +
//                 ", \"humidity\": " + String(humidity) +
//                 ", \"gas\": " + String(gas) +
//                 ", \"altitude\": " + String(altitude) + "}";

//   server.send(200, "application/json", json);
// }

// void handleGetHtml() {
//   float temperature, humidity, gas, altitude;
//   readSensorData(temperature, humidity, gas, altitude);

//   String html = "<html><body><h1>Sensor Data</h1>";
//   html += "<p>Temperature: " + String(temperature) + " &deg;C</p>";
//   html += "<p>Humidity: " + String(humidity) + " %</p>";
//   html += "<p>Gas: " + String(gas) + " kOhms</p>";
//   html += "<p>Altitude: " + String(altitude) + " m</p>";
//   html += "<a href=\"/\">Back to Menu</a>";
//   html += "</body></html>";
//   server.send(200, "text/html", html);

//   tftPrint("Temp: " + String(temperature) + " C, Hum: " + String(humidity) + " %, Gas: " + String(gas) + " kOhms, Alt: " + String(altitude) + " m");
// }

// void setupServerEndpoints() {
//   server.on("/", HTTP_GET, handleRoot);
//   server.on("/json", HTTP_GET, handleGetSensorData);
//   server.on("/html", HTTP_GET, handleGetHtml);
// }

// void showMenu() {
//   tft.fillScreen(TFT_BLACK);
//   tft.setTextSize(3); // Set text size to 3 for increased font size
//   tft.drawString("Menu:", 10, 10); // No need for the fourth parameter
//   tft.drawString("1. HTML BME680 data", 10, 30); // No need for the fourth parameter
//   tft.drawString("2. JSON BME680 data", 10, 50); // No need for the fourth parameter
// }

// void displaySensorData() {
//   float temperature, humidity, gas, altitude;
//   readSensorData(temperature, humidity, gas, altitude);

//   switch (displayMode) {
//     case 0:
//       tftPrint("Temperature: " + String(temperature) + " C");
//       break;
//     case 1:
//       tftPrint("Humidity: " + String(humidity) + " %");
//       break;
//     case 2:
//       tftPrint("Gas: " + String(gas) + " kOhms");
//       break;
//     case 3:
//       tftPrint("Altitude: " + String(altitude) + " m");
//       break;
//     default:
//       tftPrint("Unknown mode");
//       break;
//   }
// }






// #include <WiFi.h>
// #include <WebServer.h>
// #include <HTTPClient.h>
// #include <TFT_eSPI.h>
// #include <Adafruit_Sensor.h>
// #include <Adafruit_BME680.h>

// // Wi-Fi credentials
// const char* ssid = "IOT";
// const char* password = "password";
// const char* serverIP = "http://192.168.1.100:5000/data";  // Optional server endpoint

// TFT_eSPI tft = TFT_eSPI();
// WebServer server(80);
// Adafruit_BME680 bme; // I2C

// // Button pins
// const int button1Pin = 0; // GPIO0
// const int button2Pin = 2; // GPIO2

// volatile bool refreshData = false;
// volatile int displayMode = 0;

// void IRAM_ATTR handleButton1() {
//   refreshData = true;
// }

// void IRAM_ATTR handleButton2() {
//   displayMode = (displayMode + 1) % 4; // Cycle through 4 display modes
// }

// void setup() {
//   Serial.begin(115200);
//   tft.begin();
//   tft.setRotation(1);
//   tft.fillScreen(TFT_BLACK);
//   tft.setTextColor(TFT_WHITE);

//   pinMode(button1Pin, INPUT_PULLUP);
//   pinMode(button2Pin, INPUT_PULLUP);
//   attachInterrupt(button1Pin, handleButton1, FALLING);
//   attachInterrupt(button2Pin, handleButton2, FALLING);

//   connectToWiFi();
//   if (!bme.begin()) {
//     tftPrint("Could not find a valid BME680 sensor, check wiring!");
//     while (1);
//   }

//   bme.setTemperatureOversampling(BME680_OS_8X);
//   bme.setHumidityOversampling(BME680_OS_2X);
//   bme.setPressureOversampling(BME680_OS_4X);
//   bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
//   bme.setGasHeater(320, 150); // 320*C for 150 ms
  
//   setupServerEndpoints();
//   server.begin();
//   tftPrint("Server started");
//   showMenu();
// }

// void loop() {
//   server.handleClient();

//   if (refreshData) {
//     displaySensorData();
//     refreshData = false;
//   }
// }

// void connectToWiFi() {
//   WiFi.begin(ssid, password);
//   tftPrint("Connecting to Wi-Fi...");

//   while (WiFi.status() != WL_CONNECTED) {
//     delay(1000);
//     tftPrint(".");
//   }

//   tftPrint("Connected! IP Address: " + WiFi.localIP().toString());
// }

// void tftPrint(String message) {
//   tft.fillScreen(TFT_BLACK);
//   tft.setTextSize(3); // Set text size to 3 for increased font size
//   tft.drawString(message, 10, 10, 2);
//   Serial.println(message);
// }

// void readSensorData(float& temperature, float& humidity, float& gas, float& altitude) {
//   unsigned long endTime = bme.beginReading();
//   delay(500); // Wait for reading to complete

//   if (bme.endReading() > 0) {
//     temperature = bme.temperature;
//     humidity = bme.humidity;
//     gas = bme.gas_resistance / 1000.0; // Convert to kOhms
//     altitude = bme.readAltitude(1013.25); // Assuming sea level pressure
//   } else {
//     tftPrint("BME680 reading failed");
//   }
// }

// void handleRoot() {
//   String html = "<html><body><h1>ESP32 Sensor Server</h1><ul>";
//   html += "<li><a href=\"/html\">HTML BME680 data</a></li>";
//   html += "<li><a href=\"/json\">JSON BME680 data</a></li>";
//   html += "</ul></body></html>";
//   server.send(200, "text/html", html);
// }

// void handleGetSensorData() {
//   float temperature, humidity, gas, altitude;
//   readSensorData(temperature, humidity, gas, altitude);

//   String json = "{\"temperature\": " + String(temperature) +
//                 ", \"humidity\": " + String(humidity) +
//                 ", \"gas\": " + String(gas) +
//                 ", \"altitude\": " + String(altitude) + "}";

//   server.send(200, "application/json", json);
// }

// void handleGetHtml() {
//   float temperature, humidity, gas, altitude;
//   readSensorData(temperature, humidity, gas, altitude);

//   String html = "<html><body><h1>Sensor Data</h1>";
//   html += "<p>Temperature: " + String(temperature) + " &deg;C</p>";
//   html += "<p>Humidity: " + String(humidity) + " %</p>";
//   html += "<p>Gas: " + String(gas) + " kOhms</p>";
//   html += "<p>Altitude: " + String(altitude) + " m</p>";
//   html += "<a href=\"/\">Back to Menu</a>";
//   html += "</body></html>";
//   server.send(200, "text/html", html);

//   tftPrint("Temp: " + String(temperature) + " C, Hum: " + String(humidity) + " %, Gas: " + String(gas) + " kOhms, Alt: " + String(altitude) + " m");
// }

// void setupServerEndpoints() {
//   server.on("/", HTTP_GET, handleRoot);
//   server.on("/json", HTTP_GET, handleGetSensorData);
//   server.on("/html", HTTP_GET, handleGetHtml);
// }

// void showMenu() {
//   tft.fillScreen(TFT_BLACK);
//   tft.setTextSize(3); // Set text size to 3 for increased font size
//   tft.drawString("Menu:", 10, 10, 2);
//   tft.drawString("1. HTML BME680 data", 10, 30, 2);
//   tft.drawString("2. JSON BME680 data", 10, 50, 2);
// }

// void displaySensorData() {
//   float temperature, humidity, gas, altitude;
//   readSensorData(temperature, humidity, gas, altitude);

//   switch (displayMode) {
//     case 0:
//       tftPrint("Temperature: " + String(temperature) + " C");
//       break;
//     case 1:
//       tftPrint("Humidity: " + String(humidity) + " %");
//       break;
//     case 2:
//       tftPrint("Gas: " + String(gas) + " kOhms");
//       break;
//     case 3:
//       tftPrint("Altitude: " + String(altitude) + " m");
//       break;
//     default:
//       tftPrint("Unknown mode");
//       break;
//   }
// }







// #include <WiFi.h>
// #include <WebServer.h>
// #include <HTTPClient.h>
// #include <TFT_eSPI.h>
// #include <Adafruit_Sensor.h>
// #include <Adafruit_BME680.h>

// // Wi-Fi credentials
// const char* ssid = "IOT";
// const char* password = "password";
// const char* serverIP = "http://192.168.1.100:5000/data";  // Optional server endpoint

// TFT_eSPI tft = TFT_eSPI();
// WebServer server(80);
// Adafruit_BME680 bme; // I2C

// // Button pins
// const int button1Pin = 0; // GPIO0
// const int button2Pin = 2; // GPIO2

// volatile bool refreshData = false;
// volatile int displayMode = 0;

// void IRAM_ATTR handleButton1() {
//   refreshData = true;
// }

// void IRAM_ATTR handleButton2() {
//   displayMode = (displayMode + 1) % 4; // Cycle through 4 display modes
// }

// void setup() {
//   Serial.begin(115200);
//   tft.begin();
//   tft.setRotation(1);
//   tft.fillScreen(TFT_BLACK);
//   tft.setTextColor(TFT_WHITE);

//   pinMode(button1Pin, INPUT_PULLUP);
//   pinMode(button2Pin, INPUT_PULLUP);
//   attachInterrupt(button1Pin, handleButton1, FALLING);
//   attachInterrupt(button2Pin, handleButton2, FALLING);

//   connectToWiFi();
//   if (!bme.begin()) {
//     tftPrint("Could not find a valid BME680 sensor, check wiring!");
//     while (1);
//   }

//   bme.setTemperatureOversampling(BME680_OS_8X);
//   bme.setHumidityOversampling(BME680_OS_2X);
//   bme.setPressureOversampling(BME680_OS_4X);
//   bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
//   bme.setGasHeater(320, 150); // 320*C for 150 ms
  
//   setupServerEndpoints();
//   server.begin();
//   tftPrint("Server started");
//   showMenu();
// }

// void loop() {
//   server.handleClient();

//   if (refreshData) {
//     displaySensorData();
//     refreshData = false;
//   }
// }

// void connectToWiFi() {
//   WiFi.begin(ssid, password);
//   tftPrint("Connecting to Wi-Fi...");

//   while (WiFi.status() != WL_CONNECTED) {
//     delay(1000);
//     tftPrint(".");
//   }

//   tftPrint("Connected! IP Address: " + WiFi.localIP().toString());
// }

// void tftPrint(String message) {
//   tft.fillScreen(TFT_BLACK);
//   tft.drawString(message, 10, 10, 2);
//   Serial.println(message);
// }

// void readSensorData(float& temperature, float& humidity, float& gas, float& altitude) {
//   unsigned long endTime = bme.beginReading();
//   delay(500); // Wait for reading to complete

//   if (bme.endReading() > 0) {
//     temperature = bme.temperature;
//     humidity = bme.humidity;
//     gas = bme.gas_resistance / 1000.0; // Convert to kOhms
//     altitude = bme.readAltitude(1013.25); // Assuming sea level pressure
//   } else {
//     tftPrint("BME680 reading failed");
//   }
// }

// void handleRoot() {
//   String html = "<html><body><h1>ESP32 Sensor Server</h1><ul>";
//   html += "<li><a href=\"/html\">HTML BME680 data</a></li>";
//   html += "<li><a href=\"/json\">JSON BME680 data</a></li>";
//   html += "</ul></body></html>";
//   server.send(200, "text/html", html);
// }

// void handleGetSensorData() {
//   float temperature, humidity, gas, altitude;
//   readSensorData(temperature, humidity, gas, altitude);

//   String json = "{\"temperature\": " + String(temperature) +
//                 ", \"humidity\": " + String(humidity) +
//                 ", \"gas\": " + String(gas) +
//                 ", \"altitude\": " + String(altitude) + "}";

//   server.send(200, "application/json", json);
// }

// void handleGetHtml() {
//   float temperature, humidity, gas, altitude;
//   readSensorData(temperature, humidity, gas, altitude);

//   String html = "<html><body><h1>Sensor Data</h1>";
//   html += "<p>Temperature: " + String(temperature) + " &deg;C</p>";
//   html += "<p>Humidity: " + String(humidity) + " %</p>";
//   html += "<p>Gas: " + String(gas) + " kOhms</p>";
//   html += "<p>Altitude: " + String(altitude) + " m</p>";
//   html += "<a href=\"/\">Back to Menu</a>";
//   html += "</body></html>";
//   server.send(200, "text/html", html);

//   tftPrint("Temp: " + String(temperature) + " C, Hum: " + String(humidity) + " %, Gas: " + String(gas) + " kOhms, Alt: " + String(altitude) + " m");
// }

// void setupServerEndpoints() {
//   server.on("/", HTTP_GET, handleRoot);
//   server.on("/json", HTTP_GET, handleGetSensorData);
//   server.on("/html", HTTP_GET, handleGetHtml);
// }

// void showMenu() {
//   tft.fillScreen(TFT_BLACK);
//   tft.drawString("Menu:", 10, 10, 2);
//   tft.drawString("1. HTML BME680 data", 10, 30, 2);
//   tft.drawString("2. JSON BME680 data", 10, 50, 2);
// }

// void displaySensorData() {
//   float temperature, humidity, gas, altitude;
//   readSensorData(temperature, humidity, gas, altitude);

//   switch (displayMode) {
//     case 0:
//       tftPrint("Temperature: " + String(temperature) + " C");
//       break;
//     case 1:
//       tftPrint("Humidity: " + String(humidity) + " %");
//       break;
//     case 2:
//       tftPrint("Gas: " + String(gas) + " kOhms");
//       break;
//     case 3:
//       tftPrint("Altitude: " + String(altitude) + " m");
//       break;
//     default:
//       tftPrint("Unknown mode");
//       break;
//   }
// }







// #include <WiFi.h>
// #include <WebServer.h>
// #include <HTTPClient.h>
// #include <TFT_eSPI.h>
// #include <Adafruit_Sensor.h>
// #include <Adafruit_BME680.h>

// // Wi-Fi credentials
// const char* ssid = "IOT";
// const char* password = "password";
// const char* serverIP = "http://192.168.1.100:5000/data";  // Optional server endpoint

// TFT_eSPI tft = TFT_eSPI();
// WebServer server(80);
// Adafruit_BME680 bme; // I2C

// // Button pins
// const int button1Pin = 0; // GPIO0
// const int button2Pin = 2; // GPIO2

// volatile bool refreshData = false;
// volatile int displayMode = 0;

// void IRAM_ATTR handleButton1() {
//   refreshData = true;
// }

// void IRAM_ATTR handleButton2() {
//   displayMode = (displayMode + 1) % 4; // Cycle through 4 display modes
// }

// void setup() {
//   Serial.begin(115200);
//   tft.begin();
//   tft.setRotation(1);
//   tft.fillScreen(TFT_BLACK);
//   tft.setTextColor(TFT_WHITE);

//   pinMode(button1Pin, INPUT_PULLUP);
//   pinMode(button2Pin, INPUT_PULLUP);
//   attachInterrupt(button1Pin, handleButton1, FALLING);
//   attachInterrupt(button2Pin, handleButton2, FALLING);

//   connectToWiFi();
//   if (!bme.begin()) {
//     tftPrint("Could not find a valid BME680 sensor, check wiring!");
//     while (1);
//   }

//   bme.setTemperatureOversampling(BME680_OS_8X);
//   bme.setHumidityOversampling(BME680_OS_2X);
//   bme.setPressureOversampling(BME680_OS_4X);
//   bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
//   bme.setGasHeater(320, 150); // 320*C for 150 ms
  
//   setupServerEndpoints();
//   server.begin();
//   tftPrint("Server started");
//   showMenu();
// }

// void loop() {
//   server.handleClient();

//   if (refreshData) {
//     displaySensorData();
//     refreshData = false;
//   }
// }

// void connectToWiFi() {
//   WiFi.begin(ssid, password);
//   tftPrint("Connecting to Wi-Fi...");

//   while (WiFi.status() != WL_CONNECTED) {
//     delay(1000);
//     tftPrint(".");
//   }

//   tftPrint("Connected! IP Address: " + WiFi.localIP().toString());
// }

// void tftPrint(String message) {
//   tft.fillScreen(TFT_BLACK);
//   tft.drawString(message, 10, 10, 2);
//   Serial.println(message);
// }

// void readSensorData(float& temperature, float& humidity, float& gas, float& altitude) {
//   unsigned long endTime = bme.beginReading();
//   delay(500); // Wait for reading to complete

//   if (bme.endReading() > 0) {
//     temperature = bme.temperature;
//     humidity = bme.humidity;
//     gas = bme.gas_resistance / 1000.0; // Convert to kOhms
//     altitude = bme.readAltitude(1013.25); // Assuming sea level pressure
//   } else {
//     tftPrint("BME680 reading failed");
//   }
// }

// void handleRoot() {
//   String html = "<html><body><h1>ESP32 Sensor Server</h1><ul>";
//   html += "<li><a href=\"/temp\">HTML BME680 data</a></li>";
//   html += "<li><a href=\"/sensor_data\">JSON BME680 data</a></li>";
//   html += "</ul></body></html>";
//   server.send(200, "text/html", html);
// }

// void handleGetSensorData() {
//   float temperature, humidity, gas, altitude;
//   readSensorData(temperature, humidity, gas, altitude);

//   String json = "{\"temperature\": " + String(temperature) +
//                 ", \"humidity\": " + String(humidity) +
//                 ", \"gas\": " + String(gas) +
//                 ", \"altitude\": " + String(altitude) + "}";

//   server.send(200, "application/json", json);
// }

// void handleGetTemp() {
//   float temperature, humidity, gas, altitude;
//   readSensorData(temperature, humidity, gas, altitude);

//   String html = "<html><body><h1>Sensor Data</h1>";
//   html += "<p>Temperature: " + String(temperature) + " &deg;C</p>";
//   html += "<p>Humidity: " + String(humidity) + " %</p>";
//   html += "<p>Gas: " + String(gas) + " kOhms</p>";
//   html += "<p>Altitude: " + String(altitude) + " m</p>";
//   html += "<a href=\"/\">Back to Menu</a>";
//   html += "</body></html>";
//   server.send(200, "text/html", html);

//   tftPrint("Temp: " + String(temperature) + " C, Hum: " + String(humidity) + " %, Gas: " + String(gas) + " kOhms, Alt: " + String(altitude) + " m");
// }

// void setupServerEndpoints() {
//   server.on("/", HTTP_GET, handleRoot);
//   server.on("/sensor_data", HTTP_GET, handleGetSensorData);
//   server.on("/temp", HTTP_GET, handleGetTemp);
// }

// void showMenu() {
//   tft.fillScreen(TFT_BLACK);
//   tft.drawString("Menu:", 10, 10, 2);
//   tft.drawString("1. HTML BME680 data", 10, 30, 2);
//   tft.drawString("2. JSON BME680 data", 10, 50, 2);
// }

// void displaySensorData() {
//   float temperature, humidity, gas, altitude;
//   readSensorData(temperature, humidity, gas, altitude);

//   switch (displayMode) {
//     case 0:
//       tftPrint("Temperature: " + String(temperature) + " C");
//       break;
//     case 1:
//       tftPrint("Humidity: " + String(humidity) + " %");
//       break;
//     case 2:
//       tftPrint("Gas: " + String(gas) + " kOhms");
//       break;
//     case 3:
//       tftPrint("Altitude: " + String(altitude) + " m");
//       break;
//     default:
//       tftPrint("Unknown mode");
//       break;
//   }
// }
