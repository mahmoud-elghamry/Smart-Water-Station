#include "SensorManager.h"

#if ENABLE_NVS_STORAGE
#include <Preferences.h>
#endif

#include <OneWire.h>
#include <DallasTemperature.h>

// ── Static ISR counters ──
volatile long SensorManager::pulseCount1 = 0;
volatile long SensorManager::pulseCount2 = 0;

// ── ISR handlers ──
void IRAM_ATTR flowISR1() { SensorManager::pulseCount1++; }
void IRAM_ATTR flowISR2() { SensorManager::pulseCount2++; }

// ── Simulation state ──
static bool _simInit = false;
static float _simTurb1 = 25.0, _simTurb2 = 10.0;
static float _simPH1 = 7.1, _simPH2 = 7.3;
static float _simFlow1 = 0.0, _simFlow2 = 0.0;
static float _simPress1 = 1.2, _simPress2 = 0.8;
static float _simTemp1 = 22.0, _simTemp2 = 23.5;
static float _simPumpCurrent = 0.0;
static bool _pumpRunning = false;

// DS18B20 1-Wire sensors
static OneWire oneWire1(PIN_TEMP1);
static OneWire oneWire2(PIN_TEMP2);
static DallasTemperature tempSensor1(&oneWire1);
static DallasTemperature tempSensor2(&oneWire2);

void SensorManager::begin() {
  // Analog inputs
  pinMode(PIN_TURB1, INPUT);
  pinMode(PIN_TURB2, INPUT);
  pinMode(PIN_PH1, INPUT);
  pinMode(PIN_PH2, INPUT);
  pinMode(PIN_PRESS1, INPUT);
  pinMode(PIN_PRESS2, INPUT);
#if ENABLE_PUMP_CURRENT_SENSOR
  pinMode(PIN_PUMP_CURRENT, INPUT);
#endif

  // Flow sensors — digital with pullup + interrupt
  pinMode(PIN_FLOW1, INPUT_PULLUP);
  pinMode(PIN_FLOW2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_FLOW1), flowISR1, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_FLOW2), flowISR2, FALLING);

  // ADC config
  analogSetAttenuation(ADC_11db);
  analogReadResolution(12);

  // Load calibration
  loadCalibrationFromNVS();
  _lastFlowTime = millis();

#if ENABLE_TEMPERATURE_SENSORS
  // DS18B20 temperature sensors
  tempSensor1.begin();
  tempSensor2.begin();
  tempSensor1.setResolution(12);
  tempSensor2.setResolution(12);
#endif

  if (!_simInit) {
    randomSeed(analogRead(0));
    _simInit = true;
  }
}

void SensorManager::setPumpState(bool running) { _pumpRunning = running; }

// ══════════════════════════════════════════════════════════════
//  Analog reading (real or simulated)
// ══════════════════════════════════════════════════════════════

float SensorManager::readAnalogSmoothed(int pin, int samples) {
#if SIMULATION_MODE
  float noise = (random(-100, 100) / 1000.0);
  float drift = 0.1;

  if (pin == PIN_TURB1) {
    _simTurb1 += (_pumpRunning ? -drift : drift * 0.3) + noise;
    _simTurb1 = constrain(_simTurb1, 5.0, 85.0);
    // Return ADC value that maps to turbidity %
    return map(_simTurb1, 0, 100, TURB_CLEAN_ADC, TURB_DIRTY_ADC);
  }
  if (pin == PIN_TURB2) {
    _simTurb2 += (_pumpRunning ? -drift * 0.5 : drift * 0.2) + noise;
    _simTurb2 = constrain(_simTurb2, 2.0, 60.0);
    return map(_simTurb2, 0, 100, TURB_CLEAN_ADC, TURB_DIRTY_ADC);
  }
  if (pin == PIN_PH1) {
    _simPH1 += noise * 0.05;
    _simPH1 = constrain(_simPH1, 5.5, 8.5);
    float v = PH_NEUTRAL_VOLTAGE - ((_simPH1 - 7.0) * PH_SLOPE);
    return (v / 3.3) * 4095.0;
  }
  if (pin == PIN_PH2) {
    _simPH2 += noise * 0.05;
    _simPH2 = constrain(_simPH2, 6.0, 8.0);
    float v = PH_NEUTRAL_VOLTAGE - ((_simPH2 - 7.0) * PH_SLOPE);
    return (v / 3.3) * 4095.0;
  }
  if (pin == PIN_PRESS1) {
    _simPress1 += (_pumpRunning ? drift * 2 : -drift) + noise;
    _simPress1 = constrain(_simPress1, 0.2, 3.0);
    float v = PRESS_ZERO_VOLTAGE_1 + (_simPress1 / PRESS_MAX_BAR) * (PRESS_MAX_VOLTAGE - PRESS_ZERO_VOLTAGE_1);
    return (v / 3.3) * 4095.0;
  }
  if (pin == PIN_PRESS2) {
    _simPress2 += (_pumpRunning ? drift * 1.5 : -drift * 0.5) + noise;
    _simPress2 = constrain(_simPress2, 0.1, 2.5);
    float v = PRESS_ZERO_VOLTAGE_2 + (_simPress2 / PRESS_MAX_BAR) * (PRESS_MAX_VOLTAGE - PRESS_ZERO_VOLTAGE_2);
    return (v / 3.3) * 4095.0;
  }
  return 2048.0;

#else
  // Real ADC reading — non-blocking, averaged
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
  }
  return (float)sum / samples;
