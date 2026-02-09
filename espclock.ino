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
#include <Preferences.h>
#include <WebServer.h>
#include <DNSServer.h>

#define TFT_CS  D1
#define TFT_DC  D2
#define TFT_MOSI D3
#define TFT_SCL D4
#define TFT_RST D5
#define TFT_MISO D6

WiFiMulti wifiMulti;
Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCL, TFT_RST, TFT_MISO);

// Configuration storage
Preferences preferences;
WebServer server(80);
DNSServer dnsServer;

// Configuration variables
String wifiSSID = "";
String wifiPassword = "";
String apiKey = "";
float latitude = 0.0;
float longitude = 0.0;
String userName = "";
String locationName = "";

// Timer variables
int timerDuration = 270; // Default 270 minutes (4.5 hours)
int timerRemaining = 0; // Minutes remaining
unsigned long timerStartTime = 0; // When timer was started
bool timerActive = false; // Is timer running

// AP mode settings
const char* AP_SSID = "WeatherPuck";
const char* AP_PASSWORD = "12345678";
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);

// Weather API URL (will be built dynamically)
String weatherURL = "";

StaticJsonDocument<4096> doc;

// Timezone offset for Boston (EST = UTC-5, EDT = UTC-4)
// Adjust based on your timezone
const long gmtOffset_sec = -5 * 3600;
const int daylightOffset_sec = 3600;

unsigned long lastWeatherUpdate = 0;
unsigned long lastTimeUpdate = 0;
const unsigned long WEATHER_UPDATE_INTERVAL = 3600000; // 1 hour (60 minutes)
const unsigned long TIME_UPDATE_INTERVAL = 30000;      // 30 seconds

bool isConfigured = false;
bool inAPMode = false;

// HTML configuration page
const char CONFIG_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Weather Puck Setup</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 400px;
            margin: 50px auto;
            padding: 20px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
        }
        h1 {
            text-align: center;
            margin-bottom: 30px;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            margin-bottom: 5px;
            font-weight: bold;
        }
        input {
            width: 100%;
            padding: 10px;
            border: none;
            border-radius: 5px;
            box-sizing: border-box;
            font-size: 16px;
        }
        button {
            width: 100%;
            padding: 12px;
            background-color: #4CAF50;
            color: white;
            border: none;
            border-radius: 5px;
            font-size: 18px;
            cursor: pointer;
            margin-top: 20px;
        }
        button:hover {
            background-color: #45a049;
        }
        .reset-button {
            background-color: #f44336;
            margin-top: 40px;
        }
        .reset-button:hover {
            background-color: #da190b;
        }
        .info {
            background-color: rgba(255, 255, 255, 0.2);
            padding: 10px;
            border-radius: 5px;
            margin-bottom: 20px;
            font-size: 14px;
        }
        .warning {
            background-color: rgba(255, 50, 50, 0.3);
            padding: 10px;
            border-radius: 5px;
            margin-top: 20px;
            font-size: 14px;
            text-align: center;
        }
    </style>
</head>
<body>
    <h1>Weather Puck Configuration</h1>
    <div class="info">
        Configure or update your Weather Puck settings. Access this page anytime at the device's IP address.
    </div>
    <form action="/save" method="POST">
        <div class="form-group">
            <label for="name">Your Name:</label>
            <input type="text" id="name" name="name" required>
        </div>
        
        <div class="form-group">
            <label for="ssid">WiFi Network Name:</label>
            <input type="text" id="ssid" name="ssid" required>
        </div>
        
        <div class="form-group">
            <label for="password">WiFi Password:</label>
            <input type="password" id="password" name="password" required>
        </div>
        
        <div class="form-group">
            <label for="apikey">OpenWeather API Key:</label>
            <input type="text" id="apikey" name="apikey" required>
        </div>
        
        <div class="form-group">
            <label for="location">Location Name:</label>
            <input type="text" id="location" name="location" placeholder="e.g., Boston, MA" required>
        </div>
        
        <div class="form-group">
            <label for="lat">Latitude:</label>
            <input type="number" id="lat" name="lat" step="0.000001" required>
        </div>
        
        <div class="form-group">
            <label for="lon">Longitude:</label>
            <input type="number" id="lon" name="lon" step="0.000001" required>
        </div>
        
        <button type="submit">Save Configuration</button>
    </form>
    
    <form action="/reset" method="POST" onsubmit="return confirm('Are you sure you want to factory reset? This will delete all settings and restart in setup mode.');">
        <button type="submit" class="reset-button">Factory Reset</button>
    </form>
    
    <div class="warning">
        ⚠️ Factory Reset will erase all settings and restart the device in setup mode.
    </div>
