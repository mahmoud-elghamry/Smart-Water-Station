/**
 * Smart Water Station V2.1.0
 * Professional ESP32 Firmware with Modular Architecture
 *
 * Main entry point - Clean state machine integration
 * All business logic is delegated to modules, not in loop()
 */

#include "CommsHandler.h"
#include "Config.h"
#include "SensorManager.h"
#include "StateMachine.h"
#include <Arduino.h>


// ============== Module Instances ==============
SensorManager sensors;
CommsHandler comms;

// ============== State Variables ==============
SystemState currentState = STATE_IDLE;
SystemState previousState = STATE_IDLE;
ErrorCode lastError = ERR_NONE;

// ============== Timing (Non-Blocking) ==============
unsigned long lastTelemetryTime = 0;
unsigned long lastStateTime = 0;
unsigned long stateEntryTime = 0;

// ============== Function Prototypes ==============
void processCommands();
void updateStateMachine();
void sendTelemetry();
void setState(SystemState newState);
void handleStateEntry();
void handleStateExit();

// ============== Setup ==============
void setup() {
  // Initialize communication first (for debug output)
  comms.begin();

  // Send boot message
  delay(100); // Brief delay for serial stability
  comms.sendInfo();

  // Initialize sensors
  sensors.begin();

  // Initialize output pins
  pinMode(PIN_PUMP, OUTPUT);
  pinMode(PIN_VALVE, OUTPUT);
  pinMode(PIN_LED_STATUS, OUTPUT);

  // Set safe initial state
  digitalWrite(PIN_PUMP, LOW);
  digitalWrite(PIN_VALVE, LOW);
  digitalWrite(PIN_LED_STATUS, LOW);

  // Record state entry time
  stateEntryTime = millis();

  // Send initial state
  comms.sendState(currentState, lastError);
}

// ============== Main Loop (Clean!) ==============
void loop() {
  unsigned long currentTime = millis();

  // 1. Update Communication (read serial buffer)
  comms.update();

  // 2. Process Incoming Commands (high priority)
  if (comms.hasCommand()) {
    processCommands();
  }

  // 3. Check Safety (unless in calibration or already stopped)
  if (currentState != STATE_EMERGENCY_STOP &&
      currentState != STATE_CALIBRATING && currentState != STATE_MAINTENANCE) {

    ErrorCode safety = sensors.checkSafety();
    if (safety != ERR_NONE) {
      lastError = safety;
      // Critical errors trigger emergency stop
      if (sensors.isCritical()) {
        setState(STATE_EMERGENCY_STOP);
      }
    }
  }

  // 4. Update State Machine
  updateStateMachine();

  // 5. Send Telemetry (1 second interval)
  if (currentTime - lastTelemetryTime >= TELEMETRY_INTERVAL) {
    sendTelemetry();
    lastTelemetryTime = currentTime;
  }

  // 6. Send State Update (on change or heartbeat)
  if (currentState != previousState ||
      (currentTime - lastStateTime >= HEARTBEAT_INTERVAL)) {
    comms.sendState(currentState, lastError);
    previousState = currentState;
    lastStateTime = currentTime;
  }
}

// ============== Command Processing ==============
void processCommands() {
  Command cmd = comms.getCommand();

  if (!cmd.isValid)
    return;

  switch (cmd.type) {
  case CMD_SET_PUMP:
    if (currentState != STATE_EMERGENCY_STOP) {
      if (cmd.state) {
        setState(STATE_PUMP_RUNNING);
      } else {
        setState(STATE_IDLE);
      }
    }
    break;

  case CMD_SET_VALVE:
    if (currentState != STATE_EMERGENCY_STOP) {
      digitalWrite(PIN_VALVE, cmd.state ? HIGH : LOW);
    }
    break;

  case CMD_EMERGENCY_STOP:
    setState(STATE_EMERGENCY_STOP);
    lastError = ERR_NONE; // Clear error, this is intentional stop
    break;

  case CMD_RESET:
    if (currentState == STATE_EMERGENCY_STOP || currentState == STATE_ERROR) {
      lastError = ERR_NONE;
      setState(STATE_IDLE);
    }
    break;

  case CMD_PING:
    comms.sendPong();
    break;

  case CMD_GET_STATUS:
    comms.sendInfo();
    comms.sendState(currentState, lastError);
    break;

  case CMD_CALIBRATE_TDS:
    if (currentState == STATE_IDLE || currentState == STATE_CALIBRATING) {
      setState(STATE_CALIBRATING);
      sensors.calibrateTDS(cmd.value);
      comms.sendCalibrationAck("tds", cmd.value);
      setState(STATE_IDLE);
    }
    break;

  case CMD_CALIBRATE_PRESSURE:
    if (currentState == STATE_IDLE || currentState == STATE_CALIBRATING) {
      setState(STATE_CALIBRATING);
      sensors.calibratePressure(cmd.value);
      comms.sendCalibrationAck("pressure", cmd.value);
      setState(STATE_IDLE);
    }
    break;

  case CMD_SET_MODE:
    if (cmd.state) {
      setState(STATE_MAINTENANCE);
    } else {
      setState(STATE_IDLE);
    }
    break;

  default:
    comms.sendError("Unknown command");
    break;
  }
}

