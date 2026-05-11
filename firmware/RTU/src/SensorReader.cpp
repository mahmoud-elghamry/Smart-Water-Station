#include "SensorReader.h"

void SensorReader::begin()
{
  pinMode(PIN_TDS, INPUT_ANALOG);
  pinMode(PIN_PRESSURE, INPUT_ANALOG);
  pinMode(PIN_FLOW, INPUT_ANALOG);
  pinMode(PIN_WATER_LEVEL, INPUT_ANALOG);
}

uint16_t SensorReader::readSmoothed(uint32_t pin)
{
  uint32_t sum = 0;

  for (uint8_t i = 0; i < ADC_SAMPLES; i++)
  {
    sum += analogRead(pin);
  }

  return static_cast<uint16_t>(sum / ADC_SAMPLES);
}

bool SensorReader::isSensorFault(uint16_t raw)
{
  // ADC values near the rails indicate a hardware problem:
  //   - Near 0: short circuit to ground
  //   - Near 4095: open circuit / disconnected sensor
  return (raw <= SENSOR_DISCONNECT_LOW || raw >= SENSOR_DISCONNECT_HIGH);
}

RtuSensorFrame SensorReader::readFrame()
{
  RtuSensorFrame frame;
  frame.seq = _seq++;
  frame.timestamp = millis();
  frame.errorFlags = SENSOR_ERR_NONE;

  // Read raw values
  frame.tdsRaw = readSmoothed(PIN_TDS);
  frame.pressureRaw = readSmoothed(PIN_PRESSURE);
  frame.flowRaw = readSmoothed(PIN_FLOW);
  frame.levelRaw = readSmoothed(PIN_WATER_LEVEL);

  // Check for sensor faults (disconnected / shorted)
  if (isSensorFault(frame.tdsRaw))
    frame.errorFlags |= SENSOR_ERR_TDS;
  if (isSensorFault(frame.pressureRaw))
    frame.errorFlags |= SENSOR_ERR_PRESSURE;
  if (isSensorFault(frame.flowRaw))
    frame.errorFlags |= SENSOR_ERR_FLOW;
  if (isSensorFault(frame.levelRaw))
    frame.errorFlags |= SENSOR_ERR_LEVEL;

  // Convert to engineering units (only if sensor is healthy)
  frame.tds = (frame.errorFlags & SENSOR_ERR_TDS)
                  ? -1.0f
                  : (frame.tdsRaw / ADC_MAX_VALUE) * 1000.0f;
  frame.pressure = (frame.errorFlags & SENSOR_ERR_PRESSURE)
                       ? -1.0f
                       : (frame.pressureRaw / ADC_MAX_VALUE) * 150.0f;
  frame.flow = (frame.errorFlags & SENSOR_ERR_FLOW)
                   ? -1.0f
                   : (frame.flowRaw / ADC_MAX_VALUE) * 100.0f;
  frame.level = (frame.errorFlags & SENSOR_ERR_LEVEL)
                    ? -1.0f
                    : (frame.levelRaw / ADC_MAX_VALUE) * 100.0f;

  return frame;
}
