#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "Config.h"
#include "StateMachine.h"

// Sensor reading result structure
struct SensorReading {
  float value = 0.0;
  bool isValid = false;
  ErrorCode errorCode = ERR_NONE;
  unsigned long timestamp = 0;
};

// Calibration data structure
struct CalibrationData {
  float tdsOffset = 0.0;
  float tdsMultiplier = 1.0;
  float pressureOffset = 0.0;
  float pressureMultiplier = 1.0;
};

class SensorManager {
public:
  void begin();

  // Sensor readings
  SensorReading readTDS();
  SensorReading readPressure();
  SensorReading readFlowRate();
  SensorReading readWaterLevel();

  // Safety checks
  ErrorCode checkSafety();
  bool isCritical();

  // Calibration
  void setCalibration(CalibrationData cal);
  CalibrationData getCalibration();
  void calibrateTDS(float referenceValue);
  void calibratePressure(float referenceValue);

  // Simulation support
  void setPumpState(bool running);

private:
  CalibrationData _calibration;

  // Helper functions
  float readAnalogSmoothed(int pin, int samples = ADC_SAMPLES);
  bool validateReading(float value, float min, float max);
};

#endif // SENSOR_MANAGER_H
