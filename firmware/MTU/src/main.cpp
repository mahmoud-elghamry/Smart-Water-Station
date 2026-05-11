/**
 * Smart Water Station MTU  v3.0.0
 *
 * Graduation-friendly architecture:
 * - FreeRTOS tasks for control, serial, web, AI, HMI, RTU link, and data forwarder.
 * - ESP32 works standalone in simulation mode while RTU/Edge Impulse are added later.
 * - All output decisions pass through ControlTask.
 * - Task Watchdog Timer protects against stuck tasks.
 *
 * Edge Impulse workflow:
 *   1. Set ENABLE_DATA_FORWARDER=1 → upload → run edge-impulse-data-forwarder on PC
 *   2. Collect training data on Edge Impulse Studio
 *   3. Deploy as Arduino Library → place in lib/edge_impulse/
 *   4. Set ENABLE_DATA_FORWARDER=0, ENABLE_EDGE_IMPULSE=1 → upload
 */

#include "AiService.h"
#include "AppTypes.h"
#include "CommsHandler.h"
#include "Config.h"
#include "SensorManager.h"
#include "StateMachine.h"
#include "WebDashboard.h"
#include <Arduino.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#if ENABLE_RTU_LINK
#include <ArduinoJson.h>
#endif

// ============== Module Instances ==============
SensorManager sensors;
CommsHandler comms;
AiService aiService;

#if ENABLE_WEB_DASHBOARD
WebDashboard webDashboard;
#endif

// ============== RTOS Resources ==============
QueueHandle_t controlQueue = nullptr;
SemaphoreHandle_t serialMutex = nullptr;
portMUX_TYPE snapshotMux = portMUX_INITIALIZER_UNLOCKED;

// ============== Shared System Model ==============
TelemetrySnapshot latestSnapshot;
SystemState currentState = STATE_IDLE;
ErrorCode lastError = ERR_NONE;
bool pumpRunning = false;
bool valveOpen = false;

// ============== Helpers ==============
TelemetrySnapshot getSnapshotCopy();
bool publishControlCommand(const ControlCommand &command);
void publishSnapshot(const TelemetrySnapshot &snapshot);
void initializeOutputs();
void setState(SystemState newState);
void updateOutputsForState();
void drainControlQueue();
void applyControlCommand(const ControlCommand &command);
void performSafetyCheck(const SensorSnapshot &sensorData);
void updateTelemetrySnapshot(const SensorSnapshot &sensorData);
void sendTelemetrySnapshot(const TelemetrySnapshot &snapshot);
bool mapSerialCommand(const Command &input, ControlCommand &output);
void handleSerialCommand(const Command &command);

// ============== Tasks ==============
void controlTask(void *parameter);
void serialTask(void *parameter);
void telemetryTask(void *parameter);
void aiTask(void *parameter);
void webTask(void *parameter);
void hmiTask(void *parameter);

#if ENABLE_RTU_LINK
void rtuTask(void *parameter);
void parseRtuFrame(const char *json, uint16_t len);
static unsigned long lastRtuFrameTime = 0;
#endif

#if ENABLE_DATA_FORWARDER
void dataForwarderTask(void *parameter);
#endif

void setup()
{
  comms.begin();
  delay(100);
  comms.sendInfo();

  sensors.begin();
  initializeOutputs();

  controlQueue = xQueueCreate(10, sizeof(ControlCommand));
  serialMutex = xSemaphoreCreateMutex();

  latestSnapshot.state = currentState;
  latestSnapshot.error = lastError;
  latestSnapshot.timestamp = millis();
  latestSnapshot.uptime = millis();

  aiService.begin();

  // Initialize Task Watchdog Timer
  esp_task_wdt_init(WDT_TIMEOUT_SECONDS, true);

#if ENABLE_WEB_DASHBOARD
  webDashboard.begin(getSnapshotCopy, publishControlCommand);
#endif

  xTaskCreatePinnedToCore(controlTask, "ControlTask", 4096, nullptr, 3, nullptr,
                          1);
  xTaskCreatePinnedToCore(serialTask, "SerialTask", 4096, nullptr, 2, nullptr,
                          0);
  xTaskCreatePinnedToCore(telemetryTask, "TelemetryTask", 4096, nullptr, 1,
                          nullptr, 0);
  xTaskCreatePinnedToCore(aiTask, "AiTask", 4096, nullptr, 1, nullptr, 1);

#if ENABLE_WEB_DASHBOARD
  xTaskCreatePinnedToCore(webTask, "WebTask", 6144, nullptr, 1, nullptr, 0);
#endif

#if ENABLE_HMI_DISPLAY
  xTaskCreatePinnedToCore(hmiTask, "HmiTask", 4096, nullptr, 1, nullptr, 1);
#endif

#if ENABLE_RTU_LINK
  xTaskCreatePinnedToCore(rtuTask, "RtuTask", 4096, nullptr, 2, nullptr, 0);
#endif

#if ENABLE_DATA_FORWARDER
  xTaskCreatePinnedToCore(dataForwarderTask, "DataFwdTask", 4096, nullptr, 1,
                          nullptr, 0);
#endif
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
}

