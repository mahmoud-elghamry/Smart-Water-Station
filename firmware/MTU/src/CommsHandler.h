#ifndef COMMS_HANDLER_H
#define COMMS_HANDLER_H

#include "Config.h"
#include "StateMachine.h"
#include <Arduino.h>
#include <ArduinoJson.h>

// Command types
enum CommandType
{
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

struct Command
{
  CommandType type = CMD_UNKNOWN;
  char rawCmd[BUFFER_SIZE]; // Fixed-size buffer instead of String to avoid heap fragmentation
  bool state = false;
  float value = 0.0;
  bool isValid = false;
  unsigned long timestamp = 0; // Time when command was received
};

class CommsHandler
{
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
  void sendInfo();
  void sendError(const char *msg);

  // Edge Impulse data forwarder (CSV: turb1,turb2,ph1,ph2,flow1,flow2,press1,press2,temp1,temp2,pump_current)
  void sendDataForwarderLine(float t1, float t2, float ph1, float ph2,
                             float f1, float f2, float p1, float p2,
                             float temp1, float temp2, float pumpCurrent);

private:
  char _inputBuffer[BUFFER_SIZE]; // Fixed-size buffer instead of String
  uint16_t _inputLen;
  Command _lastCommand;
  bool _commandReady;

  CommandType parseCommandType(const char *cmd);
  void processBuffer();
  int findJsonStart();
};

#endif // COMMS_HANDLER_H