</body>
</html>
)rawliteral";

// Success page
const char SUCCESS_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Configuration Saved</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 400px;
            margin: 50px auto;
            padding: 20px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            text-align: center;
        }
        h1 { color: #4CAF50; }
        p { font-size: 18px; margin: 20px 0; }
    </style>
</head>
<body>
    <h1>✓ Configuration Saved!</h1>
    <p>Your Weather Puck is now configured.</p>
    <p>The device will restart and connect to your WiFi network.</p>
    <p>You can close this page.</p>
</body>
</html>
)rawliteral";

// Factory reset page
const char RESET_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Factory Reset</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 400px;
            margin: 50px auto;
            padding: 20px;
            background: linear-gradient(135deg, #f44336 0%, #e91e63 100%);
            color: white;
            text-align: center;
        }
        h1 { color: #ffeb3b; }
        p { font-size: 18px; margin: 20px 0; }
    </style>
</head>
<body>
    <h1>⚠️ Factory Reset Complete</h1>
    <p>All settings have been erased.</p>
    <p>The device will restart in setup mode.</p>
    <p>Connect to WiFi: WeatherPuck</p>
    <p>Password: 12345678</p>
</body>
</html>
)rawliteral";

// Load configuration from preferences
void loadConfiguration() {
  preferences.begin("weather-puck", false);
  
  wifiSSID = preferences.getString("wifi_ssid", "");
  wifiPassword = preferences.getString("wifi_pass", "");
  apiKey = preferences.getString("api_key", "");
  latitude = preferences.getFloat("latitude", 0.0);
  longitude = preferences.getFloat("longitude", 0.0);
  userName = preferences.getString("user_name", "");
  locationName = preferences.getString("location", "");
  
  // Check if device is configured
  isConfigured = (wifiSSID.length() > 0 && apiKey.length() > 0 && latitude != 0.0);
  
  if (isConfigured) {
    // Build the weather API URL
    weatherURL = "https://api.openweathermap.org/data/3.0/onecall?lat=" + 
                 String(latitude, 6) + "&lon=" + String(longitude, 6) + 
                 "&units=imperial&appid=" + apiKey;
  }
  
  preferences.end();
}

// Save configuration to preferences
void saveConfiguration() {
  preferences.begin("weather-puck", false);
  
  preferences.putString("wifi_ssid", wifiSSID);
  preferences.putString("wifi_pass", wifiPassword);
  preferences.putString("api_key", apiKey);
  preferences.putFloat("latitude", latitude);
  preferences.putFloat("longitude", longitude);
  preferences.putString("user_name", userName);
  preferences.putString("location", locationName);
  
  preferences.end();
}

// Clear all stored configuration
void clearConfiguration() {
  preferences.begin("weather-puck", false);
  preferences.clear();
  preferences.end();
}

