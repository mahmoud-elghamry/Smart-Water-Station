#include "SensorManager.h"

#if ENABLE_NVS_STORAGE
#include <Preferences.h>
#endif

// ============== Simulation State (for realistic drifting) ==============
static bool _simInitialized = false;
static float _simTDS = 250.0;     // Starting TDS value
static float _simPressure = 45.0; // Starting pressure
static float _simFlow = 0.0;      // Starting flow
static float _simLevel = 75.0;    // Starting water level
static bool _pumpRunning = false;  // Track pump state for drift

void SensorManager::begin() {
  // Configure pins as inputs
  pinMode(PIN_TDS, INPUT);
  pinMode(PIN_PRESSURE, INPUT);
  pinMode(PIN_FLOW, INPUT);
  pinMode(PIN_WATER_LEVEL, INPUT);

  // Initialize calibration with defaults
  _calibration.tdsOffset = TDS_OFFSET;
  _calibration.tdsMultiplier = TDS_MULTIPLIER;
  _calibration.pressureOffset = PRESSURE_OFFSET;
  _calibration.pressureMultiplier = PRESSURE_MULTIPLIER;

  // Load saved calibration from NVS (overrides defaults if present)
  loadCalibrationFromNVS();

  // Initialize random seed for simulation
  if (!_simInitialized) {
    randomSeed(analogRead(0));
    _simInitialized = true;
  }
}

void SensorManager::setPumpState(bool running) { _pumpRunning = running; }

float SensorManager::readAnalogSmoothed(int pin, int samples) {
#if SIMULATION_MODE
  // ============== Realistic Drifting Simulation ==============
  // Values drift based on pump state, not just random noise

  float driftRate = 0.1;                      // How fast values change
  float noise = (random(-100, 100) / 1000.0); // ±0.1 noise

  if (pin == PIN_TDS) {
    // TDS increases when pump is running (mixing), decreases when idle
    if (_pumpRunning) {
      _simTDS += driftRate * 2.0 + noise;
    } else {
      _simTDS -= driftRate * 0.5 + noise;
    }
    // Clamp to realistic range
    _simTDS = constrain(_simTDS, 100.0, 450.0);
    return (_simTDS / 1000.0) * 4095.0; // Convert back to ADC-like value
  } else if (pin == PIN_PRESSURE) {
    // Pressure increases when pump runs, drops when idle
    if (_pumpRunning) {
      _simPressure += driftRate * 5.0 + noise;
    } else {
      _simPressure -= driftRate * 2.0 + noise;
    }
    _simPressure = constrain(_simPressure, 20.0, 90.0);
    return (_simPressure / 150.0) * 4095.0;
  } else if (pin == PIN_FLOW) {
    // Flow only exists when pump is running
    if (_pumpRunning) {
      _simFlow += driftRate * 3.0 + noise;
      _simFlow = constrain(_simFlow, 15.0, 35.0);
    } else {
      _simFlow -= driftRate * 5.0;
      _simFlow = constrain(_simFlow, 0.0, 35.0);
    }
    return (_simFlow / 100.0) * 4095.0;
  } else if (pin == PIN_WATER_LEVEL) {
    // Level drops when pump runs (draining), rises when idle (filling)
    if (_pumpRunning) {
      _simLevel -= driftRate * 0.5 + noise;
    } else {
      _simLevel += driftRate * 0.2 + noise;
    }
    _simLevel = constrain(_simLevel, 20.0, 95.0);
    return (_simLevel / 100.0) * 4095.0;
  }

  return 2048.0; // Default mid-range

#else
  // ============== Real ADC Reading (Non-Blocking) ==============
  // Using analogReadMilliVolts for better ESP32 linearity

  long sum = 0;
  for (int i = 0; i < samples; i++) {
    // analogReadMilliVolts returns 0-3300 (millivolts)
    sum += analogReadMilliVolts(pin);
    // NO delay - strictly non-blocking!
  }
  float avgMilliVolts = (float)sum / samples;

  // Convert millivolts to ADC-equivalent value (0-4095 scale)
  // 3300mV = 4095, so: adc = (mV / 3300) * 4095
  return (avgMilliVolts / 3300.0) * 4095.0;
#endif
}

bool SensorManager::validateReading(float value, float min, float max) {
  return (value >= min && value <= max);
}

SensorSnapshot SensorManager::readAll() {
  SensorSnapshot snap;
  snap.tds = readTDS();
  snap.pressure = readPressure();
  snap.flow = readFlowRate();
  snap.level = readWaterLevel();
  return snap;
}

