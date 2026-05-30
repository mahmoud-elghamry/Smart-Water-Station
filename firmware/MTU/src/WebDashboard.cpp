#include "WebDashboard.h"
#include "Config.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <cstring>

WebDashboard::WebDashboard()
    : _server(80), _ws(81), _snapshotProvider(nullptr),
      _commandPublisher(nullptr), _lastWsBroadcast(0), _fsReady(false)
{
}

void WebDashboard::begin(SnapshotProvider snapshotProvider,
                         CommandPublisher commandPublisher)
{
  _snapshotProvider = snapshotProvider;
  _commandPublisher = commandPublisher;

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL, 0,
              WIFI_AP_MAX_CLIENTS);

  _fsReady = LittleFS.begin(true);

  registerRoutes();
  _server.begin();

  // Register WebSocket event handler for bidirectional communication
  _ws.onEvent([this](uint8_t clientNum, WStype_t type, uint8_t *payload,
                     size_t length) {
    onWsEvent(clientNum, type, payload, length);
  });
  _ws.begin();
}

void WebDashboard::loop()
{
  _server.handleClient();
  _ws.loop();
  broadcastStatusIfDue();
}

void WebDashboard::onWsEvent(uint8_t clientNum, WStype_t type,
                              uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_CONNECTED:
  {
    // Send current status to newly connected client
    String status = buildStatusJson();
    _ws.sendTXT(clientNum, status);
    break;
  }

  case WStype_TEXT:
  {
    // Parse incoming WebSocket command (same format as REST /api/control)
    if (!_commandPublisher || length == 0)
    {
      break;
    }

    JsonDocument doc;
    DeserializationError parseErr =
        deserializeJson(doc, (const char *)payload, length);
    if (parseErr)
    {
      String errMsg = "{\"ok\":false,\"error\":\"invalid_json\"}";
      _ws.sendTXT(clientNum, errMsg);
      break;
    }

    const char *cmd = doc["cmd"] | "";
    ControlCommand command;
    command.source = SOURCE_WEB;
    command.timestamp = millis();

    if (!parseAction(cmd, command.action))
    {
      String errMsg = "{\"ok\":false,\"error\":\"unknown_command\"}";
      _ws.sendTXT(clientNum, errMsg);
      break;
    }

    if (doc["state"].is<bool>())
    {
      command.state = doc["state"].as<bool>();
    }
    else if (doc["state"].is<const char *>())
    {
      const char *state = doc["state"];
      command.state =
          strcmp(state, "ON") == 0 || strcmp(state, "on") == 0 ||
          strcmp(state, "true") == 0 || strcmp(state, "1") == 0;
    }

    if (!doc["value"].isNull())
    {
      command.value = doc["value"].as<float>();
    }

    bool queued = _commandPublisher(command);
    String ack = queued ? "{\"ok\":true}"
                        : "{\"ok\":false,\"error\":\"queue_full\"}";
    _ws.sendTXT(clientNum, ack);
    break;
  }

  case WStype_DISCONNECTED:
  case WStype_ERROR:
  case WStype_PING:
  case WStype_PONG:
  default:
    break;
  }
}

void WebDashboard::registerRoutes()
{
  _server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
  _server.on("/api/control", HTTP_POST, [this]() { handleControl(); });
  _server.on("/api/config", HTTP_GET, [this]() { handleConfig(); });
  _server.on("/api/ai", HTTP_GET, [this]() { handleAi(); });
  _server.on("/health", HTTP_GET, [this]() { handleHealth(); });
  _server.onNotFound([this]() { handleNotFound(); });
}