// Handle root page
void handleRoot() {
  // Pre-fill form with current values if configured
  String html = String(CONFIG_PAGE);
  
  if (isConfigured) {
    // Add JavaScript to pre-fill the form with current values
    String script = "<script>window.onload = function() {";
    script += "document.getElementById('name').value = '" + userName + "';";
    script += "document.getElementById('ssid').value = '" + wifiSSID + "';";
    script += "document.getElementById('password').value = '" + wifiPassword + "';";
    script += "document.getElementById('apikey').value = '" + apiKey + "';";
    script += "document.getElementById('location').value = '" + locationName + "';";
    script += "document.getElementById('lat').value = '" + String(latitude, 6) + "';";
    script += "document.getElementById('lon').value = '" + String(longitude, 6) + "';";
    script += "};</script></body>";
    
    html.replace("</body>", script);
  }
  
  server.send(200, "text/html", html);
}

// Handle configuration save
void handleSave() {
  if (server.hasArg("name") && server.hasArg("ssid") && server.hasArg("password") && 
      server.hasArg("apikey") && server.hasArg("lat") && server.hasArg("lon") && 
      server.hasArg("location")) {
    
    userName = server.arg("name");
    wifiSSID = server.arg("ssid");
    wifiPassword = server.arg("password");
    apiKey = server.arg("apikey");
    latitude = server.arg("lat").toFloat();
    longitude = server.arg("lon").toFloat();
    locationName = server.arg("location");
    
    saveConfiguration();
    
    server.send(200, "text/html", SUCCESS_PAGE);
    
    delay(3000);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Missing required parameters");
  }
}

// Handle factory reset
void handleReset() {
  clearConfiguration();
  server.send(200, "text/html", RESET_PAGE);
  
  delay(3000);
  ESP.restart();
}

// Display text centered on screen
void displayCenteredText(String text, int y, int textSize, uint16_t color) {
  tft.setTextSize(textSize);
  tft.setTextColor(color);
  
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((240 - w) / 2, y);
  tft.print(text);
}

// Show welcome screen
void showWelcomeScreen() {
  tft.fillScreen(GC9A01A_BLACK);
  
  // Draw a decorative circle
  tft.drawCircle(120, 80, 40, GC9A01A_CYAN);
  tft.drawCircle(120, 80, 41, GC9A01A_CYAN);
  
  displayCenteredText("Welcome to", 130, 2, GC9A01A_WHITE);
  displayCenteredText("Weather Puck", 150, 3, GC9A01A_CYAN);
}

// Show AP mode screen
void showAPModeScreen() {
  tft.fillScreen(GC9A01A_BLACK);
  
  displayCenteredText("Setup Mode", 40, 2, GC9A01A_YELLOW);
  
  displayCenteredText("Connect to WiFi:", 80, 1, GC9A01A_WHITE);
  displayCenteredText(AP_SSID, 100, 2, GC9A01A_CYAN);
  
  displayCenteredText("Password:", 130, 1, GC9A01A_WHITE);
  displayCenteredText(AP_PASSWORD, 150, 2, GC9A01A_GREEN);
  
  displayCenteredText("Then open browser:", 180, 1, GC9A01A_WHITE);
  displayCenteredText("192.168.4.1", 200, 1, GC9A01A_CYAN);
}

// Show greeting screen
void showGreetingScreen() {
  tft.fillScreen(GC9A01A_BLACK);
  
  // Draw a decorative circle
  tft.drawCircle(120, 80, 35, GC9A01A_GREEN);
  tft.fillCircle(120, 80, 30, GC9A01A_GREEN);
  
  // Draw a simple smiley face
  tft.fillCircle(110, 75, 3, GC9A01A_BLACK);
  tft.fillCircle(130, 75, 3, GC9A01A_BLACK);
  tft.drawCircle(120, 80, 20, GC9A01A_BLACK);
  tft.drawLine(110, 85, 130, 85, GC9A01A_BLACK);
  tft.drawLine(110, 86, 130, 86, GC9A01A_BLACK);
  
  String greeting = "Hello";
  if (userName.length() > 0) {
    greeting += " " + userName;
  }
  greeting += "!";
  
  displayCenteredText(greeting, 140, 2, GC9A01A_WHITE);
  
  delay(3000); // Show greeting for 3 seconds
}

