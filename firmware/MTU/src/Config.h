#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============== Communication ==============
#define BAUD_RATE 115200
#define HEARTBEAT_INTERVAL 5000 // 5 seconds
#define TELEMETRY_INTERVAL 1000 // 1 second

// ============== Feature Flags ==============
// Keep the graduation-project build useful without pulling in heavy pieces too early.
#define TARGET_ESP32S3_N16R8 1
#define ENABLE_WEB_DASHBOARD 1
#define ENABLE_RTU_LINK 1
#define ENABLE_EDGE_IMPULSE 0
#define ENABLE_HMI_DISPLAY 0
#define ENABLE_DATA_FORWARDER 0 // 1 = output CSV for edge-impulse-data-forwarder

// ============== Simulation Mode ==============
// Set to true for testing without real sensors
#define SIMULATION_MODE true

// ============== WiFi Access Point ==============
#define WIFI_AP_SSID "AquaPuer-MTU"
#define WIFI_AP_PASSWORD "12345678"
#define WIFI_AP_CHANNEL 6
#define WIFI_AP_MAX_CLIENTS 4

// ============== Task Timing ==============
#define CONTROL_TASK_INTERVAL_MS 200
#define SERIAL_TASK_INTERVAL_MS 10
#define WEB_TASK_INTERVAL_MS 5
#define AI_TASK_INTERVAL_MS 2000
#define HMI_TASK_INTERVAL_MS 1000
#define RTU_TASK_INTERVAL_MS 10
#define DATA_FORWARDER_INTERVAL_MS 50 // 20 Hz for Edge Impulse data collection
#define RTU_TIMEOUT_MS 3000

// ============== Watchdog ==============
#define WDT_TIMEOUT_SECONDS 10

// ============== Pin Definitions ==============
// ESP32-S3 N16R8 note:
// Avoid GPIO26-GPIO37 on most N16R8 modules because flash/PSRAM can use them.
// Confirm these pins with your exact dev board before connecting power hardware.

// Analog Sensors
#define PIN_TDS 1
#define PIN_PRESSURE 2
#define PIN_FLOW 3
#define PIN_WATER_LEVEL 4

// Digital Outputs - FIXED: No more pin conflict!
#define PIN_PUMP 12           // Relay driver output
#define PIN_VALVE 13          // Valve driver output
#define PIN_LED_STATUS 15     // External status LED

// Optional HMI display bus
#define PIN_I2C_SDA 8
#define PIN_I2C_SCL 9

// Future RTU link to STM32
#define PIN_RTU_RX 17
#define PIN_RTU_TX 18

// ============== Safety Thresholds ==============
#define MAX_PRESSURE 100.0   // PSI
#define MIN_PRESSURE 5.0     // PSI
#define MAX_TDS 500.0        // PPM
#define MIN_TDS 50.0         // PPM
#define MAX_FLOW_RATE 50.0   // L/min
#define MIN_WATER_LEVEL 10.0 // %

// ============== Calibration Defaults ==============
#define TDS_OFFSET 0.0
#define PRESSURE_OFFSET 0.0
#define TDS_MULTIPLIER 1.0
#define PRESSURE_MULTIPLIER 1.0

// ============== System Constants ==============
#define BUFFER_SIZE 512
#define MAX_RETRY_COUNT 2
#define DEBOUNCE_DELAY 50
#define ADC_SAMPLES 10       // Number of samples for smoothing

// ============== Edge Impulse ==============
// Number of features sent per inference window.
// Must match the Edge Impulse project design (4 sensors: tds, pressure, flow, level).
#define EI_FEATURES_COUNT 4

// ============== NVS Persistent Storage ==============
// Saves calibration data so it survives power cycles.
#define ENABLE_NVS_STORAGE 1
#define NVS_NAMESPACE "aquapuer"

// ============== RTU Frame Validation ==============
// RTU sends -1.0 for sensors that are disconnected/shorted.
#define RTU_SENSOR_FAULT_VALUE -1.0f

// ============== Version ==============
#define FIRMWARE_VERSION "3.1.0"

#endif // CONFIG_H