SensorReading SensorManager::readTDS() {
  SensorReading result;
  result.timestamp = millis();

  float raw = readAnalogSmoothed(PIN_TDS);
  result.value = (raw / 4095.0) * 1000.0; // Scale to 0-1000 PPM

  // Apply calibration
  result.value =
      (result.value + _calibration.tdsOffset) * _calibration.tdsMultiplier;

  // Validate
  if (result.value > MAX_TDS) {
    result.isValid = false;
    result.errorCode = ERR_TDS_HIGH;
  } else if (result.value < MIN_TDS) {
    result.isValid = false;
    result.errorCode = ERR_TDS_LOW;
  } else {
    result.isValid = true;
    result.errorCode = ERR_NONE;
  }

  return result;
}

SensorReading SensorManager::readPressure() {
  SensorReading result;
  result.timestamp = millis();

  float raw = readAnalogSmoothed(PIN_PRESSURE);
  result.value = (raw / 4095.0) * 150.0; // Scale to 0-150 PSI

  // Apply calibration
  result.value = (result.value + _calibration.pressureOffset) *
                 _calibration.pressureMultiplier;

  if (result.value > MAX_PRESSURE) {
    result.isValid = false;
    result.errorCode = ERR_PRESSURE_HIGH;
  } else if (result.value < MIN_PRESSURE) {
    result.isValid = false;
    result.errorCode = ERR_PRESSURE_LOW;
  } else {
    result.isValid = true;
    result.errorCode = ERR_NONE;
  }

  return result;
}

SensorReading SensorManager::readFlowRate() {
  SensorReading result;
  result.timestamp = millis();

  float raw = readAnalogSmoothed(PIN_FLOW);
  result.value = (raw / 4095.0) * 100.0; // Scale to 0-100 L/min

  result.isValid = (result.value <= MAX_FLOW_RATE);
  result.errorCode = result.isValid ? ERR_NONE : ERR_SENSOR_FAILURE;

  return result;
}

SensorReading SensorManager::readWaterLevel() {
  SensorReading result;
  result.timestamp = millis();

  float raw = readAnalogSmoothed(PIN_WATER_LEVEL);
  result.value = (raw / 4095.0) * 100.0; // Scale to 0-100%

  if (result.value < MIN_WATER_LEVEL) {
    result.isValid = false;
    result.errorCode = ERR_WATER_LEVEL_LOW;
  } else {
    result.isValid = true;
    result.errorCode = ERR_NONE;
  }

  return result;
}

ErrorCode SensorManager::checkSafety(const SensorSnapshot &snap) {
  // Use the already-captured readings instead of re-reading sensors
  if (!snap.tds.isValid)
    return snap.tds.errorCode;

  if (!snap.pressure.isValid)
    return snap.pressure.errorCode;

  if (!snap.level.isValid)
    return snap.level.errorCode;

  return ERR_NONE;
}

bool SensorManager::isCritical(ErrorCode err) {
  return (err == ERR_PRESSURE_HIGH || err == ERR_WATER_LEVEL_LOW);
}

void SensorManager::setCalibration(CalibrationData cal) { _calibration = cal; }

CalibrationData SensorManager::getCalibration() { return _calibration; }

void SensorManager::calibrateTDS(float referenceValue) {
  float currentRaw = readAnalogSmoothed(PIN_TDS);
  float currentValue = (currentRaw / 4095.0) * 1000.0;

  if (currentValue > 0) {
    _calibration.tdsMultiplier = referenceValue / currentValue;
    saveCalibrationToNVS();
  }
}

void SensorManager::calibratePressure(float referenceValue) {
  float currentRaw = readAnalogSmoothed(PIN_PRESSURE);
  float currentValue = (currentRaw / 4095.0) * 150.0;

  if (currentValue > 0) {
    _calibration.pressureMultiplier = referenceValue / currentValue;
    saveCalibrationToNVS();
  }
}

void SensorManager::loadCalibrationFromNVS() {
#if ENABLE_NVS_STORAGE
  Preferences prefs;
  if (prefs.begin(NVS_NAMESPACE, true)) // read-only
  {
    _calibration.tdsOffset = prefs.getFloat("tdsOff", TDS_OFFSET);
    _calibration.tdsMultiplier = prefs.getFloat("tdsMul", TDS_MULTIPLIER);
    _calibration.pressureOffset = prefs.getFloat("presOff", PRESSURE_OFFSET);
    _calibration.pressureMultiplier = prefs.getFloat("presMul", PRESSURE_MULTIPLIER);
    prefs.end();
  }
#endif
}

void SensorManager::saveCalibrationToNVS() {
#if ENABLE_NVS_STORAGE
  Preferences prefs;
  if (prefs.begin(NVS_NAMESPACE, false)) // read-write
  {
    prefs.putFloat("tdsOff", _calibration.tdsOffset);
    prefs.putFloat("tdsMul", _calibration.tdsMultiplier);
    prefs.putFloat("presOff", _calibration.pressureOffset);
    prefs.putFloat("presMul", _calibration.pressureMultiplier);
    prefs.end();
  }
#endif
}

