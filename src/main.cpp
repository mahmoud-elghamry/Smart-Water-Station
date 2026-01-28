/**
 * Smart Water Quality Monitoring System
 * ESP32 + MQTT + Safety System
 */

#include "config.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <Wire.h>


// Objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

// State
struct {
  float ph = 7.0;
  float turbidity = 0;
  float flowRate = 10.0;
  bool pumpOn = false;
  bool pumpLocked = false;
  bool wifiOk = false;
  bool mqttOk = false;
  int alertType = 0;
} state;

struct {
  float ph = -1;
  float turbidity = -1;
  float flowRate = -1;
  bool pumpOn = false;
  bool pumpLocked = false;
} prevState;

unsigned long lastPublish = 0;
unsigned long lastDisplay = 0;
unsigned long lastAlert = 0;
String clientId;

// Function declarations
void initHardware();
void initDisplay();
bool connectWiFi();
bool connectMQTT();
void onMqttMessage(char *topic, byte *payload, unsigned int length);
void readSensors();
float readUltrasonic();
void checkSafety();
void publishData();
void updateDisplay();
void setPump(bool on);

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n=== Water Station ===");
  clientId = "ESP32_" + String(random(0xFFFF), HEX);

  initHardware();
  initDisplay();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);

  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(onMqttMessage);
  mqtt.setBufferSize(512);

  Serial.println("[OK] Ready");
}

void loop() {
  unsigned long now = millis();

  // Connection management
  if (!state.wifiOk) {
    state.wifiOk = connectWiFi();
  } else {
    state.wifiOk = (WiFi.status() == WL_CONNECTED);
  }

  if (state.wifiOk && !state.mqttOk) {
    state.mqttOk = connectMQTT();
  } else if (state.mqttOk) {
    if (!mqtt.loop())
      state.mqttOk = false;
  }

  // Sensor reading and publishing
  if (now - lastPublish >= PUBLISH_INTERVAL) {
    lastPublish = now;
    readSensors();
    checkSafety();
    if (state.mqttOk)
      publishData();
  }

  // Display update
  if (now - lastDisplay >= DISPLAY_INTERVAL) {
    lastDisplay = now;
    updateDisplay();
  }
}

void initHardware() {
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, LOW);

  pinMode(ULTRASONIC_TRIG, OUTPUT);
  pinMode(ULTRASONIC_ECHO, INPUT);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
}

void initDisplay() {
  Wire.begin(I2C_SDA, I2C_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("[ERR] OLED");
    return;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 25);
  display.println("Water Station");
  display.setCursor(30, 40);
  display.println("Starting...");
  display.display();
}

bool connectWiFi() {
  Serial.print("[WiFi] Connecting...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(250);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" OK");
    return true;
  }
  Serial.println(" Failed");
  return false;
}

bool connectMQTT() {
  if (mqtt.connect(clientId.c_str())) {
    Serial.println("[MQTT] Connected");
    mqtt.subscribe(TOPIC_CONTROL);
    return true;
  }
  return false;
}

void onMqttMessage(char *topic, byte *payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.print("[CMD] ");
  Serial.println(msg);

  if (msg == "PUMP_ON") {
    if (state.pumpLocked) {
      Serial.println("[!] Locked");
    } else {
      setPump(true);
    }
  } else if (msg == "PUMP_OFF") {
    setPump(false);
  } else if (msg == "RESET_SAFETY") {
    state.pumpLocked = false;
    state.alertType = 0;
    Serial.println("[OK] Reset");
  }
}

void readSensors() {
  // pH (potentiometer: 0-4095 -> 0-14)
  int phRaw = analogRead(PH_SENSOR_PIN);
  state.ph = (phRaw / 4095.0) * 14.0;

  // Turbidity (LDR: high light = clear water)
  int ldrRaw = analogRead(TURBIDITY_LDR_PIN);
  state.turbidity = (1.0 - (ldrRaw / 4095.0)) * 1000.0;

  // Flow rate (ultrasonic distance -> flow)
  float dist = readUltrasonic();
  if (dist < 2)
    dist = 2;
  if (dist > 400)
    dist = 400;
  state.flowRate = 20.0 - ((dist - 2) / 398.0 * 20.0);
  if (state.flowRate < 0)
    state.flowRate = 0;
}

float readUltrasonic() {
  digitalWrite(ULTRASONIC_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG, LOW);

  long duration = pulseIn(ULTRASONIC_ECHO, HIGH, 30000);
  return duration * 0.034 / 2;
}

