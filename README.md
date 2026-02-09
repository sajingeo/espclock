# ESP Clock - Weather Puck

A beautiful ESP32-based weather clock with a 1.28" round TFT display that shows time, weather, and location information.

## Features

- 🕐 **Real-time Clock**: Displays current time with updates every 30 seconds
- 🌤️ **Weather Display**: Shows temperature, weather conditions, and humidity
- 📍 **Location Aware**: Displays weather for your configured location
- ⏱️ **Visual Timer**: Countdown timer with progress bar controlled via serial commands
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

## Timer Feature

The Weather Puck includes a visual countdown timer that displays as a progress bar on the screen.

### Serial Commands

Connect via Serial Monitor (115200 baud) and use these commands:

- `help` - Show available commands
- `start_timer HH:MM` - Start timer (e.g., `start_timer 2:30` for 2.5 hours)
- `stop_timer` - Stop the current timer
- `status` - Check timer status

### Python Control Script

A Python script (`timer.py`) is included for easy timer control from your computer:

#### Installation

```bash
# Install required library
pip3 install --break-system-packages pyserial
```

#### Usage

```bash
# List available serial ports
python3 timer.py -l

# Start timer (auto-detect port)
python3 timer.py -t 1:30    # 1 hour 30 minutes

# Start timer (specific port)
python3 timer.py -p /dev/tty.usbmodem2134201 -t 0:45

# Stop timer
python3 timer.py -s

# Check status
python3 timer.py -r

# Help
python3 timer.py -h
```

**Note**: Close Arduino Serial Monitor before using the Python script.

## Normal Operation

After configuration, the device will:

1. Show personalized greeting: "Hello [Your Name]!"
2. Attempt to connect to WiFi
3. If WiFi fails, automatically enter setup mode
4. Display the main weather screen with:
   - Current time (updates every 30 seconds)
   - Date
   - Location name
   - Animated weather icon
   - Temperature in Fahrenheit
   - Weather description
   - Humidity percentage
   - Timer progress bar (when active)

## Reconfiguration

You can update settings at any time:

1. Connect to the same WiFi network as the device
2. Open `http://[device-ip]/` in your browser (IP shown during startup)
3. Update any settings (form pre-fills with current values)
4. Save to apply changes

## Factory Reset

Via Web Interface:
- Access configuration page
- Click "Factory Reset" button
- Confirm the action

## Update Intervals

- **Time Display**: Updates every 30 seconds
- **Weather Data**: Updates every hour
- **Timer Bar**: Updates every minute when active
- **NTP Time Sync**: On boot and periodically

## Troubleshooting

### Device won't connect to WiFi
- Check WiFi credentials are correct
- Ensure WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
- Device will automatically enter setup mode if connection fails

### Weather not updating
- Verify OpenWeather API key is valid
- Check API key has One Call API 3.0 access
- Ensure coordinates are correct

### Timer not working
- Make sure Serial Monitor is closed when using Python script
- Check serial port with `python3 timer.py -l`
- Use specific port: `python3 timer.py -p /dev/tty.usbmodem... -t 1:00`

### Display issues
- Check all wire connections
- Verify pin assignments match your setup
- Ensure display is GC9A01A compatible

## Display Layout

```
    [Time HH:MM AM/PM]
    [Day, Month Date]
    [Location Name]
    
    [Weather Icon]
    
    [Temperature °F]
    [Weather Description]
    [Humidity %]
    
    [Timer Bar ========----] XXm
```

## Serial Monitor Output

When connected via Serial Monitor (115200 baud), you'll see:
- Startup messages
- WiFi connection status
- Weather update notifications
- Timer commands and responses
- Debug information

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
