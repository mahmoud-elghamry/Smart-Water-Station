#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============== Communication ==============
#define BAUD_RATE 115200
#define HEARTBEAT_INTERVAL 5000 // 5 seconds
#define TELEMETRY_INTERVAL 1000 // 1 second

// ============== Feature Flags ==============
#define TARGET_ESP32S3_N16R8 1
#define ENABLE_WEB_DASHBOARD 1
#define ENABLE_RTU_LINK 1
#define ENABLE_EDGE_IMPULSE 0
#define ENABLE_HMI_DISPLAY 0
#define ENABLE_DATA_FORWARDER 0
#define ENABLE_TEMPERATURE_SENSORS 0
#define ENABLE_PUMP_CURRENT_SENSOR 0

// ============== Simulation Mode ==============
// Set to false for real hardware!
#define SIMULATION_MODE false

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
#define DATA_FORWARDER_INTERVAL_MS 50
#define RTU_TIMEOUT_MS 3000

// ============== Watchdog ==============
#define WDT_TIMEOUT_SECONDS 10

// ═══════════════════════════════════════════════════════════════
//  Pin Definitions — ESP32-S3 N16R8
//  ADC1 = GPIO 1-10  |  Avoid GPIO 26-37 (Flash/PSRAM)
//  Just change the numbers here and recompile!
// ═══════════════════════════════════════════════════════════════

// ── Analog Sensors (ADC1: GPIO 1-10) ──
#define PIN_TURB1    4   // Turbidity sensor 1 (before filter)
#define PIN_TURB2    5   // Turbidity sensor 2 (after filter)
#define PIN_PH1      6   // pH sensor 1 (before filter)
#define PIN_PH2      7   // pH sensor 2 (after filter)
#define PIN_PRESS1   8   // Pressure sensor 1 (input)
#define PIN_PRESS2   9   // Pressure sensor 2 (output)
#define PIN_PUMP_CURRENT 2 // Set to the real ADC pin when enabled

// ── DS18B20 Temperature Sensors (1-Wire, any digital GPIO) ──
#define PIN_TEMP1    11  // Temperature sensor 1 (before filter)
#define PIN_TEMP2    21  // Temperature sensor 2 (after filter)

// ── Digital Sensors (Interrupt-capable) ──
#define PIN_FLOW1    14  // Flow sensor 1 (input) - pulse counter
#define PIN_FLOW2    13  // Flow sensor 2 (output) - pulse counter

// ── Digital Outputs ──
#define PIN_PUMP     12  // Relay: pump
#define PIN_VALVE    16  // Relay: valve
#define PIN_LED_STATUS 15 // Status LED

// ── I2C (LCD display) ──
#define PIN_I2C_SDA  18
#define PIN_I2C_SCL  17

// ── RTU UART (to STM32) ──
// Keep 17/18 for I2C LCD. Avoid 19/20 because they are commonly used by USB.
#define PIN_RTU_RX   39
#define PIN_RTU_TX   40

// ═══════════════════════════════════════════════════════════════
//  Sensor Calibration — adjust based on your actual sensors
// ═══════════════════════════════════════════════════════════════

// Turbidity: ADC reading for clean vs dirty water
#define TURB_CLEAN_ADC   3465   // ADC when water is clean (0%)
#define TURB_DIRTY_ADC   2800   // ADC when water is dirty (100%)

// pH: voltage at pH 7 and slope
#define PH_NEUTRAL_VOLTAGE  2.44  // Voltage at pH 7.0
#define PH_SLOPE            0.18  // Voltage change per pH unit

// Pressure: 0-3.5 bar sensors with per-sensor zero offsets
#define PRESS_ZERO_VOLTAGE_1  0.50  // Voltage at 0 bar for pressure sensor 1
#define PRESS_ZERO_VOLTAGE_2  0.55  // Voltage at 0 bar for pressure sensor 2
#define PRESS_ZERO_VOLTAGE    PRESS_ZERO_VOLTAGE_1
#define PRESS_MAX_VOLTAGE   3.30  // Voltage at max bar
#define PRESS_MAX_BAR       3.5   // Maximum bar reading

// Flow: pulse-based sensor calibration
#define FLOW_CALIBRATION_FACTOR  7.5  // Pulses per liter per minute

// Pump current sensor calibration.
// current = max(0, raw_adc - zero_adc) * amps_per_adc
#define PUMP_CURRENT_ZERO_ADC     2048.0
#define PUMP_CURRENT_AMPS_PER_ADC 0.01

// ============== Safety Thresholds ==============
#define MAX_TURBIDITY  80.0  // %
#define MAX_PH         9.0
#define MIN_PH         5.0
#define MAX_PRESSURE   3.5   // bar
#define MIN_PRESSURE   0.1   // bar
#define MAX_FLOW_RATE  50.0  // L/min
#define MAX_TEMPERATURE 45.0 // °C
#define MIN_TEMPERATURE 5.0  // °C
#define MAX_PUMP_CURRENT 8.0 // A

// ============== System Constants ==============
#define BUFFER_SIZE 512
#define MAX_RETRY_COUNT 2
#define DEBOUNCE_DELAY 50
#define ADC_SAMPLES 10

// ============== Edge Impulse ==============
#define EI_FEATURES_COUNT 11 // 10 water sensors + optional pump_current

// ============== NVS Persistent Storage ==============
#define ENABLE_NVS_STORAGE 1
#define NVS_NAMESPACE "aquapuer"

// ============== RTU Frame Validation ==============
#define RTU_SENSOR_FAULT_VALUE -1.0f

// ============== Version ==============
#define FIRMWARE_VERSION "3.2.0"

#endif // CONFIG_H
