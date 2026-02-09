# ESP Clock - Weather Puck

A beautiful ESP32-based weather clock with a 1.28" round TFT display that shows time, weather, and location information.

## Features

- 🕐 **Real-time Clock**: Displays current time with second-by-second updates
- 🌤️ **Weather Display**: Shows temperature, weather conditions, and humidity
- 📍 **Location Aware**: Displays weather for your configured location
- 🔧 **Easy Setup**: Web-based configuration portal
- 💾 **Persistent Storage**: Saves all settings to flash memory
- 🔄 **Auto-recovery**: Falls back to setup mode if WiFi fails
- 🎨 **Beautiful UI**: Animated weather icons and colorful display

## Hardware Requirements

- **Microcontroller**: ESP32-C6
- **Display**: 1.28" Round TFT LCD (GC9A01A driver)
- **Connections**:
  - TFT_CS → D1
  - TFT_DC → D2
  - TFT_MOSI → D3
  - TFT_SCL → D4
  - TFT_RST → D5
  - TFT_MISO → D6

## Software Dependencies

- Arduino IDE with ESP32 board support
- Libraries:
  - ArduinoJson
  - Adafruit GFX
  - Adafruit GC9A01A
  - WiFi (ESP32)
  - HTTPClient
  - Preferences
  - WebServer
  - DNSServer

## Initial Setup

### First Time Configuration

1. **Power on the device** - You'll see "Welcome to Weather Puck"

2. **Connect to the setup WiFi**:
   - Network Name: `WeatherPuck`
   - Password: `12345678`

3. **Open configuration page**:
   - Navigate to `http://192.168.4.1` in your browser
   - Or just open any webpage (captive portal will redirect)

4. **Enter your settings**:
   - **Your Name**: For personalized greeting
   - **WiFi Network Name**: Your home WiFi SSID
   - **WiFi Password**: Your home WiFi password
   - **OpenWeather API Key**: Get from [OpenWeatherMap](https://openweathermap.org/api)
   - **Location Name**: e.g., "Boston, MA"
   - **Latitude**: Your location's latitude
   - **Longitude**: Your location's longitude

5. **Save Configuration** - Device will restart and connect to your WiFi

## Getting an OpenWeather API Key

1. Sign up at [OpenWeatherMap](https://openweathermap.org/api)
2. Go to "API Keys" in your account
3. Generate a new API key
4. Use the "One Call API 3.0" subscription (free tier available)

## Finding Your Coordinates

- Use [Google Maps](https://maps.google.com):
  1. Right-click on your location
  2. Click on the coordinates to copy them
- Or use [LatLong.net](https://www.latlong.net/)

## Normal Operation

After configuration, the device will:

1. Show personalized greeting: "Hello [Your Name]!"
2. Attempt to connect to WiFi (20 second timeout)
3. If WiFi fails, automatically enter setup mode
4. Display the main weather screen with:
   - Current time (updates every second)
   - Date
   - Location name
   - Animated weather icon
   - Temperature in Fahrenheit
   - Weather description
   - Humidity percentage
   - Device IP address (bottom of screen)

## Reconfiguration

You can update settings at any time:

1. Find the device IP address (shown at bottom of weather display)
2. Open `http://[device-ip]/` in your browser
3. Update any settings (form pre-fills with current values)
4. Save to apply changes

## Factory Reset

Two ways to factory reset:

1. **Via Web Interface**:
   - Access configuration page
   - Click "Factory Reset" button
   - Confirm the action

2. **Automatic Fallback**:
   - Device automatically enters setup mode if WiFi connection fails

## Update Intervals

- **Time Display**: Updates every second
- **Weather Data**: Updates every hour
- **NTP Time Sync**: On boot and periodically

## Troubleshooting

### Device won't connect to WiFi
- Check WiFi credentials are correct
- Ensure WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
- Device will automatically enter setup mode if connection fails (20 second timeout)

### Weather not updating
- Verify OpenWeather API key is valid
- Check API key has One Call API 3.0 access
- Ensure coordinates are correct

### Display issues
- Check all wire connections
- Verify pin assignments match your setup
- Ensure display is GC9A01A compatible

### Can't access configuration page
- Check device IP on the display
- Ensure you're on the same network
- Try `http://` not `https://`

## LED Status (if implemented)

- **Solid**: Normal operation
- **Blinking**: Connecting to WiFi
- **Fast Blink**: AP mode active

## Security Notes

- Change the default AP password in code if desired
- API keys are stored in flash (not encrypted)
- Web interface has no authentication (local network only)

## License

This project is open source and available under the [MIT License](LICENSE).

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Acknowledgments

- OpenWeatherMap for weather API
- Adafruit for display libraries
- ESP32 community for examples and support
