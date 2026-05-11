#include "CommsHandler.h"

void CommsHandler::begin()
{
  Serial.begin(BAUD_RATE);
  memset(_inputBuffer, 0, sizeof(_inputBuffer));
  _inputLen = 0;
  _commandReady = false;
  _lastCommand.isValid = false;
}

void CommsHandler::update()
{
  // Read available serial data into buffer
  while (Serial.available() > 0)
  {
    char c = Serial.read();

    // Buffer overflow protection
    if (_inputLen >= BUFFER_SIZE - 1)
    {
      _inputLen = 0; // Clear on overflow
      memset(_inputBuffer, 0, sizeof(_inputBuffer));
      continue;
    }

    _inputBuffer[_inputLen++] = c;
    _inputBuffer[_inputLen] = '\0';

    // Process on newline or closing brace
    if (c == '\n' || c == '}')
    {
      processBuffer();
    }
  }
}

int CommsHandler::findJsonStart()
{
  for (uint16_t i = 0; i < _inputLen; i++)
  {
    if (_inputBuffer[i] == '{')
    {
      return i;
    }
  }
  return -1;
}

void CommsHandler::processBuffer()
{
  // Find start of JSON
  int startIdx = findJsonStart();
  if (startIdx < 0)
  {
    _inputLen = 0;
    _inputBuffer[0] = '\0';
    return;
  }

  // Shift buffer if garbage exists before JSON start
  if (startIdx > 0)
  {
    _inputLen -= startIdx;
    memmove(_inputBuffer, _inputBuffer + startIdx, _inputLen + 1);
  }

  // Use JsonDocument for memory safety (no heap fragmentation)
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, _inputBuffer, _inputLen);

  if (!error)
  {
    // Successfully parsed JSON
    _lastCommand = Command();
    _lastCommand.isValid = true;
    _lastCommand.timestamp = millis();
    strncpy(_lastCommand.rawCmd, _inputBuffer, BUFFER_SIZE - 1);
    _lastCommand.rawCmd[BUFFER_SIZE - 1] = '\0';

    // Parse command type
    const char *cmd = doc["cmd"] | "";
    _lastCommand.type = parseCommandType(cmd);

    // Parse optional parameters
    if (doc["state"].is<bool>())
    {
      _lastCommand.state = doc["state"].as<bool>();
    }
    else if (doc["state"].is<const char *>())
    {
      const char *state = doc["state"];
      _lastCommand.state =
          (strcmp(state, "ON") == 0 || strcmp(state, "on") == 0 ||
           strcmp(state, "true") == 0 || strcmp(state, "1") == 0);
    }

    if (!doc["value"].isNull())
    {
      _lastCommand.value = doc["value"].as<float>();
    }

    _commandReady = true;
    _inputLen = 0;
    _inputBuffer[0] = '\0';
  }
  else if (error == DeserializationError::IncompleteInput)
  {
    // Wait for more data - don't clear buffer
  }
  else
  {
    // Invalid JSON, clear buffer
    _inputLen = 0;
    _inputBuffer[0] = '\0';
  }
}

CommandType CommsHandler::parseCommandType(const char *cmd)
{
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

Command CommsHandler::getCommand()
{
  _commandReady = false;
  return _lastCommand;
}

void CommsHandler::sendTelemetry(const char *sensor, float value,
                                 const char *status)
{
  JsonDocument doc;
  doc["type"] = "telemetry";
  doc["sensor"] = sensor;
  doc["value"] = value;
  doc["status"] = status;
  doc["ts"] = millis();

  serializeJson(doc, Serial);
  Serial.println();
}

void CommsHandler::sendState(SystemState state, ErrorCode error)
{
  JsonDocument doc;
  doc["type"] = "state";
  doc["state"] = stateToString(state);
  doc["stateCode"] = (int)state;
  doc["error"] = errorToString(error);
  doc["errorCode"] = (int)error;
  doc["ts"] = millis();

  serializeJson(doc, Serial);
  Serial.println();
}

void CommsHandler::sendPong()
{
  JsonDocument doc;
  doc["type"] = "pong";
  doc["version"] = FIRMWARE_VERSION;
  doc["uptime"] = millis();

  serializeJson(doc, Serial);
  Serial.println();
}

void CommsHandler::sendCalibrationAck(const char *sensor, float value)
{
  JsonDocument doc;
  doc["type"] = "calibration";
  doc["sensor"] = sensor;
  doc["value"] = value;
  doc["status"] = "applied";

  serializeJson(doc, Serial);
  Serial.println();
}

void CommsHandler::sendInfo()
{
  JsonDocument doc;
  doc["type"] = "info";
  doc["device"] = "Smart Water Station";
  doc["firmware"] = FIRMWARE_VERSION;
  doc["uptime"] = millis();
  doc["freeHeap"] = ESP.getFreeHeap();

  serializeJson(doc, Serial);
  Serial.println();
}

void CommsHandler::sendError(const char *message)
{
  JsonDocument doc;
  doc["type"] = "error";
  doc["error"] = message;
  doc["ts"] = millis();

  serializeJson(doc, Serial);
  Serial.println();
}

void CommsHandler::sendDataForwarderLine(float tds, float pressure,
                                         float flow, float level)
{
  // Edge Impulse data forwarder expects: value1,value2,...,valueN\n
  // No JSON, no labels - just comma-separated floats.
  Serial.print(tds, 2);
  Serial.print(",");
  Serial.print(pressure, 2);
  Serial.print(",");
  Serial.print(flow, 2);
  Serial.print(",");
  Serial.println(level, 2);
}
