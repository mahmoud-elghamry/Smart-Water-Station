#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "Config.h"
#include "StateMachine.h"

// Single sensor reading
struct SensorReading {
  float value = 0.0;
  bool isValid = false;
  ErrorCode errorCode = ERR_NONE;
  unsigned long timestamp = 0;
};

// All 8 sensors in one snapshot (read once, use everywhere)
struct SensorSnapshot {
  SensorReading turb1;      // Turbidity before filter (%)
  SensorReading turb2;      // Turbidity after filter (%)
  SensorReading ph1;        // pH before filter
  SensorReading ph2;        // pH after filter
  SensorReading flow1;      // Flow rate input (L/min)
  SensorReading flow2;      // Flow rate output (L/min)
  SensorReading pressure1;  // Pressure input (bar)
  SensorReading pressure2;  // Pressure output (bar)
  SensorReading temp1;      // Temperature before filter (°C)
  SensorReading temp2;      // Temperature after filter (°C)
  SensorReading pumpCurrent; // Pump current (A), optional
};

// Calibration data
struct CalibrationData {
  float turbCleanADC = TURB_CLEAN_ADC;
  float turbDirtyADC = TURB_DIRTY_ADC;
  float phNeutralVoltage = PH_NEUTRAL_VOLTAGE;
  float phSlope = PH_SLOPE;
  float pressZeroVoltage1 = PRESS_ZERO_VOLTAGE_1;
  float pressZeroVoltage2 = PRESS_ZERO_VOLTAGE_2;
  float flowCalFactor = FLOW_CALIBRATION_FACTOR;
};

class SensorManager {
public:
  void begin();

  // Read all 8 sensors at once
  SensorSnapshot readAll();

  // Individual reads
  SensorReading readTurbidity(int pin);
  SensorReading readPH(int pin);
  SensorReading readPressure(int pin, float zeroVoltage);
  SensorReading readFlowRate(int channel);  // 0 = flow1, 1 = flow2
  SensorReading readTemperature(int channel); // 0 = temp1, 1 = temp2
  SensorReading readPumpCurrent();

  // Safety
  ErrorCode checkSafety(const SensorSnapshot &snap);
  bool isCritical(ErrorCode err);

  // Calibration
  void setCalibration(CalibrationData cal);
  CalibrationData getCalibration();
  void loadCalibrationFromNVS();
  void saveCalibrationToNVS();

  // Simulation
  void setPumpState(bool running);

  // Flow interrupt counters (public for ISR access)
  static volatile long pulseCount1;
  static volatile long pulseCount2;

private:
  CalibrationData _cal;
  unsigned long _lastFlowTime = 0;
  float _lastFlow1 = 0;
  float _lastFlow2 = 0;

  // EMA filters for analog smoothing
  float _turbFiltered1 = 0;
  float _turbFiltered2 = 0;
  float _phFiltered1 = 0;
  float _phFiltered2 = 0;

  float readAnalogSmoothed(int pin, int samples = ADC_SAMPLES);
  bool validateReading(float value, float min, float max);
};

#endif // SENSOR_MANAGER_H
