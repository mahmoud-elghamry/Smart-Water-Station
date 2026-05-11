/**
 * Smart Water Station RTU  v1.0.0
 *
 * STM32 Blue Pill firmware for field sensor acquisition.
 * Reads 4 analog sensors, detects faults, appends CRC8,
 * and sends JSON frames to MTU via UART.
 *
 * Features:
 *   - 8-sample ADC smoothing
 *   - Sensor disconnect/short detection
 *   - CRC8 integrity check on every frame
 *   - Single-buffer frame assembly (efficient UART)
 *   - IWDG hardware watchdog (4-second timeout)
 *   - Bidirectional: receives commands from MTU
 */

#include "Config.h"
#include "SensorReader.h"
#include <IWatchdog.h>

SensorReader sensors;
uint32_t lastFrameTime = 0;

// ============== Command Reception Buffer ==============
static char cmdBuffer[CMD_BUFFER_SIZE];
static uint16_t cmdLen = 0;

// ============== CRC8 (polynomial 0x07) ==============
// Same implementation on RTU and MTU for frame integrity.
uint8_t crc8(const char *data, uint16_t len)
{
  uint8_t crc = 0x00;
  for (uint16_t i = 0; i < len; i++)
  {
    crc ^= (uint8_t)data[i];
    for (uint8_t j = 0; j < 8; j++)
    {
      if (crc & 0x80)
        crc = (crc << 1) ^ 0x07;
      else
        crc <<= 1;
    }
  }
  return crc;
}

// ============== Send Boot Info ==============
void sendBootInfo()
{
  char buf[FRAME_BUFFER_SIZE];
  int len = snprintf(buf, sizeof(buf),
                     "{\"type\":\"rtu_info\",\"firmware\":\"%s\",\"baud\":%d",
                     RTU_FIRMWARE_VERSION, RTU_BAUD_RATE);
  uint8_t crc = crc8(buf, len);
  snprintf(buf + len, sizeof(buf) - len, ",\"crc\":%u}", crc);

  RTU_MTU_SERIAL.println(buf);
}

// ============== Send Sensor Frame ==============
// Single-buffer assembly — one UART write instead of 14.
void sendFrame(const RtuSensorFrame &frame)
{
  char buf[FRAME_BUFFER_SIZE];

  // Build the payload (everything before the CRC)
  int len = snprintf(buf, sizeof(buf),
                     "{\"type\":\"rtu_frame\",\"seq\":%lu,"
                     "\"tds\":%.2f,\"pressure\":%.2f,\"flow\":%.2f,\"level\":%.2f,"
                     "\"err\":%u,\"ts\":%lu",
                     (unsigned long)frame.seq,
                     frame.tds, frame.pressure, frame.flow, frame.level,
                     frame.errorFlags,
                     (unsigned long)frame.timestamp);

  // Append CRC8 computed over the payload so far
  uint8_t crc = crc8(buf, len);
  snprintf(buf + len, sizeof(buf) - len, ",\"crc\":%u}", crc);

  RTU_MTU_SERIAL.println(buf);
}

// ============== Process Command from MTU ==============
void processCommand(const char *json, uint16_t len)
{
  // Simple command parsing without ArduinoJson to keep RTU lightweight.
  // Expected format: {"cmd":"SET_INTERVAL","value":100}

  if (strstr(json, "\"SET_INTERVAL\""))
  {
    // Find value field
    const char *vp = strstr(json, "\"value\":");
    if (vp)
    {
      int newInterval = atoi(vp + 8);
      if (newInterval >= 50 && newInterval <= 5000)
      {
        // Note: RTU_FRAME_INTERVAL_MS is a #define, so we use a global
        // variable for runtime override. See loop().
        extern volatile uint32_t runtimeInterval;
        runtimeInterval = (uint32_t)newInterval;
      }
    }
  }

  // Acknowledge
  RTU_MTU_SERIAL.println("{\"type\":\"rtu_ack\",\"status\":\"ok\"}");
}

// ============== Check for MTU Commands ==============
void checkForCommands()
{
  while (RTU_MTU_SERIAL.available())
  {
    char c = RTU_MTU_SERIAL.read();

    if (cmdLen >= CMD_BUFFER_SIZE - 1)
    {
      cmdLen = 0; // Overflow protection
      continue;
    }

    cmdBuffer[cmdLen++] = c;
    cmdBuffer[cmdLen] = '\0';

    if (c == '}')
    {
      processCommand(cmdBuffer, cmdLen);
      cmdLen = 0;
    }
  }
}

// ============== Runtime-adjustable interval ==============
volatile uint32_t runtimeInterval = RTU_FRAME_INTERVAL_MS;

// ============== Setup ==============
void setup()
{
  RTU_MTU_SERIAL.begin(RTU_BAUD_RATE);
  delay(100);

  pinMode(PIN_STATUS_LED, OUTPUT);
  digitalWrite(PIN_STATUS_LED, HIGH);

  analogReadResolution(12);
  sensors.begin();

  // Initialize hardware watchdog (IWDG) — resets STM32 if not fed within timeout
  IWatchdog.begin(WDT_TIMEOUT_US);

  sendBootInfo();
}

// ============== Main Loop ==============
void loop()
{
  // Feed the watchdog — must be called every iteration
  IWatchdog.reload();

  // Check for incoming commands from MTU
  checkForCommands();

  const uint32_t now = millis();

  if (now - lastFrameTime >= runtimeInterval)
  {
    RtuSensorFrame frame = sensors.readFrame();
    sendFrame(frame);

    // Blink LED — fast if sensor error, normal otherwise
    if (frame.errorFlags != SENSOR_ERR_NONE)
    {
      // Fast blink on error
      digitalWrite(PIN_STATUS_LED, (millis() / 100) % 2);
    }
    else
    {
      // Normal toggle
      digitalWrite(PIN_STATUS_LED, !digitalRead(PIN_STATUS_LED));
    }

    lastFrameTime = now;
  }
}