void checkSafety() {
  bool danger = false;
  int alert = 0;

  if (state.ph < PH_CRITICAL_MIN) {
    danger = true;
    alert = 1;
  } else if (state.ph > PH_CRITICAL_MAX) {
    danger = true;
    alert = 2;
  } else if (state.turbidity > TURBIDITY_CRITICAL) {
    danger = true;
    alert = 3;
  } else if (state.pumpOn && state.flowRate < FLOW_CRITICAL_MIN) {
    danger = true;
    alert = 4;
  }

  if (danger) {
    if (state.pumpOn) {
      setPump(false);
      Serial.println("[!] Emergency stop");
    }
    state.pumpLocked = true;
    state.alertType = alert;

    if (millis() - lastAlert >= ALERT_COOLDOWN) {
      lastAlert = millis();

      StaticJsonDocument<200> doc;
      doc["type"] = alert;
      doc["ph"] = state.ph;
      doc["turbidity"] = state.turbidity;
      doc["flowRate"] = state.flowRate;

      char buf[200];
      serializeJson(doc, buf);
      mqtt.publish(TOPIC_ALERTS, buf);
    }
  } else if (state.pumpLocked) {
    bool phOk = (state.ph >= PH_SAFE_MIN && state.ph <= PH_SAFE_MAX);
    bool turbOk = (state.turbidity < TURBIDITY_SAFE);
    bool flowOk = (state.flowRate >= FLOW_NORMAL_MIN || !state.pumpOn);

    if (phOk && turbOk && flowOk) {
      state.pumpLocked = false;
      state.alertType = 0;
      Serial.println("[OK] Unlocked");
    }
  }
}

void publishData() {
  bool changed = false;
  if (abs(state.ph - prevState.ph) > 0.1)
    changed = true;
  if (abs(state.turbidity - prevState.turbidity) > 10)
    changed = true;
  if (abs(state.flowRate - prevState.flowRate) > 0.5)
    changed = true;
  if (state.pumpOn != prevState.pumpOn)
    changed = true;
  if (state.pumpLocked != prevState.pumpLocked)
    changed = true;

  StaticJsonDocument<200> doc;
  doc["ph"] = round(state.ph * 100) / 100.0;
  doc["turbidity"] = round(state.turbidity);
  doc["flowRate"] = round(state.flowRate * 10) / 10.0;
  doc["pump"] = state.pumpOn ? 1 : 0;
  doc["locked"] = state.pumpLocked ? 1 : 0;
  doc["alert"] = state.alertType;

  char buf[200];
  serializeJson(doc, buf);
  mqtt.publish(TOPIC_SENSORS, buf);

  if (changed) {
    Serial.print("[DATA] pH:");
    Serial.print(state.ph, 1);
    Serial.print(" T:");
    Serial.print(state.turbidity, 0);
    Serial.print(" F:");
    Serial.print(state.flowRate, 1);
    Serial.print(" P:");
    Serial.println(state.pumpOn ? "ON" : "OFF");

    prevState.ph = state.ph;
    prevState.turbidity = state.turbidity;
    prevState.flowRate = state.flowRate;
    prevState.pumpOn = state.pumpOn;
    prevState.pumpLocked = state.pumpLocked;
  }
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  if (state.pumpLocked) {
    display.println("!! SAFETY ALERT !!");
  } else {
    display.println("=== Water Station ===");
  }

  display.setCursor(0, 10);
  display.print("W:");
  display.print(state.wifiOk ? "OK" : "--");
  display.print(" M:");
  display.print(state.mqttOk ? "OK" : "--");

  display.drawLine(0, 19, 128, 19, SSD1306_WHITE);

  // pH
  display.setCursor(0, 22);
  display.print("pH: ");
  display.print(state.ph, 1);
  display.setCursor(70, 22);
  if (state.ph < PH_CRITICAL_MIN || state.ph > PH_CRITICAL_MAX)
    display.print("[BAD]");
  else if (state.ph < PH_SAFE_MIN || state.ph > PH_SAFE_MAX)
    display.print("[LOW]");
  else
    display.print("[OK]");

  // Turbidity
  display.setCursor(0, 32);
  display.print("Turb: ");
  display.print(state.turbidity, 0);
  display.setCursor(70, 32);
  if (state.turbidity > TURBIDITY_CRITICAL)
    display.print("[BAD]");
  else if (state.turbidity > TURBIDITY_SAFE)
    display.print("[HI]");
  else
    display.print("[OK]");

  // Flow
  display.setCursor(0, 42);
  display.print("Flow: ");
  display.print(state.flowRate, 1);
  display.setCursor(70, 42);
  if (state.flowRate < FLOW_CRITICAL_MIN && state.pumpOn)
    display.print("[DRY]");
  else if (state.flowRate < FLOW_NORMAL_MIN)
    display.print("[LOW]");
  else
    display.print("[OK]");

  display.drawLine(0, 52, 128, 52, SSD1306_WHITE);

  display.setCursor(0, 55);
  display.print("PUMP: ");
  if (state.pumpLocked)
    display.print("LOCKED");
  else if (state.pumpOn)
    display.print("ON");
  else
    display.print("OFF");

  display.display();
}

void setPump(bool on) {
  if (on && state.pumpLocked)
    return;
  state.pumpOn = on;
  digitalWrite(PUMP_RELAY_PIN, on ? HIGH : LOW);
}