// Start AP mode for configuration
void startAPMode() {
  Serial.println("Starting AP Mode...");
  inAPMode = true;
  
  // Clear any existing WiFi configuration
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(200);
  
  // Set to AP mode
  WiFi.mode(WIFI_AP);
  delay(200);
  
  // Configure and start AP
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD);
  
  if (!apStarted) {
    Serial.println("Failed to start AP! Restarting...");
    delay(1000);
    ESP.restart();
  }
  
  // Start DNS server to redirect all requests to our IP
  dnsServer.start(DNS_PORT, "*", apIP);
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/reset", HTTP_POST, handleReset);
  server.onNotFound(handleRoot); // Redirect all other pages to root
  
  server.begin();
  
  Serial.println("AP Mode started successfully");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  
  showAPModeScreen();
}

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

// Start web server for configuration in normal mode
void startWebServer() {
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/reset", HTTP_POST, handleReset);
  
  server.begin();
  
  Serial.println("Web server started");
  Serial.print("Configuration page available at: http://");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100); // Set timeout for serial reads
  
  // Print welcome message with commands
  Serial.println("\n\n=================================");
  Serial.println("    Weather Puck v1.0");
  Serial.println("=================================");
  Serial.println("Type 'help' for available commands");
  Serial.println("=================================\n");
  
  tft.begin();
  tft.fillScreen(GC9A01A_BLACK);
  tft.setRotation(0);

  Serial.println();
  Serial.println("Weather Puck Starting...");

  // Load configuration from flash
  loadConfiguration();
  
  if (!isConfigured) {
    // First time setup - show welcome and start AP mode
    Serial.println("Device not configured. Starting setup mode...");
    showWelcomeScreen();
    delay(3000);
    startAPMode();
  } else {
    // Regular startup - show greeting and connect to WiFi
    Serial.println("Device configured. Starting normal mode...");
    showGreetingScreen();
    
    WiFi.mode(WIFI_STA);
    wifiMulti.addAP(wifiSSID.c_str(), wifiPassword.c_str());

    // Show connecting status
    tft.fillScreen(GC9A01A_BLACK);
    displayCenteredText("Connecting to WiFi", 90, 2, GC9A01A_WHITE);
    displayCenteredText(wifiSSID, 120, 2, GC9A01A_YELLOW);
    
    // Try to connect to WiFi (20 seconds timeout)
    Serial.print("Connecting to WiFi...");
    int attempts = 0;
    while ((wifiMulti.run() != WL_CONNECTED) && attempts < 40) {
      Serial.print(".");
      
      // Show progress dots on screen
      if (attempts % 10 == 0) {
        String dots = "";
        for (int i = 0; i < (attempts / 10); i++) {
          dots += ".";
        }
        tft.fillRect(0, 150, 240, 20, GC9A01A_BLACK);
        displayCenteredText(dots, 150, 2, GC9A01A_WHITE);
      }
      
      delay(500);
      attempts++;
    }
    
    if (wifiMulti.run() == WL_CONNECTED) {
      Serial.println(" connected!");
      
      tft.fillScreen(GC9A01A_BLACK);
      displayCenteredText("Connected!", 100, 3, GC9A01A_GREEN);
      displayCenteredText("IP: " + WiFi.localIP().toString(), 140, 1, GC9A01A_WHITE);
      delay(2000);
      
      setClock();
      
      // Start web server for configuration updates
      startWebServer();
      
      // Force initial weather update
      lastWeatherUpdate = 0;
      lastTimeUpdate = 0;
    } else {
      // WiFi connection failed - immediately fall back to AP mode
      Serial.println(" failed!");
      Serial.println("WiFi connection failed. Starting AP mode...");
      
      tft.fillScreen(GC9A01A_BLACK);
      displayCenteredText("WiFi Failed!", 80, 2, GC9A01A_RED);
      displayCenteredText("Starting", 110, 2, GC9A01A_YELLOW);
      displayCenteredText("Setup Mode", 140, 2, GC9A01A_YELLOW);
      delay(3000);
      
      // Clear WiFi settings and start AP mode for reconfiguration
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      delay(100);
      
      // Start AP mode for reconfiguration
      startAPMode();
    }
  }
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

