#ifndef SENSOR_READER_H
#define SENSOR_READER_H

#include "Config.h"

struct RtuSensorFrame
{
  uint32_t seq = 0;
  uint16_t tdsRaw = 0;
  uint16_t pressureRaw = 0;
  uint16_t flowRaw = 0;
  uint16_t levelRaw = 0;
  float tds = 0.0f;
  float pressure = 0.0f;
  float flow = 0.0f;
  float level = 0.0f;
  uint8_t errorFlags = SENSOR_ERR_NONE; // Bitmask of sensor faults
  uint32_t timestamp = 0;
};

class SensorReader
{
public:
  void begin();
  RtuSensorFrame readFrame();

private:
  uint32_t _seq = 0;

  uint16_t readSmoothed(uint32_t pin);
  bool isSensorFault(uint16_t raw);
};

#endif // SENSOR_READER_H