TelemetrySnapshot getSnapshotCopy()
{
  TelemetrySnapshot copy;
  taskENTER_CRITICAL(&snapshotMux);
  copy = latestSnapshot;
  taskEXIT_CRITICAL(&snapshotMux);
  return copy;
}

bool publishControlCommand(const ControlCommand &command)
{
  if (controlQueue == nullptr)
  {
    return false;
  }

  return xQueueSend(controlQueue, &command, 0) == pdTRUE;
}

void publishSnapshot(const TelemetrySnapshot &snapshot)
{
  taskENTER_CRITICAL(&snapshotMux);
  latestSnapshot = snapshot;
  taskEXIT_CRITICAL(&snapshotMux);
}

void initializeOutputs()
{
  pinMode(PIN_PUMP, OUTPUT);
  pinMode(PIN_VALVE, OUTPUT);
  pinMode(PIN_LED_STATUS, OUTPUT);

  digitalWrite(PIN_PUMP, LOW);
  digitalWrite(PIN_VALVE, LOW);
  digitalWrite(PIN_LED_STATUS, LOW);
  sensors.setPumpState(false);
}

void setState(SystemState newState)
{
  // Protect currentState from concurrent access (ControlTask writes, others read)
  taskENTER_CRITICAL(&snapshotMux);
  if (currentState == newState)
  {
    taskEXIT_CRITICAL(&snapshotMux);
    return;
  }

  currentState = newState;

  if (currentState == STATE_EMERGENCY_STOP)
  {
    pumpRunning = false;
    valveOpen = false;
  }
  taskEXIT_CRITICAL(&snapshotMux);
}

void updateOutputsForState()
{
  // Read state under protection
  SystemState state;
  taskENTER_CRITICAL(&snapshotMux);
  state = currentState;
  taskEXIT_CRITICAL(&snapshotMux);

  switch (state)
  {
  case STATE_IDLE:
    pumpRunning = false;
    digitalWrite(PIN_LED_STATUS, LOW);
    break;

  case STATE_PUMP_RUNNING:
    pumpRunning = true;
    digitalWrite(PIN_LED_STATUS, (millis() / 500) % 2);
    break;

  case STATE_FILLING:
  case STATE_DRAINING:
    pumpRunning = true;
    digitalWrite(PIN_LED_STATUS, (millis() / 250) % 2);
    break;

  case STATE_CALIBRATING:
    pumpRunning = false;
    digitalWrite(PIN_LED_STATUS, (millis() / 1000) % 2);
    break;

  case STATE_EMERGENCY_STOP:
    pumpRunning = false;
    valveOpen = false;
    digitalWrite(PIN_LED_STATUS, HIGH);
    break;

  case STATE_ERROR:
    pumpRunning = false;
    digitalWrite(PIN_LED_STATUS, (millis() / 100) % 2);
    break;

  case STATE_MAINTENANCE:
    pumpRunning = false;
    digitalWrite(PIN_LED_STATUS, (millis() / 2000) % 2);
    break;
  }

  digitalWrite(PIN_PUMP, pumpRunning ? HIGH : LOW);
  digitalWrite(PIN_VALVE, valveOpen ? HIGH : LOW);
  sensors.setPumpState(pumpRunning);
}

