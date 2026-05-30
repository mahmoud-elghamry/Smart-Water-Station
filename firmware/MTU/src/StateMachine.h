#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

enum SystemState
{
  STATE_IDLE = 0,
  STATE_PUMP_RUNNING = 1,
  STATE_FILLING = 2,
  STATE_DRAINING = 3,
  STATE_CALIBRATING = 4,
  STATE_EMERGENCY_STOP = 5,
  STATE_ERROR = 6,
  STATE_MAINTENANCE = 7
};

// Convert state to string for telemetry
inline const char *stateToString(SystemState state)
{
  switch (state)
  {
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

enum ErrorCode
{
  ERR_NONE = 0,
  ERR_TDS_HIGH = 1,        // kept for backwards compat (used as turbidity high)
  ERR_TDS_LOW = 2,
  ERR_PRESSURE_HIGH = 3,
  ERR_PRESSURE_LOW = 4,
  ERR_WATER_LEVEL_LOW = 5,
  ERR_SENSOR_FAILURE = 6,
  ERR_COMMUNICATION = 7,
  ERR_PUMP_FAILURE = 8,
  ERR_COMM_TIMEOUT = 9,
  ERR_PH_OUT_OF_RANGE = 10,
  ERR_TEMP_HIGH = 11,
  ERR_TEMP_LOW = 12
};

inline const char *errorToString(ErrorCode err)
{
  switch (err)
  {
  case ERR_NONE:
    return "OK";
  case ERR_TDS_HIGH:
    return "TURBIDITY_HIGH";
  case ERR_TDS_LOW:
    return "TURBIDITY_LOW";
  case ERR_PRESSURE_HIGH:
    return "PRESSURE_HIGH";
  case ERR_PRESSURE_LOW:
    return "PRESSURE_LOW";
  case ERR_WATER_LEVEL_LOW:
    return "WATER_LEVEL_LOW";
  case ERR_SENSOR_FAILURE:
    return "SENSOR_FAILURE";
  case ERR_COMMUNICATION:
    return "COMMUNICATION_ERROR";
  case ERR_PUMP_FAILURE:
    return "PUMP_FAILURE";
  case ERR_COMM_TIMEOUT:
    return "COMM_TIMEOUT";
  case ERR_PH_OUT_OF_RANGE:
    return "PH_OUT_OF_RANGE";
  case ERR_TEMP_HIGH:
    return "TEMP_HIGH";
  case ERR_TEMP_LOW:
    return "TEMP_LOW";
  default:
    return "UNKNOWN_ERROR";
  }
}

#endif // STATE_MACHINE_H
