#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

#include "AppTypes.h"
#include <Arduino.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

typedef TelemetrySnapshot (*SnapshotProvider)();
typedef bool (*CommandPublisher)(const ControlCommand &command);

class WebDashboard
{
public:
  WebDashboard();
  void begin(SnapshotProvider snapshotProvider, CommandPublisher commandPublisher);
  void loop();

private:
  WebServer _server;
  WebSocketsServer _ws;
  SnapshotProvider _snapshotProvider;
  CommandPublisher _commandPublisher;
  unsigned long _lastWsBroadcast;
  bool _fsReady;

  void registerRoutes();
  String buildStatusJson();
  void handleStatus();
  void handleControl();
  void handleConfig();
  void handleAi();
  void handleHealth();
  void handleNotFound();
  void broadcastStatusIfDue();
  void sendJson(int code, const String &payload);
  bool serveStaticFile(String path);
  String contentTypeFor(const String &path);
  bool parseAction(const char *cmd, ControlAction &action);

  // WebSocket event handler for bidirectional communication
  void onWsEvent(uint8_t clientNum, WStype_t type, uint8_t *payload, size_t length);
};

#endif // WEB_DASHBOARD_H
