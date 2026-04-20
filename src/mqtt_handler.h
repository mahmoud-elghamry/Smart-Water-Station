/**
 * MQTT Manager Module
 * Handles MQTT connection with exponential backoff and structured publishing
 */

#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include "config.h"
#include "sensors.h"
#include "safety.h"
#include "state_machine.h"
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Callback type for received commands
typedef void (*CommandCallback)(const char* command);

class MQTTManager {
public:
    void begin(PubSubClient& client, CommandCallback cmdCb) {
        _mqtt = &client;
        _cmdCallback = cmdCb;
        _clientId = "ESP32_" + String(random(0xFFFF), HEX);

        _mqtt->setServer(MQTT_BROKER, MQTT_PORT);
        _mqtt->setBufferSize(MQTT_BUFFER_SIZE);
        _mqtt->setCallback([this](char* topic, byte* payload, unsigned int len) {
            _onMessage(topic, payload, len);
        });
    }

    // ── WiFi ───────────────────────────────────────

    bool connectWiFi() {
        Serial.print("[WiFi] Connecting");
        WiFi.mode(WIFI_STA);
        WiFi.disconnect(true);
        delay(100);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED) {
            if (millis() - start >= WIFI_TIMEOUT_MS) {
                Serial.println(" TIMEOUT");
                return false;
            }
            delay(250);
            Serial.print(".");
        }

        Serial.print(" OK (");
        Serial.print(WiFi.localIP());
        Serial.println(")");
        _wifiOk = true;
        return true;
    }

    bool isWiFiConnected() {
        _wifiOk = (WiFi.status() == WL_CONNECTED);
        return _wifiOk;
    }

    // ── MQTT ───────────────────────────────────────

    bool connectMQTT() {
        if (!_wifiOk) return false;

        unsigned long now = millis();
        if (now - _lastRetry < _retryDelay) return false;
        _lastRetry = now;

        Serial.print("[MQTT] Connecting...");
        if (_mqtt->connect(_clientId.c_str())) {
            Serial.println(" OK");
            _mqtt->subscribe(TOPIC_CONTROL);
            _mqttOk = true;
            _retryDelay = MQTT_RETRY_BASE_MS;  // Reset backoff
            return true;
        }

        Serial.print(" FAIL (rc=");
        Serial.print(_mqtt->state());
        Serial.println(")");

        // Exponential backoff
        _retryDelay = min(_retryDelay * 2, (unsigned long)MQTT_RETRY_MAX_MS);
        Serial.print("[MQTT] Retry in ");
        Serial.print(_retryDelay / 1000);
        Serial.println("s");

        _mqttOk = false;
        return false;
    }

    bool loop() {
        if (_mqttOk) {
            if (!_mqtt->loop()) {
                _mqttOk = false;
                return false;
            }
        }
        return _mqttOk;
    }

    bool isMQTTConnected() const { return _mqttOk; }
    bool isWiFiOk() const { return _wifiOk; }

    // ── Publishing ─────────────────────────────────

    void publishSensors(const SensorData& data, bool pumpOn, bool locked, AlertCode alert) {
        if (!_mqttOk) return;

        StaticJsonDocument<256> doc;
        doc["ph"]          = round(data.ph * 100) / 100.0;
        doc["turbidity"]   = round(data.turbidity);
        doc["flowRate"]    = round(data.flowRate * 10) / 10.0;
        doc["temperature"] = round(data.temperature * 10) / 10.0;
        doc["pump"]        = pumpOn ? 1 : 0;
        doc["locked"]      = locked ? 1 : 0;
        doc["alert"]       = (int)alert;
        doc["uptime"]      = millis() / 1000;

        char buf[256];
        serializeJson(doc, buf);
        _mqtt->publish(TOPIC_SENSORS, buf);
    }

    void publishAlert(const SensorData& data, AlertCode alert, const char* alertName) {
        if (!_mqttOk) return;

        unsigned long now = millis();
        if (now - _lastAlert < ALERT_COOLDOWN) return;
        _lastAlert = now;

        StaticJsonDocument<256> doc;
        doc["type"]        = (int)alert;
        doc["name"]        = alertName;
        doc["ph"]          = data.ph;
        doc["turbidity"]   = data.turbidity;
        doc["flowRate"]    = data.flowRate;
        doc["temperature"] = data.temperature;
        doc["timestamp"]   = millis() / 1000;

        char buf[256];
        serializeJson(doc, buf);
        _mqtt->publish(TOPIC_ALERTS, buf);
    }

    void publishHeartbeat(const char* stateName) {
        if (!_mqttOk) return;

        unsigned long now = millis();
        if (now - _lastHeartbeat < HEARTBEAT_INTERVAL) return;
        _lastHeartbeat = now;

        StaticJsonDocument<128> doc;
        doc["status"]  = "alive";
        doc["state"]   = stateName;
        doc["uptime"]  = millis() / 1000;
        doc["version"] = SYSTEM_VERSION;
        doc["rssi"]    = WiFi.RSSI();

        char buf[128];
        serializeJson(doc, buf);
        _mqtt->publish(TOPIC_HEARTBEAT, buf);
    }

    void publishStatus(const char* stateName, const SensorData& data, bool pumpOn, bool locked) {
        if (!_mqttOk) return;

        StaticJsonDocument<384> doc;
        doc["state"]          = stateName;
        doc["wifi_rssi"]      = WiFi.RSSI();
        doc["uptime"]         = millis() / 1000;
        doc["pump"]           = pumpOn;
        doc["locked"]         = locked;

        JsonObject sensors    = doc.createNestedObject("sensors");
        sensors["ph"]         = round(data.ph * 100) / 100.0;
        sensors["turbidity"]  = round(data.turbidity);
        sensors["flowRate"]   = round(data.flowRate * 10) / 10.0;
        sensors["temperature"]= round(data.temperature * 10) / 10.0;

        char buf[384];
        serializeJson(doc, buf);
        _mqtt->publish(TOPIC_STATUS, buf);
    }

private:
    PubSubClient* _mqtt = nullptr;
    CommandCallback _cmdCallback = nullptr;
    String _clientId;
    bool _wifiOk = false;
    bool _mqttOk = false;

    // Backoff
    unsigned long _lastRetry = 0;
    unsigned long _retryDelay = MQTT_RETRY_BASE_MS;

    // Timing
    unsigned long _lastAlert = 0;
    unsigned long _lastHeartbeat = 0;

    void _onMessage(char* topic, byte* payload, unsigned int length) {
        char msg[64];
        int len = min((unsigned int)63, length);
        memcpy(msg, payload, len);
        msg[len] = '\0';

        Serial.print("[CMD] ");
        Serial.println(msg);

        if (_cmdCallback) {
            _cmdCallback(msg);
        }
    }
};

#endif
