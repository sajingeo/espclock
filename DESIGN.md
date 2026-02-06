# Design

This is ESP32 powered clock using an ESP32-C6 connected to 1.28 inch round TFT screen (GC9A01A).

## Features Implemented

### Start up
When it starts up without WiFi credentials for the first time, it shows "Welcome to Weather Puck" with a decorative circle animation.

### Onboarding
The ESP32 advertises an AP (Access Point) that users can connect to:
- **SSID**: WeatherPuck
- **Password**: 12345678
- **Configuration URL**: http://192.168.4.1

The configuration webpage allows users to enter:
- User's name (for personalized greeting)
- WiFi network credentials (SSID and password)
- OpenWeather API key
- Location name (e.g., "Boston, MA")
- Latitude and longitude coordinates

The screen displays the AP SSID and password during setup mode. Once configured, the device saves all settings to flash memory and restarts.

### Regular Startup
On power on, the device:
1. Shows "Hello [USER NAME]!" with a smiley face animation for 3 seconds
2. Attempts to connect to WiFi (up to 3 attempts)
3. If connection fails after 3 attempts, automatically falls back to AP mode
4. On successful connection, displays:
   - Current time (updates every second)
   - Date
   - Location name
   - Weather icon (animated based on conditions)
   - Temperature in Fahrenheit
   - Weather description
   - Humidity percentage
   - Device IP address (small text at bottom)

### Web Configuration Portal
The device runs a web server continuously when connected to WiFi:
- Accessible at the device's IP address
- Pre-fills form with current configuration values
- Allows updating any settings without factory reset
- Includes factory reset button to wipe all settings

### Weather Display
- Updates weather data every hour
- Updates time display every second
- Shows various weather icons:
  - Sun (clear weather)
  - Clouds (cloudy conditions)
  - Rain drops (rain/drizzle)
  - Snowflakes (snow)
  - Lightning bolt (thunderstorm)
  - Partly cloudy (sun with clouds)
  - Mist/fog lines

### Data Persistence
All configuration is stored in ESP32's flash memory using the Preferences library:
- WiFi credentials
- API key
- Location information
- User name
- No external storage required