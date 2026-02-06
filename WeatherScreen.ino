/**
 * WeatherDisplayHTTPS.ino
 * Secure weather display using OpenWeather API 3.0
 * Optimized for 240x240 round 1.28" display
 */

#include <Arduino.h>
#include "ArduinoJson.h"
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_GC9A01A.h"
#include <time.h>

#define TFT_CS  D1
#define TFT_DC  D2
#define TFT_MOSI D3
#define TFT_SCL D4
#define TFT_RST D5
#define TFT_MISO D6

WiFiMulti wifiMulti;
Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCL, TFT_RST, TFT_MISO);

#define OPEN_WEATHER_URL "https://api.openweathermap.org/data/3.0/onecall?lat=42.348806&lon=-71.038921&units=imperial&appid=466da4258f6a14aca5962811cd266593"

StaticJsonDocument<4096> doc;

// Timezone offset for Boston (EST = UTC-5, EDT = UTC-4)
// Adjust based on your timezone
const long gmtOffset_sec = -5 * 3600;
const int daylightOffset_sec = 3600;

unsigned long lastWeatherUpdate = 0;
unsigned long lastTimeUpdate = 0;
const unsigned long WEATHER_UPDATE_INTERVAL = 300000; // 5 minutes
const unsigned long TIME_UPDATE_INTERVAL = 60000;     // 1 minute

void setClock() {
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");

  Serial.print(F("Waiting for NTP time sync: "));
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    Serial.print(F("."));
    yield();
    nowSecs = time(nullptr);
  }

  Serial.println();
  struct tm timeinfo;
  localtime_r(&nowSecs, &timeinfo);
  Serial.print(F("Current time: "));
  char buf[26];
  Serial.print(asctime_r(&timeinfo, buf));
}

void setup() {
  Serial.begin(115200);
  
  tft.begin();
  tft.fillScreen(GC9A01A_BLACK);
  tft.setRotation(0);

  Serial.println();
  Serial.println("Weather Display Starting...");

  WiFi.mode(WIFI_STA);
  wifiMulti.addAP("homenet", "aksajinnet87");

  // Wait for WiFi connection
  Serial.print("Waiting for WiFi to connect...");
  while ((wifiMulti.run() != WL_CONNECTED)) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(" connected");

  setClock();
  
  // Force initial weather update
  lastWeatherUpdate = 0;
  lastTimeUpdate = 0;
}

// Draw a sun icon (optimized for round display)
void drawSun(int x, int y, int size, uint16_t color) {
  // Center circle
  tft.fillCircle(x, y, size, color);
  
  // Rays
  for (int i = 0; i < 8; i++) {
    float angle = i * 45 * PI / 180;
    int x1 = x + cos(angle) * (size + 4);
    int y1 = y + sin(angle) * (size + 4);
    int x2 = x + cos(angle) * (size + 12);
    int y2 = y + sin(angle) * (size + 12);
    tft.drawLine(x1, y1, x2, y2, color);
    tft.drawLine(x1+1, y1, x2+1, y2, color);
    tft.drawLine(x1, y1+1, x2, y2+1, color);
  }
}

// Draw a cloud icon
void drawCloud(int x, int y, int size, uint16_t color) {
  int s = size / 2;
  tft.fillCircle(x - s, y, s, color);
  tft.fillCircle(x, y - s/2, s + 2, color);
  tft.fillCircle(x + s, y, s, color);
  tft.fillRect(x - s, y, s * 2, s, color);
}

// Draw a rain cloud
void drawRain(int x, int y, int size, uint16_t color) {
  drawCloud(x, y - 5, size, GC9A01A_WHITE);
  
  // Rain drops
  int drops = size / 5;
  for (int i = 0; i < drops; i++) {
    int drop_x = x - size/2 + i * (size/drops);
    tft.drawLine(drop_x, y + size/2, drop_x, y + size, GC9A01A_CYAN);
    tft.drawLine(drop_x, y + size/2, drop_x, y + size, GC9A01A_CYAN);
  }
}

