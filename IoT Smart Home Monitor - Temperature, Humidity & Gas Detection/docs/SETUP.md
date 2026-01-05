# 🔌 IoT Smart Home Monitor - Complete Wiring & Setup Guide

## 1. HARDWARE CONNECTIONS

### ESP32 Pin Configuration
┌─────────────────────────────────────────────────────────┐
│                       ESP32 DevKit                       │
└─────────────────────────────────────────────────────────┘GPIO 4   → DHT22 Data Pin
GPIO 1   → SH1106 SDA (I2C)
GPIO 2   → SH1106 SCL (I2C)
GPIO 3   → MQ-135 A0 (Analog)
5V       → Power rail
3.3V     → Low power devices
GND      → Common ground
### Detailed Wiring Diagram

#### DHT22 (Temperature/Humidity)DHT22 (3-pin module)
├─ Pin 1 (VCC)    → ESP32 3.3V
├─ Pin 2 (Data)   → ESP32 GPIO 4 (+ 10kΩ pull-up resistor to 3.3V)
└─ Pin 3 (GND)    → ESP32 GND
#### MQ-135 (Gas Sensor)MQ-135 (4-pin module)
├─ VCC          → ESP32 5V (power via DC converter)
├─ GND          → ESP32 GND
├─ A0 (Analog)  → ESP32 GPIO 3
└─ D0 (Digital) → Not used (optional)
#### SH1106 OLED Display (I2C)SH1106 (4-pin I2C module)
├─ GND (Pin 1)  → ESP32 GND
├─ VCC (Pin 2)  → ESP32 3.3V
├─ SCL (Pin 3)  → ESP32 GPIO 2
└─ SDA (Pin 4)  → ESP32 GPIO 1
#### Power Supply (with 12V Battery)12V Li-ion Battery (7Ah)
├─ (+) → DC-DC Converter Input
└─ (-) → GNDDC-DC Converter (12V → 5V/3A)
├─ IN+ (12V)  → Battery (+)
├─ IN- (GND)  → Battery (-)
├─ OUT+ (5V)  → ESP32 5V
└─ OUT- (GND) → ESP32 GND
### Optional: USB Power (for development)Micro USB Cable
└─ → ESP32 USB Port (powers 5V rail directly)
---

## 2. SOFTWARE SETUP

### Step 1: Install Arduino IDE
- Download from: https://www.arduino.cc/en/software
- Install ESP32 board support:
  - Tools → Board Manager
  - Search "ESP32" → Install "esp32 by Espressif Systems"

### Step 2: Install Required Libraries
In Arduino IDE: Sketch → Include Library → Manage Libraries

Search and install:
- DHT sensor library by Adafruit (v1.4.0+)
- Adafruit GFX Library by Adafruit (v1.11.0+)
- Adafruit SH110X by Adafruit (v2.1.0+)
- WebServer (built-in with ESP32)

