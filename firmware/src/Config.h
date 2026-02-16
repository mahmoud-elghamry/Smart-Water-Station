#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============== Communication ==============
#define BAUD_RATE 115200
#define HEARTBEAT_INTERVAL 5000 // 5 seconds
#define TELEMETRY_INTERVAL 1000 // 1 second

// ============== Simulation Mode ==============
// Set to true for testing without real sensors
#define SIMULATION_MODE true

// ============== Pin Definitions ==============
// Analog Sensors
#define PIN_TDS 34
#define PIN_PRESSURE 35
#define PIN_FLOW 32
#define PIN_WATER_LEVEL 33

// Digital Outputs - FIXED: No more pin conflict!
#define PIN_PUMP 27           // GPIO27 for pump relay
#define PIN_VALVE 26          // GPIO26 for valve
#define PIN_LED_STATUS 2      // Built-in LED for status indication

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
#define MAX_RETRY_COUNT 3
#define DEBOUNCE_DELAY 50
#define ADC_SAMPLES 10       // Number of samples for smoothing

// ============== Version ==============
#define FIRMWARE_VERSION "2.1.0"

#endif // CONFIG_H