String WebDashboard::buildStatusJson()
{
  TelemetrySnapshot snapshot =
      _snapshotProvider ? _snapshotProvider() : TelemetrySnapshot();

  JsonDocument doc;
  doc["device"] = "Smart Water Station";
  doc["firmware"] = FIRMWARE_VERSION;
  doc["uptime"] = snapshot.uptime;
  doc["state"] = stateToString(snapshot.state);
  doc["stateCode"] = static_cast<int>(snapshot.state);
  doc["error"] = errorToString(snapshot.error);
  doc["errorCode"] = static_cast<int>(snapshot.error);

  JsonObject network = doc["network"].to<JsonObject>();
  network["mode"] = "AP";
  network["ssid"] = WIFI_AP_SSID;
  network["ip"] = WiFi.softAPIP().toString();

  JsonObject actuators = doc["actuators"].to<JsonObject>();
  actuators["pump"] = snapshot.pumpRunning;
  actuators["valve"] = snapshot.valveOpen;

  JsonObject sensors = doc["sensors"].to<JsonObject>();
  auto addSensor = [&](const char* name, const SensorReading &r) {
    JsonObject s = sensors[name].to<JsonObject>();
    s["value"] = r.value;
    s["valid"] = r.isValid;
    s["error"] = errorToString(r.errorCode);
  };
  addSensor("turb1", snapshot.turb1);
  addSensor("turb2", snapshot.turb2);
  addSensor("ph1", snapshot.ph1);
  addSensor("ph2", snapshot.ph2);
  addSensor("flow1", snapshot.flow1);
  addSensor("flow2", snapshot.flow2);
  addSensor("press1", snapshot.pressure1);
  addSensor("press2", snapshot.pressure2);
  addSensor("temp1", snapshot.temp1);
  addSensor("temp2", snapshot.temp2);
  addSensor("pump_current", snapshot.pumpCurrent);

  JsonObject rtu = doc["rtu"].to<JsonObject>();
  rtu["enabled"] = static_cast<bool>(ENABLE_RTU_LINK);
  rtu["online"] = snapshot.rtuOnline;

  JsonObject ai = doc["ai"].to<JsonObject>();
  ai["enabled"] = static_cast<bool>(ENABLE_EDGE_IMPULSE);
  ai["available"] = snapshot.ai.available;
  ai["label"] = snapshot.ai.label;
  ai["confidence"] = snapshot.ai.confidence;
  ai["fault"] = snapshot.ai.fault;

  String payload;
  serializeJson(doc, payload);
  return payload;
}

void WebDashboard::handleStatus()
{
  String payload = buildStatusJson();
  sendJson(200, payload);
}

void WebDashboard::handleControl()
{
  if (!_commandPublisher)
  {
    sendJson(503, "{\"ok\":false,\"error\":\"command_queue_unavailable\"}");
    return;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, _server.arg("plain"));
  if (err)
  {
    sendJson(400, "{\"ok\":false,\"error\":\"invalid_json\"}");
    return;
  }

  const char *cmd = doc["cmd"] | "";
  ControlCommand command;
  command.source = SOURCE_WEB;
  command.timestamp = millis();

  if (!parseAction(cmd, command.action))
  {
    sendJson(400, "{\"ok\":false,\"error\":\"unknown_command\"}");
    return;
  }

  if (doc["state"].is<bool>())
  {
    command.state = doc["state"].as<bool>();
  }
  else if (doc["state"].is<const char *>())
  {
    const char *state = doc["state"];
    command.state =
        strcmp(state, "ON") == 0 || strcmp(state, "on") == 0 ||
        strcmp(state, "true") == 0 || strcmp(state, "1") == 0;
  }

  if (!doc["value"].isNull())
  {
    command.value = doc["value"].as<float>();
  }

  bool queued = _commandPublisher(command);
  sendJson(queued ? 202 : 429,
           queued ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"queue_full\"}");
}

void WebDashboard::handleConfig()
{
  JsonDocument doc;
  doc["firmware"] = FIRMWARE_VERSION;
  doc["simulationMode"] = static_cast<bool>(SIMULATION_MODE);
  doc["webDashboard"] = static_cast<bool>(ENABLE_WEB_DASHBOARD);
  doc["rtuLink"] = static_cast<bool>(ENABLE_RTU_LINK);
  doc["edgeImpulse"] = static_cast<bool>(ENABLE_EDGE_IMPULSE);
  doc["hmiDisplay"] = static_cast<bool>(ENABLE_HMI_DISPLAY);
  doc["dataForwarder"] = static_cast<bool>(ENABLE_DATA_FORWARDER);
  doc["apSsid"] = WIFI_AP_SSID;

  JsonObject thresholds = doc["thresholds"].to<JsonObject>();
  thresholds["maxTurbidity"] = MAX_TURBIDITY;
  thresholds["maxPH"] = MAX_PH;
  thresholds["minPH"] = MIN_PH;
  thresholds["maxPressure"] = MAX_PRESSURE;
  thresholds["minPressure"] = MIN_PRESSURE;
  thresholds["maxFlowRate"] = MAX_FLOW_RATE;
  thresholds["maxTemperature"] = MAX_TEMPERATURE;
  thresholds["minTemperature"] = MIN_TEMPERATURE;
  thresholds["maxPumpCurrent"] = MAX_PUMP_CURRENT;

  String payload;
  serializeJson(doc, payload);
  sendJson(200, payload);
}

