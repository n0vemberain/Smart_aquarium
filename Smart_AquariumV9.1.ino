// Blynk Includes
#define BLYNK_TEMPLATE_ID "TMPL60viQI5st"
#define BLYNK_TEMPLATE_NAME "SmartAquarium"
#define BLYNK_AUTH_TOKEN "OB1E9ne28nzrjKS_XvwWp8hKf5JG3b8q"
#include <BlynkSimpleEsp32.h>

BlynkTimer timer;  // Timer for updating sensors

#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <time.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ==== Wi-Fi Credentials ====
const char* ssid = "Bolbooo naaa";
const char* password = "123456789md";

// ==== Web Server ====
WebServer server(80);

// ==== LCD Setup ====
LiquidCrystal_I2C lcd(0x27, 16, 2);  // I2C address 0x27, 16 columns, 2 rows

// ==== Servo Setup ====
Servo myServo;
const int servoPin = 13;

// ==== Ultrasonic Sensor Pins ====
const int trigPin = 5;
const int echoPin = 18;

// ==== DHT11 Sensor Setup ====
#define DHT_PIN 15

// ==== DS18B20 Water Temperature Sensor Setup ====
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature waterTempSensor(&oneWire);

// ==== Water Pump Pin ====
const int pumpPin = 14;  // Relay pin for water pump
bool pumpStatus = false;

// ==== Feeding Time Variables ====
int feedHour = -1;
int feedMinute = -1;
bool hasFedToday = false;

// ==== Time Setup ====
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 6 * 3600;
const int daylightOffset_sec = 0;

// ==== LCD Display Variables ====
unsigned long lastLcdUpdate = 0;
int lcdPage = 0;  // 0 = Food/Temp, 1 = pH/TDS/Pump, 2 = Water Temperature

// ==== Notification Cooldown Variables ====
unsigned long lastLowFoodNotification = 0;
unsigned long lastAirTempNotification = 0;
unsigned long lastHumidityNotification = 0;
unsigned long lastWaterTempNotification = 0;
unsigned long lastPhNotification = 0;
unsigned long lastTdsNotification = 0;
const unsigned long notificationCooldown = 600000;  // 10 minutes

// ==== DHT11 Functions (Library-Free) ====
struct DHTData {
  float temperature;
  float humidity;
  bool valid;
};

DHTData readDHT11() {
  DHTData result = { -999, -999, false };
  uint8_t data[5] = { 0, 0, 0, 0, 0 };
  uint8_t laststate = HIGH;
  uint8_t counter = 0;
  uint8_t j = 0, i;

  digitalWrite(DHT_PIN, HIGH);
  delay(250);
  pinMode(DHT_PIN, OUTPUT);
  digitalWrite(DHT_PIN, LOW);
  delay(20);
  digitalWrite(DHT_PIN, HIGH);
  delayMicroseconds(40);
  pinMode(DHT_PIN, INPUT);

  for (i = 0; i < 85; i++) {
    counter = 0;
    while (digitalRead(DHT_PIN) == laststate) {
      counter++;
      delayMicroseconds(1);
      if (counter == 255) break;
    }
    laststate = digitalRead(DHT_PIN);
    if (counter == 255) break;
    if ((i >= 4) && (i % 2 == 0)) {
      data[j / 8] <<= 1;
      if (counter > 16) data[j / 8] |= 1;
      j++;
    }
  }

  if ((j >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
    result.humidity = data[0];
    result.temperature = data[2];
    result.valid = true;
  }
  return result;
}

float readTemperature() {
  DHTData dhtData = readDHT11();
  if (!dhtData.valid) {
    Serial.println("Failed to read temperature from DHT sensor!");
    return -999;
  }
  return dhtData.temperature;
}

float readHumidity() {
  DHTData dhtData = readDHT11();
  if (!dhtData.valid) {
    Serial.println("Failed to read humidity from DHT sensor!");
    return -999;
  }
  return dhtData.humidity;
}

String getTemperatureColor(float temp) {
  if (temp == -999) return "#888888";
  if (temp > 30) return "#FF4444";
  if (temp > 25) return "#FFB84D";
  return "#4CAF50";
}

// ==== DS18B20 Water Temperature Functions ====
float readWaterTemperature() {
  waterTempSensor.requestTemperatures();
  float temp = waterTempSensor.getTempCByIndex(0);
  if (temp == DEVICE_DISCONNECTED_C) {
    Serial.println("Failed to read water temperature from DS18B20 sensor!");
    return -999;
  }
  return temp;
}

String getWaterTemperatureColor(float temp) {
  if (temp == -999) return "#888888";
  if (temp < 20) return "#FF4444";
  if (temp >= 20 && temp <= 23) return "#FFB84D";
  if (temp > 23 && temp < 28) return "#4CAF50";
  if (temp >= 28 && temp <= 32) return "#FFB84D";
  if (temp > 32) return "#FF4444";
  return "#4CAF50";
}

String getWaterTemperatureStatus(float temp) {
  if (temp == -999) return "Sensor Error";
  if (temp < 20) return "Too Cold";
  if (temp >= 20 && temp <= 23) return "Slightly Cool";
  if (temp > 23 && temp < 28) return "Optimal";
  if (temp >= 28 && temp <= 32) return "Slightly Warm";
  if (temp > 32) return "Too Hot";
  return "Normal";
}

// ==== Simulated Sensor Functions ====
float readPH() {
  static float basePH = 7.2;
  basePH += (random(-10, 11) / 100.0);
  if (basePH < 6.5) basePH = 6.5;
  if (basePH > 8.5) basePH = 8.5;
  return basePH;
}

float readTDS() {
  static float baseTDS = 220;
  baseTDS += random(-20, 21);
  if (baseTDS < 150) baseTDS = 150;
  if (baseTDS > 300) baseTDS = 300;
  return baseTDS;
}

String getPHColor(float ph) {
  if (ph < 6.8 || ph > 8.2) return "#FF4444";
  if (ph < 7.0 || ph > 8.0) return "#FFB84D";
  return "#4CAF50";
}

String getTDSColor(float tds) {
  if (tds < 180 || tds > 280) return "#FF4444";
  if (tds < 200 || tds > 260) return "#FFB84D";
  return "#4CAF50";
}

// ==== Ultrasonic Functions ====
float measureDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  float distance = duration * 0.034 / 2;
  return distance;
}