// Draw a snow cloud
void drawSnow(int x, int y, int size, uint16_t color) {
  drawCloud(x, y - 5, size, GC9A01A_WHITE);
  
  // Snowflakes
  int drops = size / 5;
  for (int i = 0; i < drops; i++) {
    int snow_x = x - size/2 + i * (size/drops);
    int snow_y = y + size/2 + (i % 2) * 3;
    tft.drawCircle(snow_x, snow_y, 2, GC9A01A_WHITE);
    tft.drawPixel(snow_x, snow_y, GC9A01A_WHITE);
  }
}

// Draw a thunderstorm
void drawThunderstorm(int x, int y, int size, uint16_t color) {
  drawCloud(x, y - 5, size, GC9A01A_DARKGREY);
  
  // Lightning bolt
  int bolt_w = size / 3;
  tft.drawLine(x, y + 3, x - bolt_w/2, y + size/2, GC9A01A_YELLOW);
  tft.drawLine(x - bolt_w/2, y + size/2, x + bolt_w/2, y + size/2, GC9A01A_YELLOW);
  tft.drawLine(x + bolt_w/2, y + size/2, x, y + size, GC9A01A_YELLOW);
  tft.fillTriangle(x, y + 3, x - bolt_w/2, y + size/2, x + bolt_w/2, y + size/2, GC9A01A_YELLOW);
}

// Draw partly cloudy
void drawPartlyCloudy(int x, int y, int size, uint16_t color) {
  drawSun(x - size/3, y - size/3, size/2, GC9A01A_YELLOW);
  drawCloud(x + size/4, y + size/4, size/2, GC9A01A_WHITE);
}

// Draw mist/fog
void drawMist(int x, int y, int size, uint16_t color) {
  for (int i = 0; i < 3; i++) {
    int line_y = y - size/2 + i * size/2;
    tft.drawLine(x - size, line_y, x + size, line_y, GC9A01A_LIGHTGREY);
    tft.drawLine(x - size + size/4, line_y + size/4, x + size, line_y + size/4, GC9A01A_LIGHTGREY);
  }
}

void drawWeatherIcon(const char* weather, int x, int y, int size) {
  String weatherStr = String(weather);
  weatherStr.toLowerCase();
  
  if (weatherStr.indexOf("clear") >= 0) {
    drawSun(x, y, size, GC9A01A_YELLOW);
  } 
  else if (weatherStr.indexOf("cloud") >= 0) {
    if (weatherStr.indexOf("few") >= 0 || weatherStr.indexOf("scattered") >= 0) {
      drawPartlyCloudy(x, y, size, GC9A01A_WHITE);
    } else {
      drawCloud(x, y, size, GC9A01A_WHITE);
    }
  }
  else if (weatherStr.indexOf("rain") >= 0 || weatherStr.indexOf("drizzle") >= 0) {
    drawRain(x, y, size, GC9A01A_CYAN);
  }
  else if (weatherStr.indexOf("thunder") >= 0 || weatherStr.indexOf("storm") >= 0) {
    drawThunderstorm(x, y, size, GC9A01A_YELLOW);
  }
  else if (weatherStr.indexOf("snow") >= 0) {
    drawSnow(x, y, size, GC9A01A_WHITE);
  }
  else if (weatherStr.indexOf("mist") >= 0 || weatherStr.indexOf("fog") >= 0 || 
           weatherStr.indexOf("haze") >= 0 || weatherStr.indexOf("smoke") >= 0) {
    drawMist(x, y, size, GC9A01A_LIGHTGREY);
  }
  else {
    // Default to cloud
    drawCloud(x, y, size, GC9A01A_WHITE);
  }
}

