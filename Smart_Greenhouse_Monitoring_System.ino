#define BLYNK_TEMPLATE_ID "TMPL6PGPi_Y5f"
#define BLYNK_TEMPLATE_NAME "Smart Greenhouse"
#define BLYNK_AUTH_TOKEN "1NSzDax5S0y7Q8UX16TdwarfnYN0M_i0"

#define BLYNK_PRINT Serial
// Include the library files
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <esp_wifi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>                       // DHT11 library
#include <Adafruit_Sensor.h>
#include <Adafruit_SSD1306.h>          // OLED library
#include <Adafruit_GFX.h>

// Define pins for soil moisture system
#define sensor1 34   // Soil moisture sensor 1
#define sensor2 35   // Soil moisture sensor 2
#define relay1 4  // Relay (Pump 1)
#define relay2 13    // Relay (Pump 2)

// Define pins for water sensor and buzzer
#define BUZZER_PIN 23  // Buzzer pin
#define POWER_PIN 17   // Water sensor power pin
#define SIGNAL_PIN 36  // Water sensor signal pin
#define WATER_THRESHOLD 1500

// Define pins for DHT11 and LDR
#define DHTPIN 14    // Pin connected to DHT11
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
#define LDRPIN 39    // Pin connected to LDR

// Define pins for fan and lamp relays
#define relayFan 25    // Fan control relay
#define relayLamp 26   // Lamp control relay

// Threshold values
const int lightThresholdOn = 0;  // Turn lamp ON at 0% light intensity
const int lightThresholdOff = 80; // Turn lamp OFF at 80% light intensity
const int humidityThreshold = 30; // Soil humidity threshold (%)
const int fanOnThreshold = 16;   // Temperature threshold to turn fan on (%)
const int fanOffThreshold = 14;  // Temperature threshold to turn fan off (%)

// Initialize components
LiquidCrystal_I2C lcd1(0x27, 16, 2);   // LCD for Sensor 1 and Pump 1
LiquidCrystal_I2C lcd2(0x26, 16, 2);   // LCD for Sensor 2 and Pump 2

#define SCREEN_WIDTH 128               // OLED width, in pixels
#define SCREEN_HEIGHT 64               // OLED height, in pixels
#define OLED_RESET    -1               // Reset pin (or -1 if sharing the ESP32 reset)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Blynk and Wi-Fi credentials
BlynkTimer timer;
char auth[] = "1NSzDax5S0y7Q8UX16TdwarfnYN0M_i0";
char ssid[] = "Nakhosport";
char pass[] = "12345678";

void setup() {
  // Debug console
  Serial.begin(115200);
  analogSetAttenuation(ADC_11db);

  //wifi
 wifi_config_t config;
 esp_wifi_get_config(WIFI_IF_STA, &config);
 config.sta.listen_interval = 0;
 esp_wifi_set_config(WIFI_IF_STA, &config);

  // Initialize Blynk
  Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);
  // Initialize water sensor and buzzer pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Initialize LCDs
  lcd1.init();
  lcd1.backlight();
  lcd2.init();
  lcd2.backlight();

  // Initialize relays
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relayFan, OUTPUT);
  pinMode(relayLamp, OUTPUT);
  digitalWrite(relay1, HIGH);  // Turn OFF initially
  digitalWrite(relay2, HIGH);  // Turn OFF initially
  digitalWrite(relayFan, HIGH); // Turn OFF fan initially
  digitalWrite(relayLamp, HIGH); // Turn OFF lamp initially

  // Initialize DHT11
  dht.begin();
  Serial.println("DHT11 Initialized");

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("System Loading...");
  display.display();
  delay(2000);

  // Loading messages on LCDs
  lcd1.setCursor(0, 0);
  lcd1.print("System 1 Loading");
  lcd2.setCursor(0, 0);
  lcd2.print("System 2 Loading");

  for (int a = 0; a <= 15; a++) {
    lcd1.setCursor(a, 1);
    lcd1.print(".");
    lcd2.setCursor(a, 1);
    lcd2.print(".");
    delay(200);
  }
  lcd1.clear();
  lcd2.clear();

  // Set a timer to check moisture periodically
  timer.setInterval(1000L, checkSoilMoisture);

  // Set a timer to update DHT11 and LDR values
  timer.setInterval(2000L, updateOLED);

  //temperature
  timer.setInterval(2000L, checkTemperature);
  
  //set timer to water level
  timer.setInterval(1000L, checkWaterSensor);
  
  // Set timer for light intensity checks
  timer.setInterval(1000L, checkLightIntensity);
}