#endif
}

bool SensorManager::validateReading(float value, float min, float max) {
  return (value >= min && value <= max);
}

// ══════════════════════════════════════════════════════════════
//  Read all 8 sensors
// ══════════════════════════════════════════════════════════════

SensorSnapshot SensorManager::readAll() {
  SensorSnapshot snap;
  snap.turb1 = readTurbidity(PIN_TURB1);
  snap.turb2 = readTurbidity(PIN_TURB2);
  snap.ph1 = readPH(PIN_PH1);
  snap.ph2 = readPH(PIN_PH2);
  snap.pressure1 = readPressure(PIN_PRESS1, _cal.pressZeroVoltage1);
  snap.pressure2 = readPressure(PIN_PRESS2, _cal.pressZeroVoltage2);
  snap.flow1 = readFlowRate(0);
  snap.flow2 = readFlowRate(1);
  snap.temp1 = readTemperature(0);
  snap.temp2 = readTemperature(1);
  snap.pumpCurrent = readPumpCurrent();
  return snap;
}

// ══════════════════════════════════════════════════════════════
//  Turbidity — ADC mapped to 0-100%
// ══════════════════════════════════════════════════════════════

SensorReading SensorManager::readTurbidity(int pin) {
  SensorReading r;
  r.timestamp = millis();

  float raw = readAnalogSmoothed(pin);

  // EMA filter (same as engineer's code: 60% old + 40% new)
  if (pin == PIN_TURB1) {
    _turbFiltered1 = (0.6 * _turbFiltered1) + (0.4 * raw);
    raw = _turbFiltered1;
  } else {
    _turbFiltered2 = (0.6 * _turbFiltered2) + (0.4 * raw);
    raw = _turbFiltered2;
  }

  r.value = map(raw, _cal.turbCleanADC, _cal.turbDirtyADC, 0, 100);
  r.value = constrain(r.value, 0, 100);

  if (r.value > MAX_TURBIDITY) {
    r.isValid = false;
    r.errorCode = ERR_TDS_HIGH;  // Reuse error code for turbidity
  } else {
    r.isValid = true;
    r.errorCode = ERR_NONE;
  }
  return r;
}

// ══════════════════════════════════════════════════════════════
//  pH — voltage to pH conversion
// ══════════════════════════════════════════════════════════════

SensorReading SensorManager::readPH(int pin) {
  SensorReading r;
  r.timestamp = millis();

  float raw = readAnalogSmoothed(pin);
  float voltage = raw * (3.3 / 4095.0);

  // EMA filter
  if (pin == PIN_PH1) {
    _phFiltered1 = (0.6 * _phFiltered1) + (0.4 * voltage);
    voltage = _phFiltered1;
  } else {
    _phFiltered2 = (0.6 * _phFiltered2) + (0.4 * voltage);
    voltage = _phFiltered2;
  }

  // pH = 7 + ((neutralVoltage - measuredVoltage) / slope)
  r.value = 7.0 + ((_cal.phNeutralVoltage - voltage) / _cal.phSlope);

  if (r.value > MAX_PH || r.value < MIN_PH) {
    r.isValid = false;
    r.errorCode = ERR_SENSOR_FAILURE;
  } else {
    r.isValid = true;
    r.errorCode = ERR_NONE;
  }
  return r;
}