void WebDashboard::handleAi()
{
  TelemetrySnapshot snapshot =
      _snapshotProvider ? _snapshotProvider() : TelemetrySnapshot();

  JsonDocument doc;
  doc["enabled"] = static_cast<bool>(ENABLE_EDGE_IMPULSE);
  doc["available"] = snapshot.ai.available;
  doc["label"] = snapshot.ai.label;
  doc["confidence"] = snapshot.ai.confidence;
  doc["fault"] = snapshot.ai.fault;
  doc["timestamp"] = snapshot.ai.timestamp;

  String payload;
  serializeJson(doc, payload);
  sendJson(200, payload);
}

void WebDashboard::handleHealth()
{
  JsonDocument doc;
  doc["ok"] = true;
  doc["fsReady"] = _fsReady;
  doc["firmware"] = FIRMWARE_VERSION;
  doc["ip"] = WiFi.softAPIP().toString();
  doc["webSocketPort"] = 81;
  doc["dataForwarder"] = static_cast<bool>(ENABLE_DATA_FORWARDER);
  doc["pumpCurrentSensor"] = static_cast<bool>(ENABLE_PUMP_CURRENT_SENSOR);

  String payload;
  serializeJson(doc, payload);
  sendJson(200, payload);
}

void WebDashboard::handleNotFound()
{
  String path = _server.uri();
  if (path.startsWith("/api/"))
  {
    sendJson(404, "{\"ok\":false,\"error\":\"not_found\"}");
    return;
  }

  if (!serveStaticFile(path))
  {
    sendJson(404, "{\"ok\":false,\"error\":\"file_not_found\"}");
  }
}

void WebDashboard::broadcastStatusIfDue()
{
  unsigned long now = millis();
  if (now - _lastWsBroadcast < TELEMETRY_INTERVAL)
  {
    return;
  }

  String payload = buildStatusJson();
  _ws.broadcastTXT(payload);
  _lastWsBroadcast = now;
}

void WebDashboard::sendJson(int code, const String &payload)
{
  _server.sendHeader("Access-Control-Allow-Origin", "*");
  _server.send(code, "application/json", payload);
}

bool WebDashboard::serveStaticFile(String path)
{
  if (!_fsReady)
  {
    return false;
  }

  if (path == "/")
  {
    path = "/index.html";
  }

  String actualPath = path;
  bool gzipped = false;

  if (LittleFS.exists(path + ".gz"))
  {
    actualPath = path + ".gz";
    gzipped = true;
  }
  else if (!LittleFS.exists(actualPath))
  {
    if (LittleFS.exists("/index.html.gz"))
    {
      actualPath = "/index.html.gz";
      path = "/index.html";
      gzipped = true;
    }
    else if (LittleFS.exists("/index.html"))
    {
      actualPath = "/index.html";
      path = "/index.html";
    }
    else
    {
      return false;
    }
  }

  File file = LittleFS.open(actualPath, "r");
  if (!file)
  {
    return false;
  }

  if (gzipped)
  {
    _server.sendHeader("Content-Encoding", "gzip");
  }

  _server.streamFile(file, contentTypeFor(path));
  file.close();
  return true;
}

String WebDashboard::contentTypeFor(const String &path)
{
  if (path.endsWith(".html"))
    return "text/html";
  if (path.endsWith(".css"))
    return "text/css";
  if (path.endsWith(".js"))
    return "application/javascript";
  if (path.endsWith(".json"))
    return "application/json";
  if (path.endsWith(".png"))
    return "image/png";
  if (path.endsWith(".jpg") || path.endsWith(".jpeg"))
    return "image/jpeg";
  if (path.endsWith(".svg"))
    return "image/svg+xml";
  if (path.endsWith(".ico"))
    return "image/x-icon";
  return "application/octet-stream";
}

bool WebDashboard::parseAction(const char *cmd, ControlAction &action)
{
  if (strcmp(cmd, "SET_PUMP") == 0)
    action = ACTION_SET_PUMP;
  else if (strcmp(cmd, "SET_VALVE") == 0)
    action = ACTION_SET_VALVE;
  else if (strcmp(cmd, "EMERGENCY_STOP") == 0)
    action = ACTION_EMERGENCY_STOP;
  else if (strcmp(cmd, "RESET") == 0)
    action = ACTION_RESET;
  else if (strcmp(cmd, "CALIBRATE_TDS") == 0)
    action = ACTION_CALIBRATE_TDS;
  else if (strcmp(cmd, "CALIBRATE_PRESSURE") == 0)
    action = ACTION_CALIBRATE_PRESSURE;
  else if (strcmp(cmd, "SET_MODE") == 0)
    action = ACTION_SET_MAINTENANCE;
  else
    return false;

  return true;
}