// Draw simple horizontal progress bar below humidity
void drawTimerBar() {
  // Clear the timer bar area
  tft.fillRect(50, 205, 140, 12, GC9A01A_BLACK);
  
  if (!timerActive) {
    // Just show empty box when no timer
    tft.drawRect(60, 208, 120, 6, GC9A01A_DARKGREY);
    return;
  }
  
  // Calculate progress (1.0 = full bar, 0.0 = empty)
  float progress = (float)timerRemaining / (float)timerDuration;
  
  // Progress bar dimensions - smaller and centered
  int barX = 60;
  int barY = 208;
  int barWidth = 120;
  int barHeight = 6;
  
  // Draw bar outline
  tft.drawRect(barX, barY, barWidth, barHeight, GC9A01A_WHITE);
  
  // Calculate filled width
  int filledWidth = (int)((barWidth - 2) * progress);
  
  // Color based on time remaining
  uint16_t color;
  if (progress > 0.5) {
    color = GC9A01A_GREEN;
  } else if (progress > 0.25) {
    color = GC9A01A_YELLOW;
  } else {
    color = GC9A01A_RED;
  }
  
  // Draw filled portion
  if (filledWidth > 0) {
    tft.fillRect(barX + 1, barY + 1, filledWidth, barHeight - 2, color);
  }
  
  // Show minutes remaining on the right
  tft.setTextSize(1);
  tft.setTextColor(GC9A01A_WHITE);
  tft.setCursor(barX + barWidth + 5, barY - 2);
  tft.print(String(timerRemaining) + "m");
}

