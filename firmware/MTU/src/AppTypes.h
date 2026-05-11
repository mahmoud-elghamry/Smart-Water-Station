#ifndef APP_TYPES_H
#define APP_TYPES_H

#include "SensorManager.h"
#include "StateMachine.h"
#include <Arduino.h>

enum ControlAction
{
  ACTION_NONE = 0,
  ACTION_SET_PUMP,
  ACTION_SET_VALVE,
  ACTION_EMERGENCY_STOP,
  ACTION_RESET,
  ACTION_CALIBRATE_TDS,
  ACTION_CALIBRATE_PRESSURE,
  ACTION_SET_MAINTENANCE
};

enum CommandSource
{
  SOURCE_LOCAL = 0,
  SOURCE_SERIAL,
  SOURCE_WEB,
  SOURCE_AI
};

struct ControlCommand
{
  ControlAction action = ACTION_NONE;
  CommandSource source = SOURCE_LOCAL;
  bool state = false;
  float value = 0.0f;
  unsigned long timestamp = 0;
};

struct AiResult
{
  bool available = false;
  bool fault = false;
  float confidence = 0.0f;
  char label[24] = "not_ready";
  unsigned long timestamp = 0;
};

struct TelemetrySnapshot
{
  SensorReading tds;
  SensorReading pressure;
  SensorReading flow;
  SensorReading level;
  AiResult ai;
  SystemState state = STATE_IDLE;
  ErrorCode error = ERR_NONE;
  bool pumpRunning = false;
  bool valveOpen = false;
  bool rtuOnline = false;
  unsigned long timestamp = 0;
  unsigned long uptime = 0;
};

#endif // APP_TYPES_H
