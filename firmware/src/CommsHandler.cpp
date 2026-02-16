#include "CommsHandler.h"

void CommsHandler::begin() {
  Serial.begin(BAUD_RATE);
  _inputBuffer.reserve(BUFFER_SIZE);
  _inputBuffer = "";
  _commandReady = false;
  _lastCommand.isValid = false;
}

void CommsHandler::update() {
  // Read available serial data into buffer
  while (Serial.available() > 0) {
    char c = Serial.read();

    // Buffer overflow protection
    if (_inputBuffer.length() >= BUFFER_SIZE - 1) {
      _inputBuffer = ""; // Clear on overflow
      continue;
    }

    _inputBuffer += c;

    // Process on newline or buffer near full
    if (c == '\n' || c == '}') {
      processBuffer();
    }
  }
}

int CommsHandler::findJsonStart() {
  for (unsigned int i = 0; i < _inputBuffer.length(); i++) {
    if (_inputBuffer[i] == '{') {
      return i;
    }
  }
  return -1;
}

void CommsHandler::processBuffer() {
  // Find start of JSON
  int startIdx = findJsonStart();
  if (startIdx < 0) {
    _inputBuffer = "";
    return;
  }

  // Remove garbage before JSON start
  if (startIdx > 0) {
    _inputBuffer = _inputBuffer.substring(startIdx);
  }

  // Use StaticJsonDocument for memory safety (no heap fragmentation)
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, _inputBuffer);

  if (!error) {
    // Successfully parsed JSON
    _lastCommand.isValid = true;
    _lastCommand.timestamp = millis();

    // Parse command type
    const char *cmd = doc["cmd"] | "";
    _lastCommand.type = parseCommandType(cmd);

    // Parse optional parameters
    if (doc.containsKey("state")) {
      const char *state = doc["state"];
      _lastCommand.state = (strcmp(state, "ON") == 0);
    }

    if (doc.containsKey("value")) {
      _lastCommand.value = doc["value"].as<float>();
    }

    _commandReady = true;
    _inputBuffer = "";

  } else if (error == DeserializationError::IncompleteInput) {
    // Wait for more data - don't clear buffer
  } else {
    // Invalid JSON, clear buffer
    _inputBuffer = "";
  }
}

CommandType CommsHandler::parseCommandType(const char *cmd) {
  if (strcmp(cmd, "SET_PUMP") == 0)
    return CMD_SET_PUMP;
  if (strcmp(cmd, "SET_VALVE") == 0)
    return CMD_SET_VALVE;
  if (strcmp(cmd, "EMERGENCY_STOP") == 0)
    return CMD_EMERGENCY_STOP;
  if (strcmp(cmd, "RESET") == 0)
    return CMD_RESET;
  if (strcmp(cmd, "PING") == 0)
    return CMD_PING;
  if (strcmp(cmd, "GET_STATUS") == 0)
    return CMD_GET_STATUS;
  if (strcmp(cmd, "CALIBRATE_TDS") == 0)
    return CMD_CALIBRATE_TDS;
  if (strcmp(cmd, "CALIBRATE_PRESSURE") == 0)
    return CMD_CALIBRATE_PRESSURE;
  if (strcmp(cmd, "SET_MODE") == 0)
    return CMD_SET_MODE;
  return CMD_UNKNOWN;
}

bool CommsHandler::hasCommand() { return _commandReady; }

Command CommsHandler::getCommand() {
  _commandReady = false;
  return _lastCommand;
}

void CommsHandler::sendTelemetry(const char *sensor, float value,
                                 const char *status) {
  StaticJsonDocument<128> doc;
  doc["type"] = "telemetry";
  doc["sensor"] = sensor;
  doc["value"] = value;
  doc["status"] = status;
  doc["ts"] = millis();

  serializeJson(doc, Serial);
  Serial.println();
}

void CommsHandler::sendState(SystemState state, ErrorCode error) {
  StaticJsonDocument<128> doc;
  doc["type"] = "state";
  doc["state"] = stateToString(state);
  doc["stateCode"] = (int)state;
  doc["error"] = errorToString(error);
  doc["errorCode"] = (int)error;
  doc["ts"] = millis();

  serializeJson(doc, Serial);
  Serial.println();
}

void CommsHandler::sendPong() {
  StaticJsonDocument<64> doc;
  doc["type"] = "pong";
  doc["version"] = FIRMWARE_VERSION;
  doc["uptime"] = millis();

  serializeJson(doc, Serial);
  Serial.println();
}

void CommsHandler::sendCalibrationAck(const char *sensor, float value) {
  StaticJsonDocument<96> doc;
  doc["type"] = "calibration";
  doc["sensor"] = sensor;
  doc["value"] = value;
  doc["status"] = "applied";

  serializeJson(doc, Serial);
  Serial.println();
}

void CommsHandler::sendInfo() {
  StaticJsonDocument<128> doc;
  doc["type"] = "info";
  doc["device"] = "Smart Water Station";
  doc["firmware"] = FIRMWARE_VERSION;
  doc["uptime"] = millis();
  doc["freeHeap"] = ESP.getFreeHeap();

  serializeJson(doc, Serial);
  Serial.println();
}

void CommsHandler::sendError(const char *message) {
  StaticJsonDocument<96> doc;
  doc["type"] = "error";
  doc["error"] = message;
  doc["ts"] = millis();

  serializeJson(doc, Serial);
  Serial.println();
}

const char *CommsHandler::stateToString(SystemState state) {
  switch (state) {
  case STATE_IDLE:
    return "IDLE";
  case STATE_PUMP_RUNNING:
    return "PUMP_RUNNING";
  case STATE_FILLING:
    return "FILLING";
  case STATE_DRAINING:
    return "DRAINING";
  case STATE_CALIBRATING:
    return "CALIBRATING";
  case STATE_EMERGENCY_STOP:
    return "EMERGENCY_STOP";
  case STATE_ERROR:
    return "ERROR";
  case STATE_MAINTENANCE:
    return "MAINTENANCE";
  default:
    return "UNKNOWN";
  }
}

const char *CommsHandler::errorToString(ErrorCode error) {
  switch (error) {
  case ERR_NONE:
    return "NONE";
  case ERR_TDS_HIGH:
    return "TDS_HIGH";
  case ERR_TDS_LOW:
    return "TDS_LOW";
  case ERR_PRESSURE_HIGH:
    return "PRESSURE_HIGH";
  case ERR_PRESSURE_LOW:
    return "PRESSURE_LOW";
  case ERR_WATER_LEVEL_LOW:
    return "WATER_LEVEL_LOW";
  case ERR_SENSOR_FAILURE:
    return "SENSOR_FAILURE";
  case ERR_COMMUNICATION:
    return "COMMUNICATION";
  case ERR_TIMEOUT:
    return "TIMEOUT";
  default:
    return "UNKNOWN";
  }
}