void drainControlQueue()
{
  ControlCommand command;
  while (xQueueReceive(controlQueue, &command, 0) == pdTRUE)
  {
    applyControlCommand(command);
  }
}

void applyControlCommand(const ControlCommand &command)
{
  switch (command.action)
  {
  case ACTION_SET_PUMP:
    if (currentState != STATE_EMERGENCY_STOP)
    {
      setState(command.state ? STATE_PUMP_RUNNING : STATE_IDLE);
    }
    break;

  case ACTION_SET_VALVE:
    if (currentState != STATE_EMERGENCY_STOP)
    {
      valveOpen = command.state;
    }
    break;

  case ACTION_EMERGENCY_STOP:
    lastError = ERR_NONE;
    setState(STATE_EMERGENCY_STOP);
    break;

  case ACTION_RESET:
    if (currentState == STATE_EMERGENCY_STOP || currentState == STATE_ERROR)
    {
      lastError = ERR_NONE;
      setState(STATE_IDLE);
    }
    break;

  case ACTION_CALIBRATE_TDS:
    if (currentState == STATE_IDLE || currentState == STATE_CALIBRATING)
    {
      setState(STATE_CALIBRATING);
      sensors.calibrateTDS(command.value);
      setState(STATE_IDLE);
    }
    break;

  case ACTION_CALIBRATE_PRESSURE:
    if (currentState == STATE_IDLE || currentState == STATE_CALIBRATING)
    {
      setState(STATE_CALIBRATING);
      sensors.calibratePressure(command.value);
      setState(STATE_IDLE);
    }
    break;

  case ACTION_SET_MAINTENANCE:
    setState(command.state ? STATE_MAINTENANCE : STATE_IDLE);
    break;

  case ACTION_NONE:
  default:
    break;
  }
}

void performSafetyCheck(const SensorSnapshot &sensorData)
{
  if (currentState == STATE_EMERGENCY_STOP || currentState == STATE_CALIBRATING ||
      currentState == STATE_MAINTENANCE)
  {
    return;
  }

  // Use the already-captured sensor readings — no double read!
  ErrorCode safety = sensors.checkSafety(sensorData);
  if (safety == ERR_NONE)
  {
    lastError = ERR_NONE;
    if (currentState == STATE_ERROR)
    {
      setState(STATE_IDLE);
    }
    return;
  }

  lastError = safety;
  if (sensors.isCritical(safety))
  {
    setState(STATE_EMERGENCY_STOP);
  }
}

void updateTelemetrySnapshot(const SensorSnapshot &sensorData)
{
  TelemetrySnapshot snapshot = getSnapshotCopy();

  // Use the already-captured sensor readings — no double read!
  snapshot.tds = sensorData.tds;
  snapshot.pressure = sensorData.pressure;
  snapshot.flow = sensorData.flow;
  snapshot.level = sensorData.level;
  snapshot.state = currentState;
  snapshot.error = lastError;
  snapshot.pumpRunning = pumpRunning;
  snapshot.valveOpen = valveOpen;
  snapshot.timestamp = millis();
  snapshot.uptime = millis();

#if ENABLE_RTU_LINK
  snapshot.rtuOnline = (millis() - lastRtuFrameTime) < RTU_TIMEOUT_MS;
#else
  snapshot.rtuOnline = false;
#endif

  publishSnapshot(snapshot);
}

void sendTelemetrySnapshot(const TelemetrySnapshot &snapshot)
{
  if (serialMutex == nullptr)
  {
    return;
  }

  if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100)) != pdTRUE)
  {
    return;
  }

  comms.sendTelemetry("tds", snapshot.tds.value,
                      snapshot.tds.isValid ? "OK" : "ERROR");
  comms.sendTelemetry("pressure", snapshot.pressure.value,
                      snapshot.pressure.isValid ? "OK" : "ERROR");
  comms.sendTelemetry("flow", snapshot.flow.value,
                      snapshot.flow.isValid ? "OK" : "ERROR");
  comms.sendTelemetry("level", snapshot.level.value,
                      snapshot.level.isValid ? "OK" : "ERROR");
  comms.sendState(snapshot.state, snapshot.error);

  xSemaphoreGive(serialMutex);
}