String getFoodStatus(float distance) {
  if (distance > 5) return "Food Is Finished";
  else if (distance > 4) return "Food Is Almost Finished";
  else return "There is enough food";
}

String getStatusColor(float distance) {
  if (distance > 5) return "#FF4444";
  else if (distance > 4) return "#FFB84D";
  else return "#4CAF50";
}

// ==== Pump Control Functions ====
void turnPumpOn() {
  digitalWrite(pumpPin, HIGH);
  pumpStatus = true;
  Serial.println("Water pump turned ON");
  Blynk.virtualWrite(V6, 1);
}

void turnPumpOff() {
  digitalWrite(pumpPin, LOW);
  pumpStatus = false;
  Serial.println("Water pump turned OFF");
  Blynk.virtualWrite(V6, 0);
}

// ==== LCD Display Functions ====
void updateLCD() {
  if (millis() - lastLcdUpdate < 3000) return;  // Update every 3 seconds

  lcd.clear();

  if (lcdPage == 0) {
    // Page 1: Food Level & Air Temperature
    float distance = measureDistance();
    lcd.setCursor(0, 0);
    lcd.print("Food: ");
    lcd.print(distance, 1);
    lcd.print("cm");

    float temp = readTemperature();
    lcd.setCursor(0, 1);
    if (temp != -999) {
      lcd.print("Air: ");
      lcd.print(temp, 1);
      lcd.print("C");
    } else {
      lcd.print("Air Temp: Error");
    }
  } else if (lcdPage == 1) {
    // Page 2: pH, TDS, Pump Status
    float ph = readPH();
    lcd.setCursor(0, 0);
    lcd.print("pH: ");
    lcd.print(ph, 1);
    lcd.print(" TDS:");
    lcd.print(readTDS(), 0);

    lcd.setCursor(0, 1);
    lcd.print("Pump: ");
    lcd.print(pumpStatus ? "ON " : "OFF");
  } else {
    // Page 3: Water Temperature
    float waterTemp = readWaterTemperature();
    lcd.setCursor(0, 0);
    if (waterTemp != -999) {
      lcd.print("Water: ");
      lcd.print(waterTemp, 1);
      lcd.print("C");
    } else {
      lcd.print("Water: Error");
    }

    lcd.setCursor(0, 1);
    String status = getWaterTemperatureStatus(waterTemp);
    lcd.print("Status: ");
    lcd.print(status.substring(0, 8));
  }

  lcdPage = (lcdPage + 1) % 3;
  lastLcdUpdate = millis();
}

