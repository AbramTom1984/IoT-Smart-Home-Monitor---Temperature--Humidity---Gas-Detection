#include "DHT.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WiFi.h>
#include <WebServer.h>

// ===== PIN DEFINITIONS =====
#define DHTPIN 4
#define DHTTYPE DHT22
#define I2C_SDA 1
#define I2C_SCL 2
#define MQ135_PIN 3

// ===== OLED =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
DHT dht(DHTPIN, DHTTYPE);

// ===== WiFi =====
const char* ssid = "YOUR_WIFI_SSID";           // CHANGE THIS!
const char* password = "YOUR_WIFI_PASSWORD";   // CHANGE THIS!
WebServer server(80);

// ===== HTTP AUTHENTICATION =====
const char* http_username = "admin";
const char* http_password = "airquality123";   // CHANGE THIS!

// ===== MQ-135 PARAMETERS =====
#define RL_VALUE 10000.0
#define RO_CLEAN_AIR_FACTOR 3.6

float Ro = 10000.0;
bool calibrated = false;

// ===== GAS VARIABLES =====
float co = 0, co2 = 0;
float toluene = 0, nh4 = 0, acetone = 0;
float temperature = 0, humidity = 0;

// ===== PAGE DISPLAY =====
int currentPage = 0;
unsigned long lastPageChange = 0;
const unsigned long PAGE_INTERVAL = 4000;

// ===== MOVING AVERAGE BUFFER =====
const int BUFFER_SIZE = 20;
float coBuffer[BUFFER_SIZE];
float co2Buffer[BUFFER_SIZE];
int bufferIdx = 0;

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("");
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║  Smart Home Monitor v10.0 (SECURE)     ║");
  Serial.println("║  WiFi + MQ-135 + DHT22 + OLED          ║");
  Serial.println("║  CO + CO2 + Toluene + NH4 + Acetone    ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println("");

  pinMode(MQ135_PIN, INPUT);
  dht.begin();
  Wire.begin(I2C_SDA, I2C_SCL);

  if (!display.begin(OLED_ADDR, true)) {
    Serial.println("ERROR: OLED NOT FOUND!");
    while(1) { delay(1000); }
  }

  Serial.println("OLED initialized!");

  // Splash screen
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("Smart Home Monitor");
  display.println("v10.0 SECURE");
  display.println("");
  display.println("Initializing WiFi...");
  display.display();

  // WiFi Setup
  setupWiFi();

  // Show calibration screen
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("Calibrating MQ-135...");
  display.println("Wait 30 seconds...");
  display.display();

  Serial.println("Calibrating MQ-135 (30s)...");

  // 30 seconds calibration
  for (int i = 30; i > 0; i--) {
    int raw = analogRead(MQ135_PIN);
    float voltage = (raw / 4095.0) * 5.0;
    float Rs = calculateRs(voltage);
    Ro += Rs;

    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(25, 20);
    display.println("CALIB");
    display.setCursor(40, 45);
    display.setTextSize(3);
    display.println(i);
    display.display();

    delay(1000);
  }

  Ro = Ro / 30.0;
  Ro = Ro / RO_CLEAN_AIR_FACTOR;
  calibrated = true;

  Serial.print("Ro = ");
  Serial.println(Ro, 0);
  Serial.println("Ready!");
  Serial.println("");

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Ready!");
  if (WiFi.status() == WL_CONNECTED) {
    display.println("WiFi: OK");
  } else {
    display.println("WiFi: FAILED");
  }
  display.display();
  delay(2000);

  // Clear buffers
  for (int i = 0; i < BUFFER_SIZE; i++) {
    coBuffer[i] = 0;
    co2Buffer[i] = 0;
  }
}

// ===== MAIN LOOP =====
void loop() {
  if (!calibrated) return;

  // WiFi handler
  server.handleClient();

  delay(500);

  // === DHT22 ===
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("DHT22 error");
    return;
  }

