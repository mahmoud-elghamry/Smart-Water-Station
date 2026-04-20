/**
 * Smart Water Quality Monitoring System
 * Configuration - Pin definitions, thresholds, and settings
 * 
 * Version: 2.0.0
 * Board: ESP32 DevKit V4
 */

#ifndef CONFIG_H
#define CONFIG_H

// ── System ─────────────────────────────────────────
#define SYSTEM_VERSION      "2.0.0"
#define SYSTEM_NAME         "SmarterWater"

// ── Sensor Pins ────────────────────────────────────
#define PH_SENSOR_PIN       34    // Analog - Potentiometer (pH simulation)
#define TURBIDITY_LDR_PIN   35    // Analog - LDR (turbidity simulation)
#define TEMP_NTC_PIN        36    // Analog - NTC thermistor (temperature)
#define ULTRASONIC_TRIG     32    // Digital - HC-SR04 trigger
#define ULTRASONIC_ECHO     33    // Digital - HC-SR04 echo

// ── Output Pins ────────────────────────────────────
#define PUMP_RELAY_PIN      2     // Relay module IN
#define BUZZER_PIN          25    // Passive buzzer
#define RGB_RED_PIN         26    // RGB LED - Red
#define RGB_GREEN_PIN       27    // RGB LED - Green
#define RGB_BLUE_PIN        14    // RGB LED - Blue

// ── Input Pins ─────────────────────────────────────
#define RESET_BUTTON_PIN    4     // Safety reset push button

// ── I2C (OLED Display) ────────────────────────────
#define I2C_SDA             21
#define I2C_SCL             22
#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT       64
#define OLED_ADDRESS        0x3C

// ── WiFi ───────────────────────────────────────────
#define WIFI_SSID           "Wokwi-GUEST"
#define WIFI_PASSWORD       ""
#define WIFI_TIMEOUT_MS     10000
#define WIFI_MAX_RETRIES    5

// ── MQTT ───────────────────────────────────────────
#define MQTT_BROKER         "broker.hivemq.com"
#define MQTT_PORT           1883
#define MQTT_BUFFER_SIZE    512

// Topics
#define TOPIC_SENSORS       "grad/project/sensors"
#define TOPIC_ALERTS        "grad/project/alerts"
#define TOPIC_CONTROL       "grad/project/control"
#define TOPIC_HEARTBEAT     "grad/project/heartbeat"
#define TOPIC_STATUS        "grad/project/status"

// Reconnect
#define MQTT_RETRY_BASE_MS  1000
#define MQTT_RETRY_MAX_MS   30000

// ── Safety Thresholds ──────────────────────────────
// pH
#define PH_SAFE_MIN         6.5f
#define PH_SAFE_MAX         8.5f
#define PH_CRITICAL_MIN     6.0f
#define PH_CRITICAL_MAX     9.0f

// Turbidity (NTU)
#define TURBIDITY_SAFE      500.0f
#define TURBIDITY_CRITICAL  800.0f

// Flow Rate (L/min)
#define FLOW_NORMAL_MIN     5.0f
#define FLOW_CRITICAL_MIN   2.0f

// Temperature (°C)
#define TEMP_SAFE_MIN       5.0f
#define TEMP_SAFE_MAX       40.0f
#define TEMP_CRITICAL_MIN   2.0f
#define TEMP_CRITICAL_MAX   45.0f

// ── Timing (ms) ────────────────────────────────────
#define PUBLISH_INTERVAL    1000
#define DISPLAY_INTERVAL    250
#define HEARTBEAT_INTERVAL  10000
#define ALERT_COOLDOWN      5000
#define SENSOR_SAMPLES      5       // Number of readings to average
#define DEBOUNCE_MS         200     // Button debounce

// ── Watchdog ───────────────────────────────────────
#define WDT_TIMEOUT_S       30      // Watchdog timer (seconds)

// ── NTC Thermistor Calibration ─────────────────────
#define NTC_NOMINAL_R       10000.0f  // 10K at 25°C
#define NTC_NOMINAL_TEMP    25.0f
#define NTC_B_COEFFICIENT   3950.0f
#define NTC_SERIES_R        10000.0f  // Series resistor

#endif