void checkWaterSensor() {
  digitalWrite(POWER_PIN, HIGH);
  delay(100);
  int waterSensorValue = analogRead(SIGNAL_PIN);
  digitalWrite(POWER_PIN, LOW);

  Serial.print("Water sensor value: ");
  Serial.println(waterSensorValue);

  if (waterSensorValue > WATER_THRESHOLD) {
    Serial.println("Warning! Water Overlimit!");
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// Function to check soil moisture and control fan
void checkSoilMoisture() {
  // ----- System 1 -----
  int moisture1 = analogRead(sensor1);
  moisture1 = map(moisture1, 0, 4095, 0, 100);
  moisture1 = (100 - moisture1);  // Convert to percentage
  Blynk.virtualWrite(V0, moisture1); // Send data to Blynk for system 1
  lcd1.setCursor(0, 0);
  lcd1.print("Moisture 1: ");
  lcd1.print(moisture1);
  lcd1.print(" %  ");
  
 if (moisture1 < humidityThreshold) { //auto moisture pump 1
  digitalWrite(relay1, LOW); // Turn pump on
    Blynk.logEvent("pump1_activated", "Pump 1 Activated: Soil moisture low");
    delay(1000); 
    lcd1.setCursor(0, 1);
    lcd1.print("Motor 1: ON ");
   } else {
    digitalWrite(relay1, HIGH); // Turn pump off
    delay(1000); 
    lcd1.setCursor(0, 1);
    lcd1.print("Motor 1: OFF "); 
   }

  // ----- System 2 -----
  int moisture2 = analogRead(sensor2);
  moisture2 = map(moisture2, 0, 4095, 0, 100);
  moisture2 = (100 - moisture2);  // Convert to percentage
  Blynk.virtualWrite(V2, moisture2); // Send data to Blynk for system 2
  lcd2.setCursor(0, 0);
  lcd2.print("Moisture 2: ");
  lcd2.print(moisture2);
  lcd2.print(" %  ");
}

void sendTemperatureToBlynk() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

    Blynk.virtualWrite(V9, temperature);
    Blynk.virtualWrite(V8, humidity);
    Serial.print("   Temperature : ");
    Serial.print(temperature);
    Serial.print("    Humidity : ");
    Serial.println(humidity);
}   

void checkLightIntensity() {
  // Read LDR value and convert to percentage
  int lightValue = analogRead(LDRPIN);
  int lightPercent = map(lightValue, 4095, 0, 0, 100);

  // Display light intensity on Serial Monitor
  Serial.print("Light Intensity: ");
  Serial.print(lightPercent);
  Serial.println("%");

  // Control lamp based on light intensity
  if (lightPercent <= lightThresholdOn) {
    digitalWrite(relayLamp, LOW); // Turn lamp ON
    Blynk.logEvent("lamp_activated", "Lamp Activated: Low light intensity");
    Serial.println("Lamp: ON");
  } else if (lightPercent >= lightThresholdOff) {
    digitalWrite(relayLamp, HIGH); // Turn lamp OFF
    Serial.println("Lamp: OFF");
  }
}

void checkTemperature() {
  // Read temperature from DHT11
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Display temperature on Serial Monitor
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println("°C");

  // Control fan based on temperature
  if (temperature >= fanOnThreshold) {
    digitalWrite(relayFan, LOW); // Turn fan on
    Blynk.logEvent("fan_activated", "Fan Activated: High temperature");
    Serial.println("Fan: ON");
  } else if (temperature <= fanOffThreshold) {
    digitalWrite(relayFan, HIGH); // Turn fan off
    Serial.println("Fan: OFF");
  }
}

// Function to update OLED with DHT11, LDR data, and control lamp
void updateOLED() {
  // Read DHT11 values
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Read LDR value
  int ldrValue = analogRead(LDRPIN);
  int lightPercent = map(ldrValue, 4095, 0, 0, 100); // Inverted mapping
  Blynk.virtualWrite(V6, lightPercent); // Send light data to Blynk



  // Read water sensor value
  int waterSensorValue = analogRead(SIGNAL_PIN);

  // Update OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("DHT11 & LDR Values:");
  display.print("Temp: "); display.print(temperature); display.println(" C");
  display.print("Hum : "); display.print(humidity); display.println(" %");
  display.print("Light: "); display.print(lightPercent); display.println(" %");

 // Placeholder for water level percentage
 int waterLevelPercent = 0;
  // Add water level to OLED display
  display.print("Water: "); display.print(waterLevelPercent); display.println(" %");

  display.display();
}


// Blynk button control for Pump 1
BLYNK_WRITE(V1) {
  bool pump1State = param.asInt();
  if (pump1State) {
    digitalWrite(relay1, LOW);
    delay(1000); 
    lcd1.setCursor(0, 1);
    lcd1.print("Motor 1: ON ");
  } else {
    digitalWrite(relay1, HIGH);
    delay(1000); 
    lcd1.setCursor(0, 1);
    lcd1.print("Motor 1: OFF");
  }
}

// Blynk button control for Pump 2
BLYNK_WRITE(V3) {
  bool pump2State = param.asInt();
  if (pump2State) {
    digitalWrite(relay2, LOW);
    delay(1000); 
    lcd2.setCursor(0, 1);
    lcd2.print("Motor 2: ON ");
  } else {
    digitalWrite(relay2, HIGH);
    delay(1000); 
    lcd2.setCursor(0, 1);
    lcd2.print("Motor 2: OFF");
  }
}

// Manual control for Fan
BLYNK_WRITE(V5) {
  bool fanState = param.asInt();
  if (fanState) {
    digitalWrite(relayFan, LOW); // Turn fan ON
    delay(1000); 
    Serial.println("Fan: Manual ON");
  } else {
    digitalWrite(relayFan, HIGH); // Turn fan OFF
    delay(1000); 
    Serial.println("Fan: Manual OFF");
  }
}

// Manual control for Lamp
BLYNK_WRITE(V7) {
  bool lampState = param.asInt();
  if (lampState) {
    digitalWrite(relayLamp, LOW); // Turn lamp ON
    delay(1000); 
    Serial.println("Lamp: Manual ON");
  } else {
    digitalWrite(relayLamp, HIGH); // Turn lamp OFF
    delay(1000); 
    Serial.println("Lamp: Manual OFF");
  }
}

void loop() {
  Blynk.run();    // Run Blynk library
  timer.run();    // Run timer for soil moisture and OLED updates
}