### Step 3: Board Configuration
Select Tools menu:Board:        ESP32 Dev Module
Port:         /dev/cu.usbserial-* (macOS/Linux) or COM* (Windows)
Upload Speed: 921600
CPU Freq:     80 MHz
### Step 4: Code Upload
1. Copy the full Arduino code from main repository
2. Change WiFi credentials:
   `cpp
   const char* ssid = "YOUR_WIFI_NAME";
   const char* password = "YOUR_WIFI_PASSWORD";
   const char* http_username = "admin";
   const char* http_password = "airquality123";Sketch → Upload3. FIRST BOOT & CALIBRATIONWhat Happens1. Serial monitor shows startup banner
2. 30-second MQ-135 calibration begins
3. OLED display counts down
4. WiFi connection attempt
5. Ready for monitoring!Monitor ProgressOpen Serial Monitor (Ctrl+Shift+M)Set baud rate to 115200Watch for calibration countdownNote the IP address shown (e.g., 192.168.1.100)Expected Output╔════════════════════════════════════════╗
║  Smart Home Monitor v10.0 (SECURE)     ║
║  WiFi + MQ-135 + DHT22 + OLED          ║
║  CO + CO2 + Toluene + NH4 + Acetone    ║
╚════════════════════════════════════════╝

Calibrating MQ-135 (30s)...
Ro = 2843
Ready!

Connecting to WiFi: MyNetwork
WiFi Connected! IP: 192.168.1.100
Secure Web Server Started!
Username: admin | Password: airquality1234. ACCESSING THE WEB DASHBOARDVia BrowserOpen web browserNavigate to: http://192.168.1.100 (replace with your ESP32 IP)Enter credentials when prompted:Username: adminPassword: airquality123Dashboard loads with live data updatesDashboard FeaturesReal-time Readings - Temp, Humidity, Gas levelsAuto-refresh - Updates every 3 secondsDark theme - Easy on eyes, professional lookSecure - HTTP Basic Auth protection5. API ENDPOINTS (for integration)Get Live JSON DataGET http://192.168.1.100/api/json
Auth: admin / airquality123

Response:
{
  "temperature": 22.5,
  "humidity": 45.2,
  "co": 12.3,
  "co2": 415,
  "toluene": 2.1,
  "nh4": 0.8,
  "acetone": 1.5
}Get CSV DataGET http://192.168.1.100/data
Auth: admin / airquality123

Tomasz, [05/01/2026 15:30]
Response:
22.5,45.2,12.3,415,2.1,0.8,1.5Example cURL commandcurl -u admin:airquality123 http://192.168.1.100/api/json6. OLED DISPLAY PAGESThe 1.3" OLED cycles through 4 pages automatically (every 4 seconds):Page 1: Environment┌──────────────────┐
│  Environment     │
│  ==============  │
│  22.5°C          │
│  45%             │
└──────────────────┘Page 2: Major Gases┌──────────────────┐
│  Major Gases     │
│  ==============  │
│  CO: 12.3 ppm    │
│  CO2: 415 ppm    │
└──────────────────┘Page 3: Other Gases┌──────────────────┐
│  Other Gases     │
│  ==============  │
│  Tol: 2.1 ppm    │
│  NH4: 0.8 ppm    │
│  Ace: 1.5 ppm    │
└──────────────────┘Page 4: Status┌──────────────────┐
│  Status          │
│  ==============  │
│  WiFi: OK        │
│  IP: 192.168.1.1 │
└──────────────────┘7. TROUBLESHOOTING"Compilation Error: DHT.h not found"Solution: Install DHT library via Library Manager"OLED NOT FOUND" messageSolution:Check I2C connections (GPIO 1 & 2)Verify I2C address is 0x3CTry I2C scanner sketch to detect device"WiFi fails to connect"Solution:Check WiFi credentials (exactly match your network)Restart ESP32Check WiFi is 2.4GHz (not 5GHz)Verify router doesn't have MAC filtering enabled"Dashboard won't load (401 Unauthorized)"Solution:Ensure login is correct: admin / airquality123Try clearing browser cache (Ctrl+Shift+Delete)Try incognito window"IP address changes every boot"Solution: Set static IP in code:WiFi.config(
  IPAddress(192, 168, 1, 100),  // Static IP
  IPAddress(192, 168, 1, 1),    // Gateway
  IPAddress(255, 255, 255, 0)   // Subnet
);8. CHANGING LOGIN CREDENTIALSEdit lines 20-21 in the Arduino code:const char* http_username = "myuser";        // Change this
const char* http_password = "mysecretpass";  // Change thisThen re-upload to ESP32.9. FOLDER STRUCTURE (for GitHub)iot-smart-home-monitor/
├── README.md                 (main overview)
├── docs/
│   ├── SETUP.md             (this file)
│   ├── WIRING.md            (hardware connections)
│   ├── API.md               (REST API documentation)
│   └── TROUBLESHOOTING.md   (FAQ & fixes)
├── arduino/
│   ├── SmartHomeMonitor.ino (main code)
│   └── libraries/           (dependencies list)
├── web/
│   └── dashboard.html       (web interface)
└── images/
    ├── wiring-diagram.jpg
    ├── dashboard-screenshot.jpg
    └── hardware-setup.jpg10. POWER CONSUMPTIONWith 12V 7Ah Battery:Current draw (typical):  ~300mA @ 12V
Daily usage (24h):       7.2 Ah
Battery life:            ~24 days on single charge

With 2 batteries parallel:
Battery life:            ~48 days (2 months!)Quick Reference Checklist□ ESP32 connected to USB for initial programming
□ All sensor pins matched to GPIO definitions
□ DHT22 has 10kΩ pull-up resistor
□ I2C OLED address verified (0x3C)
□ WiFi credentials entered correctly
□ Code compiled and uploaded successfully
□ Serial monitor shows IP address
□ Dashboard accessible via browser
□ Login credentials working (admin / airquality123)
□ Battery voltage checked if using 12V supplyReady to monitor your home's air quality! 🏠💨