Tomasz, [05/01/2026 15:13]
// === MQ-135 - average of 5 measurements ===
  float avgVoltage = 0;
  for (int i = 0; i < 5; i++) {
    int raw = analogRead(MQ135_PIN);
    avgVoltage += (raw / 4095.0) * 5.0;
    delayMicroseconds(100);
  }
  avgVoltage /= 5.0;

  float Rs = calculateRs(avgVoltage);
  float ratio = Rs / Ro;

  // Protection against crazy values
  if (ratio > 50) ratio = 50;
  if (ratio < 0.01) ratio = 0.01;

  // === Gas calculations ===
  float newCO = 605.18 * pow(ratio, -3.937);
  if (newCO > 1000) newCO = 1000;
  if (newCO < 0) newCO = 0;

  // CO2 WITHOUT +400 OFFSET (from zero)
  float newCO2 = 110.47 * pow(ratio, -2.862);
  if (newCO2 > 1600) newCO2 = 1600;
  if (newCO2 < 0) newCO2 = 0;

  // === Other gases ===
  toluene = 44.947 * pow(ratio, -3.445);
  if (toluene < 0) toluene = 0;
  if (toluene > 300) toluene = 300;

  nh4 = 102.2 * pow(ratio, -2.473);
  if (nh4 < 0) nh4 = 0;
  if (nh4 > 300) nh4 = 300;

  acetone = 34.668 * pow(ratio, -3.369);
  if (acetone < 0) acetone = 0;
  if (acetone > 400) acetone = 400;

  // === Moving average filtering ===
  coBuffer[bufferIdx] = newCO;
  co2Buffer[bufferIdx] = newCO2;

  bufferIdx = (bufferIdx + 1) % BUFFER_SIZE;

  // Average from buffer
  co = getAverage(coBuffer, BUFFER_SIZE);
  co2 = getAverage(co2Buffer, BUFFER_SIZE);

  // === Serial debug ===
  Serial.print("T:");
  Serial.print(temperature, 1);
  Serial.print("C | H:");
  Serial.print(humidity, 1);
  Serial.print("% | Rs:");
  Serial.print(Rs, 0);
  Serial.print(" | Ratio:");
  Serial.print(ratio, 2);
  Serial.print(" | CO:");
  Serial.print(co, 1);
  Serial.print(" | CO2:");
  Serial.print(co2, 0);
  Serial.print(" | Tol:");
  Serial.print(toluene, 1);
  Serial.print(" | NH4:");
  Serial.print(nh4, 1);
  Serial.print(" | Ace:");
  Serial.println(acetone, 1);

  // === Page rotation ===
  unsigned long currentTime = millis();
  if (currentTime - lastPageChange > PAGE_INTERVAL) {
    currentPage = (currentPage + 1) % 4;
    lastPageChange = currentTime;
  }

  // Display
  if (currentPage == 0) {
    displayPageTemp(temperature, humidity);
  } else if (currentPage == 1) {
    displayPageMajorGases(co, co2);
  } else if (currentPage == 2) {
    displayPageOtherGases(toluene, nh4, acetone);
  } else {
    displayPageStatus();
  }
}

// ===== HELPER FUNCTIONS =====

float calculateRs(float voltage) {
  if (voltage >= 4.9) voltage = 4.9;
  if (voltage <= 0.1) voltage = 0.1;
  float Rs = RL_VALUE * (5.0 - voltage) / voltage;
  return Rs;
}

float getAverage(float* buffer, int size) {
  float sum = 0;
  for (int i = 0; i < size; i++) {
    sum += buffer[i];
  }
  return sum / size;
}

// ===== WiFi SETUP =====
void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 15) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("WiFi Connected! IP: ");
    Serial.println(WiFi.localIP());
    
    // Setup web server
    server.on("/", handleRoot);
    server.on("/data", handleData);
    server.on("/api/json", handleJson);
    server.begin();
    Serial.println("Secure Web Server Started!");
    Serial.print("Username: ");
    Serial.print(http_username);
    Serial.print(" | Password: ");
    Serial.println(http_password);
  } else {
    Serial.println("WiFi failed - OLED display only");
  }
}

