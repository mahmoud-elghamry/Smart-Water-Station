/**
 * Smart Water Quality Monitoring System
 * ESP32 + MQTT + Safety System
 *
 * Main entry point — orchestrates all modules
 */

#include "config.h"
#include "state_machine.h"
#include "sensors.h"
#include "safety.h"
#include "mqtt_handler.h"
#include "display_handler.h"
#include <esp_task_wdt.h>

// Module instances
StateMachine  stateMachine;
SensorManager sensors;
SafetySystem  safety;
MQTTManager   network;
DisplayManager display;

WiFiClient    wifiClient;
PubSubClient  mqttClient(wifiClient);

SensorData    currentData;
unsigned long lastPublish = 0;
unsigned long lastDisplay = 0;

// ── Command handler ────────────────────────────────
void onCommand(const char* cmd) {
    if (strcmp(cmd, "PUMP_ON") == 0) {
        if (safety.isLocked()) {
            Serial.println("[!] Locked");
        } else {
            safety.setPump(true);
        }
    } else if (strcmp(cmd, "PUMP_OFF") == 0) {
        safety.setPump(false);
    } else if (strcmp(cmd, "RESET_SAFETY") == 0) {
        safety.manualReset();
    }
}

// ── Setup ──────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println();
    Serial.println("=== " SYSTEM_NAME " v" SYSTEM_VERSION " ===");

    // Init modules
    stateMachine.begin();
    sensors.begin();
    safety.begin();
    display.begin();
    network.begin(mqttClient, onCommand);

    // Watchdog
    esp_task_wdt_init(WDT_TIMEOUT_S, true);
    esp_task_wdt_add(NULL);

    stateMachine.transition(STATE_CONNECTING);
    Serial.println("[OK] Ready");
}

// ── Loop ───────────────────────────────────────────
void loop() {
    unsigned long now = millis();
    esp_task_wdt_reset();

    switch (stateMachine.getState()) {

        case STATE_CONNECTING: {
            bool wifiOk = network.isWiFiConnected();
            if (!wifiOk) wifiOk = network.connectWiFi();

            bool mqttOk = false;
            if (wifiOk) mqttOk = network.connectMQTT();

            display.showConnecting(wifiOk, mqttOk, 0);

            if (wifiOk && mqttOk) {
                stateMachine.transition(STATE_RUNNING);
            }
            break;
        }

        case STATE_RUNNING: {
            // Connection check
            if (!network.isWiFiConnected()) {
                stateMachine.transition(STATE_CONNECTING);
                break;
            }
            if (!network.loop()) {
                network.connectMQTT();
            }

            // Read sensors + check safety
            if (now - lastPublish >= PUBLISH_INTERVAL) {
                lastPublish = now;
                currentData = sensors.read();

                AlertCode alert = safety.check(currentData);

                if (alert != ALERT_NONE) {
                    stateMachine.transition(STATE_ALERT);
                    network.publishAlert(currentData, alert, safety.getAlertName());
                }

                network.publishSensors(currentData, safety.isPumpOn(),
                                       safety.isLocked(), alert);
                network.publishHeartbeat(stateMachine.getCurrentName());
            }

            // Reset button
            safety.checkResetButton();

            // Display
            if (now - lastDisplay >= DISPLAY_INTERVAL) {
                lastDisplay = now;
                display.showRunning(currentData, safety.isPumpOn(), safety.isLocked(),
                                    network.isWiFiOk(), network.isMQTTConnected());
            }
            break;
        }

        case STATE_ALERT: {
            // Keep connections alive
            if (network.isWiFiConnected()) network.loop();

            // Read sensors + check safety
            if (now - lastPublish >= PUBLISH_INTERVAL) {
                lastPublish = now;
                currentData = sensors.read();
                AlertCode alert = safety.check(currentData);

                // Auto-recover
                if (alert == ALERT_NONE && !safety.isLocked()) {
                    stateMachine.transition(STATE_RUNNING);
                }

                network.publishSensors(currentData, safety.isPumpOn(),
                                       safety.isLocked(), alert);
            }

            // Reset button
            if (safety.checkResetButton()) {
                stateMachine.transition(STATE_RUNNING);
            }

            // Display
            if (now - lastDisplay >= DISPLAY_INTERVAL) {
                lastDisplay = now;
                display.showAlert(currentData, safety.getAlertName());
            }
            break;
        }

        case STATE_ERROR: {
            display.showError("System Error");
            delay(3000);
            ESP.restart();
            break;
        }

        default:
            stateMachine.transition(STATE_CONNECTING);
            break;
    }
}