bool mapSerialCommand(const Command &input, ControlCommand &output)
{
  output.source = SOURCE_SERIAL;
  output.state = input.state;
  output.value = input.value;
  output.timestamp = input.timestamp;

  switch (input.type)
  {
  case CMD_SET_PUMP:
    output.action = ACTION_SET_PUMP;
    return true;
  case CMD_SET_VALVE:
    output.action = ACTION_SET_VALVE;
    return true;
  case CMD_EMERGENCY_STOP:
    output.action = ACTION_EMERGENCY_STOP;
    return true;
  case CMD_RESET:
    output.action = ACTION_RESET;
    return true;
  case CMD_CALIBRATE_TDS:
    output.action = ACTION_CALIBRATE_TDS;
    return true;
  case CMD_CALIBRATE_PRESSURE:
    output.action = ACTION_CALIBRATE_PRESSURE;
    return true;
  case CMD_SET_MODE:
    output.action = ACTION_SET_MAINTENANCE;
    return true;
  default:
    output.action = ACTION_NONE;
    return false;
  }
}

void handleSerialCommand(const Command &command)
{
  if (!command.isValid)
  {
    return;
  }

  if (command.type == CMD_PING)
  {
    if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
      comms.sendPong();
      xSemaphoreGive(serialMutex);
    }
    return;
  }

  if (command.type == CMD_GET_STATUS)
  {
    TelemetrySnapshot snapshot = getSnapshotCopy();
    if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
      comms.sendInfo();
      comms.sendState(snapshot.state, snapshot.error);
      xSemaphoreGive(serialMutex);
    }
    return;
  }

  ControlCommand controlCommand;
  if (!mapSerialCommand(command, controlCommand))
  {
    if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
      comms.sendError("Unknown command");
      xSemaphoreGive(serialMutex);
    }
    return;
  }

  if (!publishControlCommand(controlCommand))
  {
    if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
      comms.sendError("Control queue full");
      xSemaphoreGive(serialMutex);
    }
  }
}

// ============== FreeRTOS Tasks ==============

void controlTask(void *parameter)
{
  (void)parameter;
  esp_task_wdt_add(NULL);

  for (;;)
  {
    esp_task_wdt_reset();

    drainControlQueue();

    // Read all sensors ONCE per cycle — used for both safety and telemetry
    SensorSnapshot sensorData = sensors.readAll();

    performSafetyCheck(sensorData);
    updateOutputsForState();
    updateTelemetrySnapshot(sensorData);

    vTaskDelay(pdMS_TO_TICKS(CONTROL_TASK_INTERVAL_MS));
  }
}