// ===== WEB HANDLERS WITH AUTHENTICATION =====
void handleRoot() {
  // AUTHENTICATION CHECK
  if (!server.authenticate(http_username, http_password)) {
    return server.requestAuthentication();
  }

  String html = R"(
<!DOCTYPE html>
<html>
<head>
  <title>Air Quality Monitor</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">

Tomasz, [05/01/2026 15:13]
<style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: Arial, sans-serif;
      background: #0a0e27;
      color: #fff;
      padding: 20px;
    }
    h1 { text-align: center; margin-bottom: 30px; color: #4ade80; }
    .grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
      gap: 15px;
    }
    .card {
      background: #1a1f3a;
      border: 1px solid #333;
      padding: 20px;
      border-radius: 10px;
      text-align: center;
    }
    .label { font-size: 12px; color: #888; margin-bottom: 10px; }
    .value {
      font-size: 32px;
      font-weight: bold;
      color: #4ade80;
    }
    .unit { font-size: 12px; color: #888; }
    .status {
      text-align: center;
      margin-top: 20px;
      padding: 10px;
      background: #1a1f3a;
      border-radius: 5px;
      font-size: 12px;
      color: #888;
    }
    .security {
      text-align: center;
      margin-top: 20px;
      padding: 10px;
      background: #1a3a2a;
      border-radius: 5px;
      font-size: 12px;
      color: #4ade80;
      border: 1px solid #4ade80;
    }
  </style>
</head>
<body>
  <h1>🏠 Air Quality Monitor</h1>
  <div class="grid">
    <div class="card">
      <div class="label">Temperature</div>
      <div class="value" id="temp">--</div>
      <div class="unit">°C</div>
    </div>
    <div class="card">
      <div class="label">Humidity</div>
      <div class="value" id="hum">--</div>
      <div class="unit">%</div>
    </div>
    <div class="card">
      <div class="label">CO</div>
      <div class="value" id="co">--</div>
      <div class="unit">ppm</div>
    </div>
    <div class="card">
      <div class="label">CO2</div>
      <div class="value" id="co2">--</div>
      <div class="unit">ppm</div>
    </div>
    <div class="card">
      <div class="label">Toluene</div>
      <div class="value" id="tol">--</div>
      <div class="unit">ppm</div>
    </div>
    <div class="card">
      <div class="label">NH4</div>
      <div class="value" id="nh4">--</div>
      <div class="unit">ppm</div>
    </div>
    <div class="card">
      <div class="label">Acetone</div>
      <div class="value" id="ace">--</div>
      <div class="unit">ppm</div>
    </div>
  </div>
  <div class="status">
    <p>Updating every 3 seconds...</p>
  </div>
  <div class="security">
    <p>🔐 Secure Connection - HTTP Basic Auth Enabled</p>
  </div>
  <script>
    setInterval(updateData, 3000);
    updateData();
    function updateData() {
      fetch('/api/json')
        .then(r => r.json())
        .then(data => {
          document.getElementById('temp').innerText = data.temperature.toFixed(1);
          document.getElementById('hum').innerText = data.humidity.toFixed(1);
          document.getElementById('co').innerText = data.co.toFixed(1);
          document.getElementById('co2').innerText = Math.round(data.co2);
          document.getElementById('tol').innerText = data.toluene.toFixed(1);
          document.getElementById('nh4').innerText = data.nh4.toFixed(1);
          document.getElementById('ace').innerText = data.acetone.toFixed(1);
        })
        .catch(e => console.log('Error:', e));
    }
  </script>
</body>
</html>
)";
  server.send(200, "text/html", html);
}

void handleData() {
  // AUTHENTICATION CHECK
  if (!server.authenticate(http_username, http_password)) {
    return server.requestAuthentication();
  }

  String data = String(temperature, 1) + "," +
                String(humidity, 1) + "," +
                String(co, 1) + "," +
                String(co2, 0) + "," +
                String(toluene, 1) + "," +
                String(nh4, 1) + "," +
                String(acetone, 1);
  server.send(200, "text/plain", data);
}

void handleJson() {
  // AUTHENTICATION CHECK
  if (!server.authenticate(http_username, http_password)) {
    return server.requestAuthentication();
  }

Tomasz, [05/01/2026 15:13]
String json = "{";
  json += ""temperature":" + String(temperature, 2) + ",";
  json += ""humidity":" + String(humidity, 2) + ",";
  json += ""co":" + String(co, 2) + ",";
  json += ""co2":" + String(co2, 0) + ",";
  json += ""toluene":" + String(toluene, 2) + ",";
  json += ""nh4":" + String(nh4, 2) + ",";
  json += ""acetone":" + String(acetone, 2);
  json += "}";
  server.send(200, "application/json", json);
}

// ===== DISPLAY PAGES =====

void displayPageTemp(float temp, float humidity) {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Environment");
  display.println("====================");

  display.setTextSize(3);
  display.setCursor(5, 20);
  display.print(temp, 1);
  display.setTextSize(1);
  display.setCursor(100, 25);
  display.println("C");

  display.setTextSize(3);
  display.setCursor(5, 45);
  display.print(humidity, 0);
  display.setTextSize(1);
  display.setCursor(100, 50);
  display.println("%");

  display.display();
}

void displayPageMajorGases(float co, float co2) {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Major Gases");
  display.println("====================");

  display.setTextSize(2);
  display.setCursor(0, 18);
  display.print("CO:");
  display.println(co, 1);

  display.setTextSize(2);
  display.setCursor(0, 38);
  display.print("CO2:");
  display.println(co2, 0);

  display.display();
}

void displayPageOtherGases(float toluene, float nh4, float acetone) {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Other Gases");
  display.println("====================");

  display.setTextSize(1);
  display.setCursor(0, 18);
  display.print("Tol:");
  display.println(toluene, 1);

  display.setTextSize(1);
  display.setCursor(0, 30);
  display.print("NH4:");
  display.println(nh4, 1);

  display.setTextSize(1);
  display.setCursor(0, 42);
  display.print("Ace:");
  display.println(acetone, 1);

  display.display();
}

void displayPageStatus() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Status");
  display.println("====================");

  display.setTextSize(1);
  display.setCursor(0, 18);
  
  if (WiFi.status() == WL_CONNECTED) {
    display.println("WiFi: OK");
    display.println("IP:");
    display.setTextSize(1);
    display.setCursor(0, 32);
    display.println(WiFi.localIP().toString().c_str());
  } else {
    display.println("WiFi: OFFLINE");
    display.println("(OLED mode)");
  }

  display.display();
}