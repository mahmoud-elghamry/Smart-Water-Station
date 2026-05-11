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

// Snapshot of all sensor readings taken at the same time.
// Eliminates the double-read problem: read once, use everywhere.
struct SensorSnapshot {
  SensorReading tds;
  SensorReading pressure;
  SensorReading flow;
  SensorReading level;
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

  // Single-shot read of all sensors (preferred — avoids double reads)
  SensorSnapshot readAll();

  // Individual sensor readings (still available for calibration / debug)
  SensorReading readTDS();
  SensorReading readPressure();
  SensorReading readFlowRate();
  SensorReading readWaterLevel();

  // Safety checks using an already-captured snapshot (no re-read)
  ErrorCode checkSafety(const SensorSnapshot &snap);
  bool isCritical(ErrorCode err);

  // Calibration
  void setCalibration(CalibrationData cal);
  CalibrationData getCalibration();
  void calibrateTDS(float referenceValue);
  void calibratePressure(float referenceValue);

  // Persistent calibration (NVS)
  void loadCalibrationFromNVS();
  void saveCalibrationToNVS();

  // Simulation support
  void setPumpState(bool running);

private:
  CalibrationData _calibration;

  // Helper functions
  float readAnalogSmoothed(int pin, int samples = ADC_SAMPLES);
  bool validateReading(float value, float min, float max);
};

#endif // SENSOR_MANAGER_H