// ==== Servo Feeding Action ====
void rotateServo() {
  float distance = measureDistance();
  if (distance > 4.3) {
    Serial.println("Cannot feed - Food is finished!");
    if (millis() - lastLowFoodNotification > notificationCooldown) {
      Blynk.logEvent("LOW_FOOD", "Cannot feed - Food container is empty!");
      lastLowFoodNotification = millis();
    }
    return;
  }
  Serial.println("Feeding Now...");
  myServo.write(0);
  delay(500);
  myServo.write(90);
  delay(200);
  myServo.write(0);
  delay(500);
  Serial.println("Feeding Done.");
  Blynk.logEvent("FEED_SUCCESS", "Fish fed successfully!");
}

// ==== Blynk Control Handlers ====
BLYNK_WRITE(V6) {
  int value = param.asInt();
  if (value == 1) turnPumpOn();
  else turnPumpOff();
}

BLYNK_WRITE(V7) {
  if (param.asInt() == 1) rotateServo();
}

BLYNK_WRITE(V8) {
  feedHour = param.asInt();
  hasFedToday = false;
}

BLYNK_WRITE(V9) {
  feedMinute = param.asInt();
  hasFedToday = false;
}

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V6, V8, V9);
}

void updateBlynkSensors() {
  float airTemp = readTemperature();
  float humidity = readHumidity();
  float waterTemp = readWaterTemperature();
  float ph = readPH();
  float tds = readTDS();
  float distance = measureDistance();

  Blynk.virtualWrite(V0, airTemp);
  Blynk.virtualWrite(V1, humidity);
  Blynk.virtualWrite(V2, waterTemp);
  Blynk.virtualWrite(V3, ph);
  Blynk.virtualWrite(V4, tds);
  Blynk.virtualWrite(V5, distance);
  Blynk.virtualWrite(V6, pumpStatus ? 1 : 0);

  // Food Level Notification
  if (distance > 5 && millis() - lastLowFoodNotification > notificationCooldown) {
    Blynk.logEvent("LOW_FOOD", "Food container is empty! Please refill. Distance: " + String(distance, 1) + "cm");
    lastLowFoodNotification = millis();
  }

  // Air Temperature Notification
  if ((airTemp < 15 || airTemp > 30) && airTemp != -999 && millis() - lastAirTempNotification > notificationCooldown) {
    Blynk.logEvent("AIR_TEMP_ALERT", "Air temperature out of range: " + String(airTemp, 1) + "°C");
    lastAirTempNotification = millis();
  }

  // Humidity Notification
  if ((humidity < 30 || humidity > 80) && humidity != -999 && millis() - lastHumidityNotification > notificationCooldown) {
    Blynk.logEvent("HUMIDITY_ALERT", "Humidity out of range: " + String(humidity, 1) + "%");
    lastHumidityNotification = millis();
  }

  // Water Temperature Notification
  if ((waterTemp < 20 || waterTemp > 32) && waterTemp != -999 && millis() - lastWaterTempNotification > notificationCooldown) {
    Blynk.logEvent("WATER_TEMP_ALERT", "Water temperature out of range: " + String(waterTemp, 1) + "°C");
    lastWaterTempNotification = millis();
  }

  // pH Notification
  if ((ph < 6.8 || ph > 8.2) && millis() - lastPhNotification > notificationCooldown) {
    Blynk.logEvent("PH_ALERT", "pH out of range: " + String(ph, 1));
    lastPhNotification = millis();
  }

  // TDS Notification
  if ((tds < 180 || tds > 280) && millis() - lastTdsNotification > notificationCooldown) {
    Blynk.logEvent("TDS_ALERT", "TDS out of range: " + String(tds, 0) + " ppm");
    lastTdsNotification = millis();
  }
}

// ==== HTTP Handlers ====
void handleRoot() {
  server.send(200, "text/html", getHTMLPage());
}

void handleFeedNow() {
  float distance = measureDistance();
  if (distance > 5) {
    server.send(200, "text/plain", "NO_FOOD");
  } else {
    rotateServo();
    server.send(200, "text/plain", "Fed!");
  }
}

void handleSetTime() {
  if (server.hasArg("hour") && server.hasArg("minute")) {
    feedHour = server.arg("hour").toInt();
    feedMinute = server.arg("minute").toInt();
    hasFedToday = false;
    Serial.printf("Feeding time set: %02d:%02d\n", feedHour, feedMinute);
    server.send(200, "text/plain", "Feeding time set!");
    Blynk.virtualWrite(V8, feedHour);
    Blynk.virtualWrite(V9, feedMinute);
  } else {
    server.send(400, "text/plain", "Missing time values.");
  }
}