void displayWeather() {
  // One Call API 3.0 structure
  const char* weather = doc["current"]["weather"][0]["main"];
  const char* description = doc["current"]["weather"][0]["description"];
  float temp = doc["current"]["temp"];
  int hum = doc["current"]["humidity"];

  Serial.printf("Location: %s\n", locationName.c_str());
  Serial.printf("Weather: %s (%s)\n", weather, description);
  Serial.printf("Temperature: %.1f°F\n", temp);
  Serial.printf("Humidity: %d%%\n", hum);

  tft.fillScreen(GC9A01A_BLACK);
  tft.setTextWrap(true);
  
  // Display time first
  updateTimeDisplay();
  
  // Location name at top (below date)
  tft.setTextSize(1);
  tft.setTextColor(GC9A01A_LIGHTGREY);
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(locationName, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((240 - w) / 2, 60);
  tft.print(locationName);
  
  // Weather icon in the center
  drawWeatherIcon(weather, 120, 95, 10);
  
  // Temperature - large and centered (moved up 20px)
  char tempStr[10];
  sprintf(tempStr, "%.0f", temp);
  tft.setTextSize(6);
  tft.setTextColor(GC9A01A_ORANGE);
  tft.getTextBounds(tempStr, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((240 - w) / 2 - 15, 125);
  tft.print(tempStr);
  
  // Degree symbol and F
  tft.setTextSize(3);
  tft.print("F");
  
  // Weather description - moved up slightly
  tft.setTextSize(2);
  tft.setTextColor(GC9A01A_YELLOW);
  
  String descStr = String(description);
  tft.getTextBounds(descStr, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((240 - w) / 2, 170);
  tft.print(descStr);
  
  // Humidity - moved below description
  char humStr[20];
  sprintf(humStr, "Humidity %d%%", hum);
  tft.setTextSize(1);
  tft.setTextColor(GC9A01A_LIGHTGREY);
  tft.getTextBounds(humStr, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((240 - w) / 2, 190);
  tft.print(humStr);
  
  // Draw timer bar below humidity
  drawTimerBar();
}

void fetchWeather() {
  if ((wifiMulti.run() == WL_CONNECTED)) {
    NetworkClientSecure *client = new NetworkClientSecure;
    
    if (client) {
      client->setInsecure();  // Skip certificate validation

      HTTPClient https;

      Serial.print("[HTTPS] Connecting to OpenWeather API...\n");
      Serial.println("URL: " + weatherURL);
      
      if (https.begin(*client, weatherURL)) {
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
  // Check for serial commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "help") {
      Serial.println("\n=== Weather Puck Commands ===");
      Serial.println("start_timer HH:MM - Start timer (e.g., start_timer 2:30 for 2 hours 30 minutes)");
      Serial.println("stop_timer        - Stop current timer");
      Serial.println("status            - Show timer status");
      Serial.println("help              - Show this help message");
      Serial.println("=============================\n");
    } else if (command.startsWith("start_timer ")) {
      String timeStr = command.substring(12);
      int colonIndex = timeStr.indexOf(':');
      
      if (colonIndex > 0) {
        int hours = timeStr.substring(0, colonIndex).toInt();
        int minutes = timeStr.substring(colonIndex + 1).toInt();
        int totalMinutes = (hours * 60) + minutes;
        
        if (totalMinutes > 0 && totalMinutes <= 9999) {
          timerDuration = totalMinutes;
          timerRemaining = totalMinutes;
          timerStartTime = millis();
          timerActive = true;
          Serial.printf("Timer started: %d hours %d minutes (%d total minutes)\n", hours, minutes, totalMinutes);
          drawTimerBar(); // Immediately show the timer bar
        } else {
          Serial.println("Invalid timer duration. Use format HH:MM (max 9999 minutes)");
        }
      } else {
        Serial.println("Invalid format. Use: start_timer HH:MM (e.g., start_timer 2:30)");
      }
    } else if (command == "stop_timer") {
      timerActive = false;
      timerRemaining = 0;
      Serial.println("Timer stopped");
      drawTimerBar(); // Update to show empty box
    } else if (command == "status") {
      if (timerActive) {
        int hours = timerRemaining / 60;
        int minutes = timerRemaining % 60;
        Serial.printf("Timer active: %d:%02d remaining (%d minutes)\n", hours, minutes, timerRemaining);
      } else {
        Serial.println("No timer active");
      }
    } else if (command.length() > 0) {
      Serial.println("Unknown command. Type 'help' for available commands.");
    }
  }
  
  if (inAPMode) {
    // Handle DNS and web server in AP mode
    dnsServer.processNextRequest();
    server.handleClient();
  } else {
    // Normal operation mode
    server.handleClient(); // Handle web server requests
    
    unsigned long currentMillis = millis();
    
    // Update timer every minute if active
    static unsigned long lastMinuteUpdate = 0;
    if (timerActive && (currentMillis - lastMinuteUpdate >= 60000 || lastMinuteUpdate == 0)) {
      // Calculate remaining time
      unsigned long elapsed = (currentMillis - timerStartTime) / 60000; // Convert to minutes
      int newRemaining = timerDuration - elapsed;
      
      if (newRemaining <= 0) {
        timerRemaining = 0;
        timerActive = false;
        Serial.println("Timer complete!");
        drawTimerBar(); // Show empty box
      } else if (newRemaining != timerRemaining) {
        timerRemaining = newRemaining;
        drawTimerBar(); // Update the timer bar
      }
      lastMinuteUpdate = currentMillis;
    }
    
    // Update weather every hour
    if (currentMillis - lastWeatherUpdate >= WEATHER_UPDATE_INTERVAL || lastWeatherUpdate == 0) {
      Serial.println("Updating weather data...");
      fetchWeather();
    }
    
    // Update time display every 30 seconds
    if (currentMillis - lastTimeUpdate >= TIME_UPDATE_INTERVAL || lastTimeUpdate == 0) {
      updateTimeDisplay();
      lastTimeUpdate = currentMillis;
    }
  }
  
  delay(10); // Small delay to prevent watchdog issues
}