void serialTask(void *parameter)
{
  (void)parameter;
  esp_task_wdt_add(NULL);

  for (;;)
  {
    esp_task_wdt_reset();

    Command command;
    bool hasCommand = false;

    if (serialMutex != nullptr &&
        xSemaphoreTake(serialMutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
      comms.update();
      if (comms.hasCommand())
      {
        command = comms.getCommand();
        hasCommand = true;
      }
      xSemaphoreGive(serialMutex);
    }

    if (hasCommand)
    {
      handleSerialCommand(command);
    }

    vTaskDelay(pdMS_TO_TICKS(SERIAL_TASK_INTERVAL_MS));
  }
}

void telemetryTask(void *parameter)
{
  (void)parameter;
  esp_task_wdt_add(NULL);

  for (;;)
  {
    esp_task_wdt_reset();

#if !ENABLE_DATA_FORWARDER
    // Normal telemetry — disabled when data forwarder is active
    // because data forwarder needs exclusive Serial access with clean CSV
    sendTelemetrySnapshot(getSnapshotCopy());
#endif

    vTaskDelay(pdMS_TO_TICKS(TELEMETRY_INTERVAL));
  }
}

void aiTask(void *parameter)
{
  (void)parameter;
  esp_task_wdt_add(NULL);

  for (;;)
  {
    esp_task_wdt_reset();

    TelemetrySnapshot snapshot = getSnapshotCopy();
    AiResult result = aiService.run(snapshot);

    taskENTER_CRITICAL(&snapshotMux);
    latestSnapshot.ai = result;
    taskEXIT_CRITICAL(&snapshotMux);

    vTaskDelay(pdMS_TO_TICKS(AI_TASK_INTERVAL_MS));
  }
}

void webTask(void *parameter)
{
  (void)parameter;

#if ENABLE_WEB_DASHBOARD
  esp_task_wdt_add(NULL);

  for (;;)
  {
    esp_task_wdt_reset();
    webDashboard.loop();
    vTaskDelay(pdMS_TO_TICKS(WEB_TASK_INTERVAL_MS));
  }
#else
  vTaskDelete(nullptr);
#endif
}

void hmiTask(void *parameter)
{
  (void)parameter;

#if ENABLE_HMI_DISPLAY
  esp_task_wdt_add(NULL);

  for (;;)
  {
    esp_task_wdt_reset();
    // Future display update point. Keep Serial clean because it is the JSON link.
    vTaskDelay(pdMS_TO_TICKS(HMI_TASK_INTERVAL_MS));
  }
#else
  vTaskDelete(nullptr);
#endif
}

// ============== RTU Link ==============

#if ENABLE_RTU_LINK

// CRC8 (polynomial 0x07) — must match the RTU implementation exactly.
static uint8_t crc8(const char *data, uint16_t len)
{
  uint8_t crc = 0x00;
  for (uint16_t i = 0; i < len; i++)
  {
    crc ^= (uint8_t)data[i];
    for (uint8_t j = 0; j < 8; j++)
    {
      if (crc & 0x80)
        crc = (crc << 1) ^ 0x07;
      else
        crc <<= 1;
    }
  }
  return crc;
}

// Validate CRC8 in a JSON frame.
// The CRC covers everything from '{' up to (but not including) ',"crc":'.
static bool validateFrameCRC(const char *json, uint16_t len, uint8_t receivedCrc)
{
  // Find the CRC field start: ,"crc":
  const char *crcField = strstr(json, ",\"crc\":");
  if (!crcField)
  {
    return false;
  }
  uint16_t payloadLen = (uint16_t)(crcField - json);
  uint8_t computed = crc8(json, payloadLen);
  return (computed == receivedCrc);
}

static uint32_t lastRtuSeq = 0;
static uint32_t rtuFrameDrops = 0;

void parseRtuFrame(const char *json, uint16_t len)
{
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json, len);
  if (error)
  {
    return;
  }

  const char *type = doc["type"] | "";
  if (strcmp(type, "rtu_frame") != 0)
  {
    return;
  }

  // ── CRC8 validation ──
  uint8_t receivedCrc = doc["crc"] | 0;
  if (!validateFrameCRC(json, len, receivedCrc))
  {
    rtuFrameDrops++;
    return; // Drop corrupted frame silently
  }

  // ── Sequence number gap detection ──
  uint32_t seq = doc["seq"] | 0;
  if (lastRtuSeq > 0 && seq > lastRtuSeq + 1)
  {
    // Frames were lost (UART overflow, noise, etc.)
    rtuFrameDrops += (seq - lastRtuSeq - 1);
  }
  lastRtuSeq = seq;

  // ── RTU sensor error flags ──
  uint8_t errFlags = doc["err"] | 0;

  // ── Update snapshot with RTU sensor values ──
  TelemetrySnapshot snapshot = getSnapshotCopy();

  // TDS — skip if RTU reports sensor fault
  float tdsVal = doc["tds"] | RTU_SENSOR_FAULT_VALUE;
  if (tdsVal > RTU_SENSOR_FAULT_VALUE && !(errFlags & 0x01))
  {
    snapshot.tds.value = tdsVal;
    snapshot.tds.isValid = (tdsVal >= MIN_TDS && tdsVal <= MAX_TDS);
    snapshot.tds.errorCode = snapshot.tds.isValid ? ERR_NONE
                             : (tdsVal > MAX_TDS ? ERR_TDS_HIGH : ERR_TDS_LOW);
  }
  else
  {
    snapshot.tds.isValid = false;
    snapshot.tds.errorCode = ERR_SENSOR_FAILURE;
  }
  snapshot.tds.timestamp = millis();

  // Pressure
  float presVal = doc["pressure"] | RTU_SENSOR_FAULT_VALUE;
  if (presVal > RTU_SENSOR_FAULT_VALUE && !(errFlags & 0x02))
  {
    snapshot.pressure.value = presVal;
    snapshot.pressure.isValid = (presVal >= MIN_PRESSURE && presVal <= MAX_PRESSURE);
    snapshot.pressure.errorCode = snapshot.pressure.isValid ? ERR_NONE
                                  : (presVal > MAX_PRESSURE ? ERR_PRESSURE_HIGH : ERR_PRESSURE_LOW);
  }
  else
  {
    snapshot.pressure.isValid = false;
    snapshot.pressure.errorCode = ERR_SENSOR_FAILURE;
  }
  snapshot.pressure.timestamp = millis();

  // Flow
  float flowVal = doc["flow"] | RTU_SENSOR_FAULT_VALUE;
  if (flowVal > RTU_SENSOR_FAULT_VALUE && !(errFlags & 0x04))
  {
    snapshot.flow.value = flowVal;
    snapshot.flow.isValid = (flowVal <= MAX_FLOW_RATE);
    snapshot.flow.errorCode = snapshot.flow.isValid ? ERR_NONE : ERR_SENSOR_FAILURE;
  }
  else
  {
    snapshot.flow.isValid = false;
    snapshot.flow.errorCode = ERR_SENSOR_FAILURE;
  }
  snapshot.flow.timestamp = millis();

  // Level
  float levelVal = doc["level"] | RTU_SENSOR_FAULT_VALUE;
  if (levelVal > RTU_SENSOR_FAULT_VALUE && !(errFlags & 0x08))
  {
    snapshot.level.value = levelVal;
    snapshot.level.isValid = (levelVal >= MIN_WATER_LEVEL);
    snapshot.level.errorCode = snapshot.level.isValid ? ERR_NONE : ERR_WATER_LEVEL_LOW;
  }
  else
  {
    snapshot.level.isValid = false;
    snapshot.level.errorCode = ERR_SENSOR_FAILURE;
  }
  snapshot.level.timestamp = millis();

  snapshot.rtuOnline = true;
  lastRtuFrameTime = millis();

  publishSnapshot(snapshot);
}

