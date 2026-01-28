/**
 * Configuration - Pin definitions and settings
 */

#ifndef CONFIG_H
#define CONFIG_H

// Pins
#define PH_SENSOR_PIN       34
#define TURBIDITY_LDR_PIN   35
#define ULTRASONIC_TRIG     32
#define ULTRASONIC_ECHO     33
#define PUMP_RELAY_PIN      2
#define I2C_SDA             21
#define I2C_SCL             22

// OLED
#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT       64
#define OLED_ADDRESS        0x3C

// WiFi
#define WIFI_SSID           "Wokwi-GUEST"
#define WIFI_PASSWORD       ""

// MQTT
#define MQTT_BROKER         "broker.hivemq.com"
#define MQTT_PORT           1883
#define TOPIC_SENSORS       "grad/project/sensors"
#define TOPIC_ALERTS        "grad/project/alerts"
#define TOPIC_CONTROL       "grad/project/control"

// Safety thresholds
#define PH_SAFE_MIN         6.5f
#define PH_SAFE_MAX         8.5f
#define PH_CRITICAL_MIN     6.0f
#define PH_CRITICAL_MAX     9.0f
#define TURBIDITY_SAFE      500.0f
#define TURBIDITY_CRITICAL  800.0f
#define FLOW_NORMAL_MIN     5.0f
#define FLOW_CRITICAL_MIN   2.0f

// Timing (ms)
#define PUBLISH_INTERVAL    1000
#define DISPLAY_INTERVAL    250
#define ALERT_COOLDOWN      5000

#endif