// ============== State Machine Update ==============
void updateStateMachine() {
  // Update LED based on state
  switch (currentState) {
  case STATE_IDLE:
    digitalWrite(PIN_PUMP, LOW);
    digitalWrite(PIN_LED_STATUS, LOW);
    sensors.setPumpState(false);
    break;

  case STATE_PUMP_RUNNING:
    digitalWrite(PIN_PUMP, HIGH);
    // Blink LED while running
    digitalWrite(PIN_LED_STATUS, (millis() / 500) % 2);
    sensors.setPumpState(true);
    break;

  case STATE_FILLING:
  case STATE_DRAINING:
    digitalWrite(PIN_PUMP, HIGH);
    digitalWrite(PIN_LED_STATUS, (millis() / 250) % 2); // Fast blink
    sensors.setPumpState(true);
    break;

  case STATE_CALIBRATING:
    digitalWrite(PIN_PUMP, LOW);
    digitalWrite(PIN_LED_STATUS, (millis() / 1000) % 2); // Slow blink
    sensors.setPumpState(false);
    break;

  case STATE_EMERGENCY_STOP:
    digitalWrite(PIN_PUMP, LOW);
    digitalWrite(PIN_VALVE, LOW);
    digitalWrite(PIN_LED_STATUS, HIGH); // Solid ON = Emergency
    sensors.setPumpState(false);
    break;

  case STATE_ERROR:
    digitalWrite(PIN_PUMP, LOW);
    digitalWrite(PIN_LED_STATUS, (millis() / 100) % 2); // Very fast blink
    sensors.setPumpState(false);
    break;

  case STATE_MAINTENANCE:
    digitalWrite(PIN_PUMP, LOW);
    digitalWrite(PIN_LED_STATUS, (millis() / 2000) % 2); // Very slow blink
    sensors.setPumpState(false);
    break;
  }
}

// ============== State Transition ==============
void setState(SystemState newState) {
  if (currentState != newState) {
    handleStateExit();
    previousState = currentState;
    currentState = newState;
    stateEntryTime = millis();
    handleStateEntry();
    comms.sendState(currentState, lastError);
  }
}

void handleStateEntry() {
  // Actions when entering a new state
  switch (currentState) {
  case STATE_EMERGENCY_STOP:
    // Immediately stop everything
    digitalWrite(PIN_PUMP, LOW);
    digitalWrite(PIN_VALVE, LOW);
    break;
  default:
    break;
  }
}

void handleStateExit() {
  // Cleanup when leaving a state
  switch (currentState) {
  case STATE_PUMP_RUNNING:
    sensors.setPumpState(false);
    break;
  default:
    break;
  }
}

// ============== Telemetry ==============
void sendTelemetry() {
  SensorReading tds = sensors.readTDS();
  SensorReading pressure = sensors.readPressure();
  SensorReading flow = sensors.readFlowRate();
  SensorReading level = sensors.readWaterLevel();

  comms.sendTelemetry("tds", tds.value, tds.isValid ? "OK" : "ERROR");

  comms.sendTelemetry("pressure", pressure.value,
                      pressure.isValid ? "OK" : "ERROR");

  comms.sendTelemetry("flow", flow.value, flow.isValid ? "OK" : "ERROR");

  comms.sendTelemetry("level", level.value, level.isValid ? "OK" : "ERROR");
}