// ══════════════════════════════════════════════════════════════
//  Pressure — voltage to bar
// ══════════════════════════════════════════════════════════════

SensorReading SensorManager::readPressure(int pin, float zeroVoltage) {
  SensorReading r;
  r.timestamp = millis();

  float raw = readAnalogSmoothed(pin);
  float voltage = raw * (3.3 / 4095.0);
  r.value = (voltage - zeroVoltage) * (PRESS_MAX_BAR / (PRESS_MAX_VOLTAGE - zeroVoltage));
  if (r.value < 0) r.value = 0;

  if (r.value > MAX_PRESSURE) {
    r.isValid = false;
    r.errorCode = ERR_PRESSURE_HIGH;
  } else {
    r.isValid = true;
    r.errorCode = ERR_NONE;
  }
  return r;
}

// ══════════════════════════════════════════════════════════════
//  Flow Rate — pulse counter (interrupt-based)
// ══════════════════════════════════════════════════════════════

SensorReading SensorManager::readFlowRate(int channel) {
  SensorReading r;
  r.timestamp = millis();

#if SIMULATION_MODE
  float noise = (random(-100, 100) / 1000.0);
  if (channel == 0) {
    _simFlow1 += (_pumpRunning ? 0.3 : -0.5) + noise;
    _simFlow1 = constrain(_simFlow1, 0.0, 30.0);
    r.value = _simFlow1;
  } else {
    _simFlow2 += (_pumpRunning ? 0.25 : -0.4) + noise;
    _simFlow2 = constrain(_simFlow2, 0.0, 25.0);
    r.value = _simFlow2;
  }
#else
  unsigned long now = millis();
  unsigned long elapsed = now - _lastFlowTime;

  if (elapsed >= 1000) {
    noInterrupts();
    long count1 = pulseCount1; pulseCount1 = 0;
    long count2 = pulseCount2; pulseCount2 = 0;
    interrupts();

    _lastFlow1 = ((1000.0 / elapsed) * count1) / _cal.flowCalFactor;
    _lastFlow2 = ((1000.0 / elapsed) * count2) / _cal.flowCalFactor;
    _lastFlowTime = now;
  }

  r.value = (channel == 0) ? _lastFlow1 : _lastFlow2;
#endif

  r.isValid = (r.value <= MAX_FLOW_RATE);
  r.errorCode = r.isValid ? ERR_NONE : ERR_SENSOR_FAILURE;
  return r;
}

// ══════════════════════════════════════════════════════════════
//  Temperature — DS18B20 (1-Wire)
// ══════════════════════════════════════════════════════════════

SensorReading SensorManager::readTemperature(int channel) {
  SensorReading r;
  r.timestamp = millis();

#if !ENABLE_TEMPERATURE_SENSORS
  r.value = 0.0;
  r.isValid = false;
  r.errorCode = ERR_NONE;
  return r;
#else
#if SIMULATION_MODE
  float noise = (random(-100, 100) / 1000.0);
  if (channel == 0) {
    _simTemp1 += noise * 0.1;
    _simTemp1 = constrain(_simTemp1, 15.0, 40.0);
    r.value = _simTemp1;
  } else {
    _simTemp2 += noise * 0.1;
    _simTemp2 = constrain(_simTemp2, 15.0, 40.0);
    r.value = _simTemp2;
  }
#else
  DallasTemperature &sensor = (channel == 0) ? tempSensor1 : tempSensor2;
  sensor.requestTemperatures();
  float temp = sensor.getTempCByIndex(0);
  r.value = (temp == DEVICE_DISCONNECTED_C) ? -127.0 : temp;
#endif

  if (r.value < MIN_TEMPERATURE || r.value > MAX_TEMPERATURE || r.value == -127.0) {
    r.isValid = false;
    r.errorCode = ERR_SENSOR_FAILURE;
  } else {
    r.isValid = true;
    r.errorCode = ERR_NONE;
  }
  return r;
#endif
}