void handlePumpControl() {
  if (server.hasArg("action")) {
    String action = server.arg("action");
    if (action == "on") {
      turnPumpOn();
      server.send(200, "text/plain", "Pump turned ON");
    } else if (action == "off") {
      turnPumpOff();
      server.send(200, "text/plain", "Pump turned OFF");
    } else {
      server.send(400, "text/plain", "Invalid action");
    }
  } else {
    server.send(400, "text/plain", "Missing action parameter");
  }
}

void handleEnvironment() {
  float temperature = readTemperature();
  float humidity = readHumidity();
  String tempColor = getTemperatureColor(temperature);
  String json = "{\"temperature\":" + String(temperature, 1) + ",\"humidity\":" + String(humidity, 1) + ",\"tempColor\":\"" + tempColor + "\"}";
  server.send(200, "application/json", json);
}

void handleWaterTemperature() {
  float waterTemp = readWaterTemperature();
  String waterTempColor = getWaterTemperatureColor(waterTemp);
  String waterTempStatus = getWaterTemperatureStatus(waterTemp);
  String json = "{\"waterTemp\":" + String(waterTemp, 1) + ",\"waterTempColor\":\"" + waterTempColor + "\",\"waterTempStatus\":\"" + waterTempStatus + "\"}";
  server.send(200, "application/json", json);
}

