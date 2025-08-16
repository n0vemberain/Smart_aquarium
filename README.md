# Smart_aquarium
Smart Aquarium, ESP32-based, monitors air temp, humidity (DHT11), water temp (DS18B20), pH, TDS, food level. Servo automates feeding; pump controls water. Blynk 2.0 app updates data, controls feeding/pump. Alerts for out-of-range values (e.g., temp &lt;15°C/>30°C). Web dashboard, LCD display. Wi-Fi, NTP-enabled. Ideal for hobbyists.


The Smart Aquarium is an IoT-based system designed to automate and monitor an aquarium’s environment, ensuring optimal conditions for aquatic life. Built on an ESP32 microcontroller, it integrates sensors, actuators, and connectivity for real-time monitoring, control, and notifications. The system is tailored for hobbyists, offering user-friendly interfaces and automation to simplify fish care.
Hardware Components

ESP32 Microcontroller: The core processing unit, providing Wi-Fi connectivity and ample GPIO pins for sensor and actuator integration.
DHT11 Sensor: Measures air temperature (threshold: <15°C or >30°C) and humidity (<30% or >80%) around the aquarium.
DS18B20 Sensor: A waterproof sensor monitoring water temperature (threshold: <20°C or >32°C).
Ultrasonic Sensor (HC-SR04): Measures food container distance to detect low food levels (>5 cm).
pH and TDS Sensors (Simulated): Simulate pH (threshold: <6.8 or >8.2) and TDS (<180 or >280 ppm) for water quality monitoring.
Servo Motor (SG90): Connected to GPIO 13, automates fish feeding by dispensing food.
Relay Module: Controls a water pump (GPIO 14) for water circulation.
16x2 LCD with I2C Interface: Displays sensor data (food level, air/water temperature, pH, TDS, pump status) on a rotating 3-second cycle.
Wiring and Power: I2C (SDA: GPIO 21, SCL: GPIO 22), 5V/3.3V power supply for sensors and actuators.

# Software Components

Arduino IDE: Used for programming and uploading code to the ESP32.
Blynk 2.0 Library: Enables cloud-based monitoring and control via the Blynk mobile app.
ESP32Servo Library: Controls the servo motor for precise feeding actions.
DallasTemperature and OneWire Libraries: Interface with the DS18B20 sensor for water temperature readings.
LiquidCrystal_I2C Library: Drives the LCD for local display (address: 0x27, may require verification).
WiFi and WebServer Libraries: Facilitate Wi-Fi connectivity and host a web dashboard.
NTPClient Library: Synchronizes time with pool.ntp.org for accurate feeding schedules.
Tailwind CSS: Styles the web dashboard for a responsive, modern interface.

# Functionality
The Smart Aquarium monitors key parameters in real-time:

Air Temperature and Humidity: DHT11 provides readings, updated every 5 seconds.
Water Temperature: DS18B20 ensures optimal aquatic conditions.
pH and TDS: Simulated sensors track water quality.
Food Level: Ultrasonic sensor detects when food is low.

# Automation:

Feeding: A servo dispenses food manually (via Blynk or web) or on a user-set schedule. Feeding is blocked if food is empty (>5 cm).
Water Pump: Controlled via relay for circulation, toggled through Blynk or web.

# Notifications:

Blynk 2.0 sends alerts when thresholds are breached (e.g., LOW_FOOD, AIR_TEMP_ALERT, HUMIDITY_ALERT, WATER_TEMP_ALERT, PH_ALERT, TDS_ALERT, FEED_SUCCESS), with a 10-minute cooldown to prevent spam.

# Interfaces:

Blynk App: Displays data , supports remote control.
Web Dashboard: Hosted on the ESP32, provides a browser-based interface with real-time sensor data and controls.
LCD Display: Shows rotating sensor data for local monitoring.

# Connectivity:

Wi-Fi connects to the user’s network for Blynk and web access.
NTP ensures accurate time for scheduled feeding.

The Smart Aquarium automates feeding and water management, provides instant alerts for abnormal conditions, and offers seamless control via app, web, and LCD, making it an efficient, reliable solution for maintaining a healthy aquarium.

# Mobile App (Blynk):

![photo_2025-08-16_21-05-19](https://github.com/user-attachments/assets/25472491-80bd-4a0f-895b-86f9d99360a7)

# Web App (Local):

<img width="1440" height="782" alt="Screenshot 2025-08-16 at 9 06 15 PM" src="https://github.com/user-attachments/assets/e351705f-9672-4a46-818c-f4856aba3543" />

# Diagram 

<img width="1920" height="1080" alt="smart aquarium " src="https://github.com/user-attachments/assets/2b35a07d-300f-4fd1-a5cb-a3f244a54874" />

### Smart Aquarium Connection Details
a. **ESP32 Microcontroller**:
   - Power: 3.3V and GND pins connected to a stable 3.3V/5V power supply.
   - Wi-Fi enabled for internet connectivity (no physical connection required).

b. **DHT11 Sensor (Air Temperature and Humidity)**:
   - VCC: Connected to 3.3V or 5V (check sensor specs).
   - GND: Connected to GND.
   - Data: Connected to GPIO 15 with a 10kΩ pull-up resistor to 3.3V.

c. **DS18B20 Sensor (Water Temperature)**:
   - VCC: Connected to 3.3V or 5V.
   - GND: Connected to GND.
   - Data: Connected to GPIO 4 with a 4.7kΩ pull-up resistor to 3.3V.

d. **HC-SR04 Ultrasonic Sensor (Food Level)**:
   - VCC: Connected to 5V.
   - GND: Connected to GND.
   - Trig: Connected to GPIO 5.
   - Echo: Connected to GPIO 18.

e. **SG90 Servo Motor (Feeding Mechanism)**:
   - VCC: Connected to 5V.
   - GND: Connected to GND.
   - Signal: Connected to GPIO 13.

f. **Relay Module (Water Pump Control)**:
   - VCC: Connected to 3.3V or 5V (check module specs).
   - GND: Connected to GND.
   - Signal: Connected to GPIO 14.
   - Pump: Connected to the relay’s NO (Normally Open) and COM (Common) terminals, with the pump powered by an external 5V/12V supply.

g. **16x2 LCD with I2C Interface**:
   - VCC: Connected to 5V.
   - GND: Connected to GND.
   - SDA: Connected to GPIO 21.
   - SCL: Connected to GPIO 22.
   - Ensure the I2C address (e.g., 0x27) matches your setup.

h. **Power Supply**:
   - Use a 5V power source (e.g., USB or external adapter) to power the ESP32 and peripherals. Ensure adequate current (e.g., 2A) for the servo and pump.

#### Wiring Notes
1. Use jumper wires or a breadboard for prototyping.
2. Add appropriate pull-up resistors (10kΩ for DHT11, 4.7kΩ for DS18B20) to ensure reliable data transmission.
3. Connect all GND pins to a common ground to avoid floating issues.
4. For the pump, use an external power supply if current exceeds ESP32 capabilities; control only via the relay.
5. Secure connections with heat-shrink tubing or terminal blocks for durability.

#### Verification
- Test each connection with a multimeter to confirm continuity and voltage levels.
- Use an I2C scanner sketch to verify the LCD’s address if the display fails to initialize.



