#ifndef COMMS_HANDLER_H
#define COMMS_HANDLER_H

#include "Config.h"
#include "StateMachine.h"
#include <Arduino.h>
#include <ArduinoJson.h>


// Command types
enum CommandType {
  CMD_UNKNOWN,
  CMD_SET_PUMP,
  CMD_SET_VALVE,
  CMD_EMERGENCY_STOP,
  CMD_RESET,
  CMD_PING,
  CMD_GET_STATUS,
  CMD_CALIBRATE_TDS,
  CMD_CALIBRATE_PRESSURE,
  CMD_SET_MODE
};

struct Command {
  CommandType type;
  String rawCmd;
  String state;
  float value;
  bool isValid;
  unsigned long timestamp;  // Time when command was received
};

class CommsHandler {
public:
  void begin();
  void update();

  // Commands
  bool hasCommand();
  Command getCommand();

  // Telemetry
  void sendTelemetry(const char *sensor, float value, const char *status);
  void sendState(SystemState state, ErrorCode error);
  void sendPong();
  void sendCalibrationAck(const char *sensor, float value);
  void sendError(const char *msg);
  void sendVersion();

private:
  String _inputBuffer;
  Command _lastCommand;
  bool _commandReady;

  CommandType parseCommandType(const String &cmd);
  void processBuffer();
  int findJsonStart();
};

#endif // COMMS_HANDLER_H
