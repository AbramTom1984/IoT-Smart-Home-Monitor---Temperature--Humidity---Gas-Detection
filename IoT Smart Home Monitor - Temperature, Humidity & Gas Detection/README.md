# 🏠 IoT Smart Home Monitor - Temperature, Humidity & Gas Detection

Real-time air quality monitoring system with WiFi connectivity, secure web dashboard, and multi-gas detection.

## Features

✅ Real-time Monitoring
- Temperature & Humidity (DHT22)
- CO & CO2 detection (MQ-135 sensor)
- Toluene, NH4, Acetone levels
- Moving average filtering for accurate readings

✅ Web Dashboard
- Secure HTTP Basic Authentication (login/password)
- Responsive design
- Live data updates every 3 seconds
- Dark mode UI

✅ OLED Display
- 1.3" SH1106 128x64 display
- 4-page rotation showing different data sets
- Real-time environmental feedback

✅ WiFi Connectivity
- REST API endpoints (/api/json, /data)
- JSON & CSV data formats
- Remote access from any device on the network

✅ Security
- HTTP Basic Authentication on all endpoints
- Configurable username/password
- Protected API access

## Hardware Required

- ESP32 microcontroller
- DHT22 temperature/humidity sensor
- MQ-135 gas sensor (air quality)
- SH1106 1.3" OLED display
- 12V Li-ion battery (7Ah recommended)
- DC-DC converter (12V → 5V/3A)

## Quick Start

1. Install Libraries in Arduino IDE:DHT Library by Adafruit
Adafruit GFX Library
Adafruit SH110X Library
2. Configure WiFi & Auth
`cpp
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";
const char* http_username = "admin";
const char* http_password = "airquality123";Upload & MonitorOpen Serial Monitor (115200 baud)Note the IP address displayedAccess dashboard at http://<ESP32_IP>API EndpointsDashboardGET / (requires authentication)JSON DataGET /api/json (requires authentication)
Returns: {"temperature": 22.5, "humidity": 45.2, "co": 12.3, ...}Raw DataGET /data (requires authentication)
Returns: CSV format (temperature, humidity, co, co2, ...)Pin ConfigurationDHT22:    GPIO 4
I2C SDA:  GPIO 1
I2C SCL:  GPIO 2
MQ-135:   GPIO 3 (ADC)PerformanceCalibration: 30 seconds at startupUpdate Rate: ~1 second per measurementWiFi Range: ~50m (depends on router)Battery Life: 22+ hours per 7Ah batterySecurity NotesChange default credentials immediatelyUse on trusted networks onlyEnable WPA2/WPA3 on your WiFi routerDo NOT expose to the internet without VPN/reverse proxyFuture Enhancements�HTTPS support�SD card logging�InfluxDB integration�Grafana dashboards�Mobile app�MQTT supportLicenseMIT License - Free for personal & educational useBuilt with ❤️ for open-source IoT enthusiasts