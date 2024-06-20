#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();  // Initialize the display

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("TFT Display is working!", 10, 10, 4);
}

void loop() {
  // Do nothing
}






// #include <WiFi.h>
// #include <HTTPClient.h>
// #include <TFT_eSPI.h>
// #include "time.h"

// const char* ssid = "IOT";
// const char* password = "saturday";
// const char* ntpServer = "pool.ntp.org";
// const long gmtOffset_sec = 3600 * -8;  // Adjust as needed
// const int daylightOffset_sec = 3600;

// TFT_eSPI tft = TFT_eSPI();

// void setup() {
//   Serial.begin(115200);

//   // Initialize the display
//   tft.init();
//   tft.setRotation(1);
//   tft.fillScreen(TFT_BLACK);
//   tft.setTextColor(TFT_WHITE);

//   // Connect to Wi-Fi
//   tft.println("Connecting to Wi-Fi...");
//   WiFi.begin(ssid, password);

//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     tft.print(".");
//   }

//   tft.fillScreen(TFT_BLACK);
//   tft.println("Connected!");
//   tft.print("IP: ");
//   tft.println(WiFi.localIP());

//   // Verify internet access
//   if (checkInternetAccess()) {
//     tft.println("Internet access verified");
//   } else {
//     tft.println("No internet access");
//     while (true); // Stop here if no internet access
//   }

//   // Synchronize time
//   configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

//   struct tm timeinfo;
//   if (!getLocalTime(&timeinfo)) {
//     tft.println("Failed to obtain time");
//     while (true); // Stop here if unable to obtain time
//   }

//   tft.println("Time set:");
//   char timeStringBuff[50];
//   strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
//   tft.println(timeStringBuff);
// }

// void loop() {
//   // Update the display with the current time every second
//   static unsigned long lastUpdate = 0;
//   if (millis() - lastUpdate >= 1000) {
//     lastUpdate = millis();

//     struct tm timeinfo;
//     if (getLocalTime(&timeinfo)) {
//       char timeStringBuff[50];
//       strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M:%S", &timeinfo);

//       tft.fillScreen(TFT_BLACK);
//       tft.setCursor(0, 0);
//       tft.println("Current Time:");
//       tft.println(timeStringBuff);
//     } else {
//       tft.fillScreen(TFT_BLACK);
//       tft.setCursor(0, 0);
//       tft.println("Failed to obtain time");
//     }
//   }
// }

// bool checkInternetAccess() {
//   HTTPClient http;
//   http.begin("http://google.com");  // URL for checking internet access
//   int httpCode = http.GET();
//   http.end();
//   return httpCode > 0;
// }