void updateTimeDisplay() {
  // Get current time
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  
  char timeStr[10];
  char dateStr[20];
  strftime(timeStr, sizeof(timeStr), "%I:%M %p", &timeinfo);
  strftime(dateStr, sizeof(dateStr), "%a, %b %d", &timeinfo);
  
  // Remove leading zero from hour if present
  if (timeStr[0] == '0') {
    memmove(timeStr, timeStr + 1, strlen(timeStr));
  }

  // Clear only the time area
  tft.fillRect(0, 15, 240, 40, GC9A01A_BLACK);
  
  // Time at top - centered
  tft.setTextSize(3);
  tft.setTextColor(GC9A01A_CYAN);
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((240 - w) / 2, 20);
  tft.print(timeStr);
  
  // Date below time
  tft.setTextSize(1);
  tft.setTextColor(GC9A01A_WHITE);
  tft.getTextBounds(dateStr, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((240 - w) / 2, 45);
  tft.print(dateStr);
}

void displayWeather() {
  // One Call API 3.0 structure
  const char* weather = doc["current"]["weather"][0]["main"];
  const char* description = doc["current"]["weather"][0]["description"];
  float temp = doc["current"]["temp"];
  int hum = doc["current"]["humidity"];

  Serial.printf("Location: Boston\n");
  Serial.printf("Weather: %s (%s)\n", weather, description);
  Serial.printf("Temperature: %.1f°F\n", temp);
  Serial.printf("Humidity: %d%%\n", hum);

  tft.fillScreen(GC9A01A_BLACK);
  tft.setTextWrap(true);
  
  // Display time first
  updateTimeDisplay();
  
  // Weather icon in the center (moved up 20px)
  drawWeatherIcon(weather, 120, 80, 25);
  
  // Temperature - large and centered (moved up 20px)
  char tempStr[10];
  sprintf(tempStr, "%.0f", temp);
  tft.setTextSize(6);
  tft.setTextColor(GC9A01A_ORANGE);
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(tempStr, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((240 - w) / 2 - 15, 125);
  tft.print(tempStr);
  
  // Degree symbol and F
  tft.setTextSize(3);
  tft.print("F");
  
  // Weather description - single line (moved up 20px)
  tft.setTextSize(2);
  tft.setTextColor(GC9A01A_YELLOW);
  
  String descStr = String(description);
  tft.getTextBounds(descStr, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((240 - w) / 2, 175);
  tft.print(descStr);
  
  // Humidity at bottom - centered (moved up 20px)
  char humStr[20];
  sprintf(humStr, "Humidity %d%%", hum);
  tft.setTextSize(1);
  tft.setTextColor(GC9A01A_LIGHTGREY);
  tft.getTextBounds(humStr, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((240 - w) / 2, 203);
  tft.print(humStr);
}

void fetchWeather() {
  if ((wifiMulti.run() == WL_CONNECTED)) {
    NetworkClientSecure *client = new NetworkClientSecure;
    
    if (client) {
      client->setInsecure();  // Skip certificate validation

      HTTPClient https;

      Serial.print("[HTTPS] Connecting to OpenWeather API...\n");
      
      if (https.begin(*client, OPEN_WEATHER_URL)) {
        Serial.print("[HTTPS] GET...\n");
        int httpCode = https.GET();

        if (httpCode > 0) {
          Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

          if (httpCode == HTTP_CODE_OK) {
            String json = https.getString();
            
            DeserializationError err = deserializeJson(doc, json);
            if (err) {
              Serial.printf("Deserialization error: %s\n", err.f_str());
            } else {
              displayWeather();
              lastWeatherUpdate = millis();
            }
          }
        } else {
          Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }

        https.end();
      } else {
        Serial.printf("[HTTPS] Unable to connect\n");
      }

      delete client;
    } else {
      Serial.println("Unable to create secure client");
    }
  }
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Update weather every 5 minutes
  if (currentMillis - lastWeatherUpdate >= WEATHER_UPDATE_INTERVAL || lastWeatherUpdate == 0) {
    Serial.println("Updating weather data...");
    fetchWeather();
  }
  
  // Update time display every minute
  if (currentMillis - lastTimeUpdate >= TIME_UPDATE_INTERVAL || lastTimeUpdate == 0) {
    Serial.println("Updating time...");
    updateTimeDisplay();
    lastTimeUpdate = currentMillis;
  }
  
  delay(1000); // Check every second
}