void handleWaterQuality() {
  float ph = readPH();
  float tds = readTDS();
  String phColor = getPHColor(ph);
  String tdsColor = getTDSColor(tds);
  String json = "{\"ph\":" + String(ph, 1) + ",\"tds\":" + String(tds, 0) + ",\"phColor\":\"" + phColor + "\",\"tdsColor\":\"" + tdsColor + "\",\"pumpStatus\":" + String(pumpStatus ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

void handleFoodLevel() {
  float distance = measureDistance();
  String status = getFoodStatus(distance);
  String color = getStatusColor(distance);
  String json = "{\"distance\":" + String(distance, 1) + ",\"status\":\"" + status + "\",\"color\":\"" + color + "\"}";
  server.send(200, "application/json", json);
}

// ==== HTML PAGE FUNCTION ====
String getHTMLPage() {
  float distance = measureDistance();
  String status = getFoodStatus(distance);
  String color = getStatusColor(distance);
  float temperature = readTemperature();
  float humidity = readHumidity();
  String tempColor = getTemperatureColor(temperature);
  float waterTemp = readWaterTemperature();
  String waterTempColor = getWaterTemperatureColor(waterTemp);
  String waterTempStatus = getWaterTemperatureStatus(waterTemp);
  float ph = readPH();
  float tds = readTDS();
  String phColor = getPHColor(ph);
  String tdsColor = getTDSColor(tds);

  return R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>🐠 Smart Aquarium Dashboard</title>
  <script src="https://cdn.tailwindcss.com"></script>
  <style>
    .gradient-bg {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    }
    .glass-effect {
      background: rgba(255, 255, 255, 0.95);
      backdrop-filter: blur(10px);
      border: 1px solid rgba(255,255,255,0.2);
    }
    .pulse-animation {
      animation: pulse 2s infinite;
    }
    @keyframes pulse {
      0% { transform: scale(1); }
      50% { transform: scale(1.05); }
      100% { transform: scale(1); }
    }
    .notification {
      position: fixed;
      top: 20px;
      right: 20px;
      padding: 16px 20px;
      border-radius: 12px;
      color: white;
      font-weight: 600;
      display: none;
      z-index: 1000;
      box-shadow: 0 10px 30px rgba(0,0,0,0.2);
      backdrop-filter: blur(10px);
    }
    .notification.success { 
      background: linear-gradient(45deg, #10b981, #059669);
    }
    .notification.error { 
      background: linear-gradient(45deg, #ef4444, #dc2626);
    }
    .notification.info { 
      background: linear-gradient(45deg, #3b82f6, #2563eb);
    }
  </style>
</head>
<body class="gradient-bg min-h-screen text-gray-800">
  <div class="container mx-auto px-4 py-8 max-w-7xl">
    <div class="text-center mb-10">
      <h1 class="text-5xl font-bold text-white mb-4 drop-shadow-lg">🐠 Smart Aquarium Dashboard</h1>
      <p class="text-xl text-white/90">Automated Fish Feeding & Complete Water Management System</p>
    </div>
    <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-6">
      <div class="glass-effect rounded-2xl p-6 shadow-2xl hover:transform hover:scale-105 transition-all duration-300">
        <div class="flex items-center mb-4">
          <span class="text-2xl mr-3">📊</span>
          <h3 class="text-xl font-semibold text-gray-700">Food Level Monitor</h3>
        </div>
        <div class="text-center">
          <div id="distance-circle" class="w-24 h-24 rounded-full mx-auto mb-4 flex items-center justify-center text-white font-bold shadow-lg transition-all duration-300">
            <div class="text-center">
              <div id="distance-display">-- cm</div>
              <div class="text-xs opacity-90">Distance</div>
            </div>
          </div>
          <div id="food-status" class="px-4 py-2 rounded-lg text-white font-semibold mb-3 shadow-md">
            Loading...
          </div>
          <div class="text-gray-500 text-sm">🔄 Updates every 5 seconds</div>
        </div>
      </div>
      <div class="glass-effect rounded-2xl p-6 shadow-2xl hover:transform hover:scale-105 transition-all duration-300">
        <div class="flex items-center mb-4">
          <span class="text-2xl mr-3">🌡️</span>
          <h3 class="text-xl font-semibold text-gray-700">Air Environment</h3>
        </div>
        <div class="grid grid-cols-1 gap-4">
          <div id="temp-display" class="p-4 rounded-lg text-white text-center shadow-md">
            <div id="temp-value" class="text-2xl font-bold mb-1">--°C</div>
            <div class="text-sm opacity-90">Air Temperature</div>
          </div>
          <div class="p-4 rounded-lg text-white text-center shadow-md bg-blue-500">
            <div id="humidity-value" class="text-2xl font-bold mb-1">--%</div>
            <div class="text-sm opacity-90">Humidity</div>
          </div>
        </div>
        <div class="text-gray-500 text-sm mt-3 text-center">🔄 Updates every 5 seconds</div>
      </div>
      <div class="glass-effect rounded-2xl p-6 shadow-2xl hover:transform hover:scale-105 transition-all duration-300">
        <div class="flex items-center mb-4">
          <span class="text-2xl mr-3">🌊</span>
          <h3 class="text-xl font-semibold text-gray-700">Water Temperature</h3>
        </div>
        <div class="text-center">
          <div id="water-temp-circle" class="w-24 h-24 rounded-full mx-auto mb-4 flex items-center justify-center text-white font-bold shadow-lg transition-all duration-300">
            <div class="text-center">
              <div id="water-temp-display">--°C</div>
              <div class="text-xs opacity-90">Water</div>
            </div>
          </div>
          <div id="water-temp-status" class="px-4 py-2 rounded-lg text-white font-semibold mb-3 shadow-md">
            Loading...
          </div>
          <div class="text-gray-500 text-sm">🎯 Optimal: 24-27°C</div>
          <div class="text-gray-500 text-sm">🔄 Updates every 5 seconds</div>
        </div>
      </div>
      <div class="glass-effect rounded-2xl p-6 shadow-2xl hover:transform hover:scale-105 transition-all duration-300">
        <div class="flex items-center mb-4">
          <span class="text-2xl mr-3">💧</span>
          <h3 class="text-xl font-semibold text-gray-700">Water Quality</h3>
        </div>
        <div class="grid grid-cols-1 gap-4">
          <div id="ph-display" class="p-4 rounded-lg text-white text-center shadow-md">
            <div id="ph-value" class="text-2xl font-bold mb-1">--</div>
            <div class="text-sm opacity-90">pH Level</div>
          </div>
          <div id="tds-display" class="p-4 rounded-lg text-white text-center shadow-md">
            <div id="tds-value" class="text-2xl font-bold mb-1">-- ppm</div>
            <div class="text-sm opacity-90">TDS Level</div>
          </div>
        </div>
        <div class="text-gray-500 text-sm mt-3 text-center">🔄 Updates every 5 seconds</div>
      </div>
      <div class="glass-effect rounded-2xl p-6 shadow-2xl hover:transform hover:scale-105 transition-all duration-300">
        <div class="flex items-center mb-4">
          <span class="text-2xl mr-3">⚡</span>
          <h3 class="text-xl font-semibold text-gray-700">Water Pump</h3>
        </div>
        <div class="text-center">
          <div class="mb-4">
            <div id="pump-status" class="inline-block px-4 py-2 rounded-full text-white font-bold text-lg shadow-md">
              OFF
            </div>
          </div>
          <div class="space-y-3">
            <button onclick="controlPump('on')" class="w-full bg-green-500 hover:bg-green-600 text-white font-bold py-3 px-4 rounded-lg transition-all duration-200 shadow-lg hover:shadow-xl">
              Turn ON
            </button>
            <button onclick="controlPump('off')" class="w-full bg-red-500 hover:bg-red-600 text-white font-bold py-3 px-4 rounded-lg transition-all duration-200 shadow-lg hover:shadow-xl">
              Turn OFF
            </button>
          </div>
          <div id="pump-message" class="mt-3 text-sm font-medium opacity-0 transition-opacity duration-300"></div>
        </div>
      </div>
      <div class="glass-effect rounded-2xl p-6 shadow-2xl hover:transform hover:scale-105 transition-all duration-300">
        <div class="flex items-center mb-4">
          <span class="text-2xl mr-3">🍽️</span>
          <h3 class="text-xl font-semibold text-gray-700">Manual Feeding</h3>
        </div>
        <button onclick="feedNow()" class="w-full bg-gradient-to-r from-orange-500 to-red-500 hover:from-orange-600 hover:to-red-600 text-white font-bold py-4 px-4 rounded-lg transition-all duration-200 shadow-lg hover:shadow-xl text-lg mb-4">
          Feed Fish Now
        </button>
        <div id="feed-message" class="text-center font-medium opacity-0 transition-opacity duration-300"></div>
      </div>
      <div class="glass-effect rounded-2xl p-6 shadow-2xl hover:transform hover:scale-105 transition-all duration-300">
        <div class="flex items-center mb-4">
          <span class="text-2xl mr-3">⏰</span>
          <h3 class="text-xl font-semibold text-gray-700">Daily Schedule</h3>
        </div>
        <div class="space-y-4">
          <div class="grid grid-cols-2 gap-3">
            <div>
              <label class="block text-sm font-medium text-gray-600 mb-1">Hour (0-23)</label>
              <input type="number" id="hour" min="0" max="23" placeholder="14" class="w-full p-3 border-2 border-gray-200 rounded-lg focus:border-blue-500 focus:outline-none transition-colors text-center">
            </div>
            <div>
              <label class="block text-sm font-medium text-gray-600 mb-1">Minute (0-59)</label>
              <input type="number" id="minute" min="0" max="59" placeholder="30" class="w-full p-3 border-2 border-gray-200 rounded-lg focus:border-blue-500 focus:outline-none transition-colors text-center">
            </div>
          </div>
          <button onclick="setTime()" class="w-full bg-gradient-to-r from-blue-500 to-purple-600 hover:from-blue-600 hover:to-purple-700 text-white font-bold py-3 px-4 rounded-lg transition-all duration-200 shadow-lg hover:shadow-xl">
            Set Feeding Time
          </button>
          <div id="time-message" class="text-center font-medium opacity-0 transition-opacity duration-300"></div>
        </div>
      </div>
      <div class="glass-effect rounded-2xl p-6 shadow-2xl hover:transform hover:scale-105 transition-all duration-300">
        <div class="flex items-center mb-4">
          <span class="text-2xl mr-3">📱</span>
          <h3 class="text-xl font-semibold text-gray-700">System Status</h3>
        </div>
        <div class="space-y-3">
          <div class="flex justify-between">
            <span class="text-gray-600">WiFi Status:</span>
            <span class="text-green-600 font-semibold">Connected</span>
          </div>
          <div class="flex justify-between">
            <span class="text-gray-600">LCD Display:</span>
            <span class="text-green-600 font-semibold">Active</span>
          </div>
          <div class="flex justify-between">
            <span class="text-gray-600">Sensors:</span>
            <span class="text-green-600 font-semibold">Online</span>
          </div>
          <div class="flex justify-between">
            <span class="text-gray-600">Last Update:</span>
            <span id="last-update" class="text-blue-600 font-semibold">--:--:--</span>
          </div>
        </div>
      </div>
      <div class="glass-effect rounded-2xl p-6 shadow-2xl hover:transform hover:scale-105 transition-all duration-300">
        <div class="flex items-center mb-4">
          <span class="text-2xl mr-3">⚙️</span>
          <h3 class="text-xl font-semibold text-gray-700">Quick Actions</h3>
        </div>
        <div class="space-y-3">
          <button onclick="refreshData()" class="w-full bg-blue-500 hover:bg-blue-600 text-white font-bold py-2 px-4 rounded-lg transition-all duration-200">
            🔄 Refresh All Data
          </button>
          <button onclick="location.reload()" class="w-full bg-gray-500 hover:bg-gray-600 text-white font-bold py-2 px-4 rounded-lg transition-all duration-200">
            🔃 Reload Page
          </button>
        </div>
      </div>
    </div>
  </div>
  <div id="notification" class="notification"></div>
<script>
  function showNotification(text, type) {
    const notification = document.getElementById('notification');
    notification.textContent = text;
    notification.className = 'notification ' + type;
    notification.style.display = 'block';
    setTimeout(() => {
      notification.style.display = 'none';
    }, 4000);
  }
  function showMessage(elementId, text, type) {
    const element = document.getElementById(elementId);
    element.textContent = text;
    element.style.opacity = '1';
    element.style.color = 'white';
    element.style.background = type === 'success' ? '#10b981' : type === 'error' ? '#ef4444' : '#3b82f6';
    element.style.padding = '8px';
    element.style.borderRadius = '8px';
    setTimeout(() => {
      element.style.opacity = '0';
    }, 3000);
  }
  function updateLastUpdateTime() {
    const now = new Date();
    const timeString = now.toLocaleTimeString();
    document.getElementById('last-update').textContent = timeString;
  }
  function updateSensorData() {
    fetch('/foodlevel')
      .then(response => response.json())
      .then(data => {
        document.getElementById('distance-display').textContent = data.distance + ' cm';
        document.getElementById('distance-circle').style.background = data.color;
        const statusDiv = document.getElementById('food-status');
        statusDiv.textContent = data.status;
        statusDiv.style.background = data.color;
        const circle = document.getElementById('distance-circle');
        if (data.distance > 5) {
          circle.classList.add('pulse-animation');
        } else {
          circle.classList.remove('pulse-animation');
        }
      })
      .catch(error => console.log('Error updating food data:', error));
    fetch('/environment')
      .then(response => response.json())
      .then(data => {
        const tempValue = data.temperature === -999 ? 'Error' : data.temperature + '°C';
        const humidityValue = data.humidity === -999 ? 'Error' : data.humidity + '%';
        document.getElementById('temp-value').textContent = tempValue;
        document.getElementById('humidity-value').textContent = humidityValue;
        document.getElementById('temp-display').style.background = data.tempColor;
        const tempDisplay = document.getElementById('temp-display');
        if (data.temperature > 30) {
          tempDisplay.classList.add('pulse-animation');
        } else {
          tempDisplay.classList.remove('pulse-animation');
        }
      })
      .catch(error => console.log('Error updating environment data:', error));
    fetch('/watertemp')
      .then(response => response.json())
      .then(data => {
        const waterTempValue = data.waterTemp === -999 ? 'Error' : data.waterTemp + '°C';
        document.getElementById('water-temp-display').textContent = waterTempValue;
        document.getElementById('water-temp-circle').style.background = data.waterTempColor;
        const statusDiv = document.getElementById('water-temp-status');
        statusDiv.textContent = data.waterTempStatus;
        statusDiv.style.background = data.waterTempColor;
        const circle = document.getElementById('water-temp-circle');
        if (data.waterTemp < 20 || data.waterTemp > 32) {
          circle.classList.add('pulse-animation');
        } else {
          circle.classList.remove('pulse-animation');
        }
      })
      .catch(error => console.log('Error updating water temperature data:', error));
    fetch('/waterquality')
      .then(response => response.json())
      .then(data => {
        document.getElementById('ph-value').textContent = data.ph;
        document.getElementById('tds-value').textContent = data.tds + ' ppm';
        document.getElementById('ph-display').style.background = data.phColor;
        document.getElementById('tds-display').style.background = data.tdsColor;
        const pumpStatusDiv = document.getElementById('pump-status');
        pumpStatusDiv.textContent = data.pumpStatus ? 'ON' : 'OFF';
        pumpStatusDiv.style.background = data.pumpStatus ? '#10b981' : '#6b7280';
      })
      .catch(error => console.log('Error updating water quality data:', error));
    updateLastUpdateTime();
  }
  setInterval(updateSensorData, 5000);
  function feedNow() {
    const button = event.target;
    button.style.transform = 'scale(0.95)';
    fetch("/feedNow").then(res => res.text()).then(txt => {
      button.style.transform = 'scale(1)';
      if (txt === 'Fed!') {
        showNotification('🐠 Fish fed successfully!', 'success');
        showMessage('feed-message', 'Feeding completed!', 'success');
        setTimeout(updateSensorData, 1000);
      } else if (txt === 'NO_FOOD') {
        showNotification('⚠️ Cannot feed - Food container is empty!', 'error');
        showMessage('feed-message', 'Please refill food!', 'error');
      }
    });
  }
  function controlPump(action) {
    fetch(`/pump?action=${action}`)
      .then(res => res.text())
      .then(txt => {
        if (txt.includes('turned ON')) {
          showNotification('⚡ Water pump turned ON!', 'success');
          showMessage('pump-message', 'Pump is now ON', 'success');
        } else if (txt.includes('turned OFF')) {
          showNotification('⚡ Water pump turned OFF!', 'info');
          showMessage('pump-message', 'Pump is now OFF', 'info');
        }
        setTimeout(() => {
          fetch('/waterquality')
            .then(response => response.json())
            .then(data => {
              const pumpStatusDiv = document.getElementById('pump-status');
              pumpStatusDiv.textContent = data.pumpStatus ? 'ON' : 'OFF';
              pumpStatusDiv.style.background = data.pumpStatus ? '#10b981' : '#6b7280';
            });
        }, 500);
      })
      .catch(error => {
        showNotification('❌ Error controlling pump!', 'error');
        showMessage('pump-message', 'Error occurred!', 'error');
      });
  }
  function setTime() {
    const h = document.getElementById("hour").value;
    const m = document.getElementById("minute").value;
    if (!h || !m) {
      showNotification('⚠️ Please enter both hour and minute!', 'error');
      return;
    }
    if (h < 0 || h > 23 || m < 0 || m > 59) {
      showNotification('⚠️ Invalid time format!', 'error');
      return;
    }
    const button = event.target;
    button.style.transform = 'scale(0.95)';
    fetch(`/setTime?hour=${h}&minute=${m}`).then(res => res.text()).then(txt => {
      button.style.transform = 'scale(1)';
      if (txt === 'Feeding time set!') {
        const timeStr = `${h}:${String(m).padStart(2, '0')}`;
        showNotification(`⏰ Daily feeding set for ${timeStr}!`, 'info');
        showMessage('time-message', `Set for ${timeStr}`, 'success');
      } else {
        showNotification('❌ Error setting feeding time!', 'error');
        showMessage('time-message', 'Error occurred!', 'error');
      }
    });
  }
  function refreshData() {
    updateSensorData();
    showNotification('🔄 All data refreshed!', 'info');
  }
  updateSensorData();
</script>
</body>
</html>)rawliteral";
}

// ==== Setup ====
void setup() {
  Serial.begin(115200);

  // Initialize I2C and LCD
  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Smart Aquarium");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  // Initialize DS18B20 water temperature sensor
  waterTempSensor.begin();
  Serial.println("DS18B20 Water Temperature Sensor initialized");

  // Initialize ultrasonic sensor pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Initialize DHT pin
  pinMode(DHT_PIN, INPUT_PULLUP);

  // Initialize pump pin
  pinMode(pumpPin, OUTPUT);
  digitalWrite(pumpPin, LOW);

  // Servo
  myServo.attach(servoPin);
  myServo.write(0);

  // Blynk and Wi-Fi Connection
  lcd.setCursor(0, 1);
  lcd.print("Connecting WiFi...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  Serial.println("Connected to Blynk and WiFi!");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected:");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(3000);

  // Time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/feedNow", HTTP_GET, handleFeedNow);
  server.on("/setTime", HTTP_GET, handleSetTime);
  server.on("/foodlevel", HTTP_GET, handleFoodLevel);
  server.on("/environment", HTTP_GET, handleEnvironment);
  server.on("/watertemp", HTTP_GET, handleWaterTemperature);
  server.on("/waterquality", HTTP_GET, handleWaterQuality);
  server.on("/pump", HTTP_GET, handlePumpControl);

  server.begin();

  // Blynk Timer Setup
  timer.setInterval(5000L, updateBlynkSensors);

  // Print initial status
  delay(1000);
  float distance = measureDistance();
  float waterTemp = readWaterTemperature();
  Serial.println("Initial food level: " + String(distance, 1) + "cm - " + getFoodStatus(distance));
  Serial.println("Initial water temperature: " + String(waterTemp, 1) + "°C - " + getWaterTemperatureStatus(waterTemp));
  Serial.println("System ready!");
}

// ==== Loop ====
void loop() {
  server.handleClient();
  Blynk.run();
  timer.run();
  updateLCD();

  // Check current time for scheduled feeding
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to get time");
    return;
  }

  int currentHour = timeinfo.tm_hour;
  int currentMinute = timeinfo.tm_min;

  if (feedHour != -1 && feedMinute != -1) {
    if (currentHour == feedHour && currentMinute == feedMinute && !hasFedToday) {
      rotateServo();
      hasFedToday = true;
    }
    if (currentHour == 0 && currentMinute == 0) {
      hasFedToday = false;
    }
  }

  // Print status every 30 seconds
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 30000) {
    float distance = measureDistance();
    float ph = readPH();
    float tds = readTDS();
    float waterTemp = readWaterTemperature();
    Serial.println("Status - Food: " + String(distance, 1) + "cm, pH: " + String(ph, 1) + ", TDS: " + String(tds, 0) + " ppm, Water Temp: " + String(waterTemp, 1) + "°C, Pump: " + (pumpStatus ? "ON" : "OFF"));
    lastPrint = millis();
  }
}