SensorReading SensorManager::readPumpCurrent() {
  SensorReading r;
  r.timestamp = millis();

#if ENABLE_PUMP_CURRENT_SENSOR
#if SIMULATION_MODE
  float noise = (random(-50, 50) / 100.0);
  _simPumpCurrent += (_pumpRunning ? 0.35 : -0.6) + noise;
  _simPumpCurrent = constrain(_simPumpCurrent, 0.0, 10.0);
  r.value = _simPumpCurrent;
#else
  float raw = readAnalogSmoothed(PIN_PUMP_CURRENT);
  r.value = (raw - PUMP_CURRENT_ZERO_ADC) * PUMP_CURRENT_AMPS_PER_ADC;
  if (r.value < 0) r.value = 0;
#endif
  r.isValid = (r.value <= MAX_PUMP_CURRENT);
  r.errorCode = r.isValid ? ERR_NONE : ERR_PUMP_FAILURE;
#else
  r.value = 0.0;
  r.isValid = false;
  r.errorCode = ERR_NONE;
#endif

  return r;
}

// ══════════════════════════════════════════════════════════════
//  Safety checks
// ══════════════════════════════════════════════════════════════

ErrorCode SensorManager::checkSafety(const SensorSnapshot &snap) {
  if (!snap.turb1.isValid) return snap.turb1.errorCode;
  if (!snap.turb2.isValid) return snap.turb2.errorCode;
  if (!snap.ph1.isValid) return snap.ph1.errorCode;
  if (!snap.ph2.isValid) return snap.ph2.errorCode;
  if (!snap.pressure1.isValid) return snap.pressure1.errorCode;
  if (!snap.pressure2.isValid) return snap.pressure2.errorCode;
  if (snap.temp1.errorCode != ERR_NONE) return snap.temp1.errorCode;
  if (snap.temp2.errorCode != ERR_NONE) return snap.temp2.errorCode;
  if (snap.pumpCurrent.errorCode == ERR_PUMP_FAILURE) return snap.pumpCurrent.errorCode;
  return ERR_NONE;
}

bool SensorManager::isCritical(ErrorCode err) {
  return (err == ERR_PRESSURE_HIGH || err == ERR_SENSOR_FAILURE || err == ERR_PUMP_FAILURE);
}

// ══════════════════════════════════════════════════════════════
//  Calibration
// ══════════════════════════════════════════════════════════════

void SensorManager::setCalibration(CalibrationData cal) { _cal = cal; }
CalibrationData SensorManager::getCalibration() { return _cal; }

void SensorManager::loadCalibrationFromNVS() {
#if ENABLE_NVS_STORAGE
  Preferences prefs;
  if (prefs.begin(NVS_NAMESPACE, true)) {
    _cal.turbCleanADC = prefs.getFloat("turbClean", TURB_CLEAN_ADC);
    _cal.turbDirtyADC = prefs.getFloat("turbDirty", TURB_DIRTY_ADC);
    _cal.phNeutralVoltage = prefs.getFloat("phNeutral", PH_NEUTRAL_VOLTAGE);
    _cal.phSlope = prefs.getFloat("phSlope", PH_SLOPE);
    _cal.pressZeroVoltage1 = prefs.getFloat("pZero1", PRESS_ZERO_VOLTAGE_1);
    _cal.pressZeroVoltage2 = prefs.getFloat("pZero2", PRESS_ZERO_VOLTAGE_2);
    _cal.flowCalFactor = prefs.getFloat("flowCal", FLOW_CALIBRATION_FACTOR);
    prefs.end();
  }
#endif
}

void SensorManager::saveCalibrationToNVS() {
#if ENABLE_NVS_STORAGE
  Preferences prefs;
  if (prefs.begin(NVS_NAMESPACE, false)) {
    prefs.putFloat("turbClean", _cal.turbCleanADC);
    prefs.putFloat("turbDirty", _cal.turbDirtyADC);
    prefs.putFloat("phNeutral", _cal.phNeutralVoltage);
    prefs.putFloat("phSlope", _cal.phSlope);
    prefs.putFloat("pZero1", _cal.pressZeroVoltage1);
    prefs.putFloat("pZero2", _cal.pressZeroVoltage2);
    prefs.putFloat("flowCal", _cal.flowCalFactor);
    prefs.end();
  }
#endif
}
