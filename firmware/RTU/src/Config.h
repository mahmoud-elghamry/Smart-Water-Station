#ifndef RTU_CONFIG_H
#define RTU_CONFIG_H

#include <Arduino.h>

// ============== Communication ==============
#define RTU_BAUD_RATE 115200
#define RTU_FRAME_INTERVAL_MS 200
#define RTU_FIRMWARE_VERSION "1.0.0"

// Blue Pill USART1 link to the ESP32-S3 MTU.
// STM32 PA9  TX -> ESP32-S3 PIN_RTU_RX
// STM32 PA10 RX <- ESP32-S3 PIN_RTU_TX
#define RTU_MTU_SERIAL Serial1
#define PIN_MTU_TX PA9
#define PIN_MTU_RX PA10

// ============== Sensor Pins ==============
// Blue Pill / generic STM32 starter pins.
// Update these once the final STM32 board and sensor wiring are fixed.
#define PIN_TDS PA0
#define PIN_PRESSURE PA1
#define PIN_FLOW PA2
#define PIN_WATER_LEVEL PA3
#define PIN_STATUS_LED PC13

// ============== ADC ==============
#define ADC_MAX_VALUE 4095.0f
#define ADC_SAMPLES 8

// ============== Sensor Fault Detection ==============
// ADC values near the rails indicate a disconnected or shorted sensor.
// Open circuit (floating input) typically reads near max.
// Short circuit reads near zero.
#define SENSOR_DISCONNECT_LOW 10    // Below this = possible short circuit
#define SENSOR_DISCONNECT_HIGH 4085 // Above this = possible open circuit / disconnected

// ============== Sensor Error Flags (bitmask) ==============
#define SENSOR_ERR_NONE     0x00
#define SENSOR_ERR_TDS      0x01
#define SENSOR_ERR_PRESSURE 0x02
#define SENSOR_ERR_FLOW     0x04
#define SENSOR_ERR_LEVEL    0x08

// ============== Watchdog ==============
// IWDG timeout in microseconds (4 seconds).
// If loop() doesn't feed the watchdog within this time, STM32 resets.
#define WDT_TIMEOUT_US 4000000

// ============== Send Buffer ==============
#define FRAME_BUFFER_SIZE 256

// ============== Command Reception ==============
#define CMD_BUFFER_SIZE 128

#endif // RTU_CONFIG_H