void rtuTask(void *parameter)
{
  (void)parameter;
  esp_task_wdt_add(NULL);

  Serial1.begin(BAUD_RATE, SERIAL_8N1, PIN_RTU_RX, PIN_RTU_TX);

  static char rtuBuffer[BUFFER_SIZE];
  uint16_t rtuLen = 0;

  for (;;)
  {
    esp_task_wdt_reset();

    while (Serial1.available())
    {
      char c = Serial1.read();

      // Buffer overflow protection
      if (rtuLen >= BUFFER_SIZE - 1)
      {
        rtuLen = 0;
        continue;
      }

      rtuBuffer[rtuLen++] = c;
      rtuBuffer[rtuLen] = '\0';

      if (c == '}')
      {
        parseRtuFrame(rtuBuffer, rtuLen);
        rtuLen = 0;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(RTU_TASK_INTERVAL_MS));
  }
}
#endif // ENABLE_RTU_LINK

// ============== Edge Impulse Data Forwarder ==============

#if ENABLE_DATA_FORWARDER
void dataForwarderTask(void *parameter)
{
  (void)parameter;
  esp_task_wdt_add(NULL);

  // Wait a moment for serial to be ready
  vTaskDelay(pdMS_TO_TICKS(2000));

  for (;;)
  {
    esp_task_wdt_reset();

    TelemetrySnapshot snapshot = getSnapshotCopy();

    if (serialMutex != nullptr &&
        xSemaphoreTake(serialMutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
      comms.sendDataForwarderLine(
          snapshot.tds.value,
          snapshot.pressure.value,
          snapshot.flow.value,
          snapshot.level.value);
      xSemaphoreGive(serialMutex);
    }

    vTaskDelay(pdMS_TO_TICKS(DATA_FORWARDER_INTERVAL_MS));
  }
}
#endif // ENABLE_DATA_FORWARDER
