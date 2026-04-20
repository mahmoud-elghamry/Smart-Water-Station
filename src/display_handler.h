/**
 * Display Manager Module
 * Handles OLED display with different screens per state
 */

#ifndef DISPLAY_HANDLER_H
#define DISPLAY_HANDLER_H

#include "config.h"
#include "sensors.h"
#include "safety.h"
#include "state_machine.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

class DisplayManager {
public:
    bool begin() {
        Wire.begin(I2C_SDA, I2C_SCL);

        if (!_display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
            Serial.println("[ERR] OLED init failed");
            return false;
        }

        _showSplash();
        return true;
    }

    void showConnecting(bool wifiOk, bool mqttOk, int attempt) {
        _display.clearDisplay();
        _display.setTextSize(1);
        _display.setTextColor(SSD1306_WHITE);

        _display.setCursor(15, 0);
        _display.println("= CONNECTING =");

        _display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

        _display.setCursor(0, 16);
        _display.print("WiFi:  ");
        _display.println(wifiOk ? "OK" : "...");

        _display.setCursor(0, 28);
        _display.print("MQTT:  ");
        _display.println(mqttOk ? "OK" : "...");

        _display.setCursor(0, 44);
        _display.print("Attempt: ");
        _display.println(attempt);

        // Animated dots
        _display.setCursor(0, 56);
        int dots = (millis() / 500) % 4;
        for (int i = 0; i < dots; i++) _display.print(".");

        _display.display();
    }

    void showRunning(const SensorData& data, bool pumpOn, bool locked,
                     bool wifiOk, bool mqttOk) {
        _display.clearDisplay();
        _display.setTextSize(1);
        _display.setTextColor(SSD1306_WHITE);

        // Header
        _display.setCursor(0, 0);
        _display.print("W:");
        _display.print(wifiOk ? "OK" : "--");
        _display.print(" M:");
        _display.print(mqttOk ? "OK" : "--");

        // Uptime on right
        _display.setCursor(80, 0);
        unsigned long secs = millis() / 1000;
        _display.print(secs / 60);
        _display.print(":");
        if (secs % 60 < 10) _display.print("0");
        _display.print(secs % 60);

        _display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

        // pH
        _display.setCursor(0, 13);
        _display.print("pH:   ");
        _display.print(data.ph, 1);
        _display.setCursor(75, 13);
        _printStatus(data.ph, PH_CRITICAL_MIN, PH_SAFE_MIN,
                     PH_SAFE_MAX, PH_CRITICAL_MAX);

        // Turbidity
        _display.setCursor(0, 23);
        _display.print("Turb: ");
        _display.print(data.turbidity, 0);
        _display.setCursor(75, 23);
        if (data.turbidity > TURBIDITY_CRITICAL)
            _display.print("[BAD]");
        else if (data.turbidity > TURBIDITY_SAFE)
            _display.print("[HI]");
        else
            _display.print("[OK]");

        // Flow
        _display.setCursor(0, 33);
        _display.print("Flow: ");
        _display.print(data.flowRate, 1);
        _display.setCursor(75, 33);
        if (data.flowRate < FLOW_CRITICAL_MIN && pumpOn)
            _display.print("[DRY]");
        else if (data.flowRate < FLOW_NORMAL_MIN)
            _display.print("[LOW]");
        else
            _display.print("[OK]");

        // Temperature
        _display.setCursor(0, 43);
        _display.print("Temp: ");
        if (data.temperature > -100) {
            _display.print(data.temperature, 1);
            _display.print("C");
        } else {
            _display.print("ERR");
        }
        _display.setCursor(75, 43);
        if (data.temperature > TEMP_CRITICAL_MAX || data.temperature < TEMP_CRITICAL_MIN)
            _display.print("[BAD]");
        else if (data.temperature > TEMP_SAFE_MAX || data.temperature < TEMP_SAFE_MIN)
            _display.print("[HI]");
        else
            _display.print("[OK]");

        _display.drawLine(0, 53, 128, 53, SSD1306_WHITE);

        // Pump status
        _display.setCursor(0, 56);
        _display.print("PUMP: ");
        if (locked)
            _display.print("LOCKED");
        else if (pumpOn)
            _display.print("ON");
        else
            _display.print("OFF");

        _display.display();
    }

    void showAlert(const SensorData& data, const char* alertName) {
        _display.clearDisplay();
        _display.setTextSize(1);
        _display.setTextColor(SSD1306_WHITE);

        // Flashing header
        bool flash = (millis() / 500) % 2;
        if (flash) {
            _display.fillRect(0, 0, 128, 12, SSD1306_WHITE);
            _display.setTextColor(SSD1306_BLACK);
            _display.setCursor(10, 2);
            _display.print("!! SAFETY ALERT !!");
            _display.setTextColor(SSD1306_WHITE);
        } else {
            _display.setCursor(10, 2);
            _display.print("!! SAFETY ALERT !!");
        }

        _display.setCursor(0, 18);
        _display.print("Alert: ");
        _display.println(alertName);

        _display.setCursor(0, 30);
        _display.print("pH:");
        _display.print(data.ph, 1);
        _display.print(" T:");
        _display.print(data.turbidity, 0);

        _display.setCursor(0, 40);
        _display.print("F:");
        _display.print(data.flowRate, 1);
        _display.print(" Tmp:");
        _display.print(data.temperature, 1);

        _display.setCursor(0, 54);
        _display.print("PUMP: LOCKED");

        _display.display();
    }

    void showError(const char* message) {
        _display.clearDisplay();
        _display.setTextSize(1);
        _display.setTextColor(SSD1306_WHITE);

        _display.setCursor(25, 10);
        _display.println("=== ERROR ===");

        _display.setCursor(0, 30);
        _display.println(message);

        _display.setCursor(0, 50);
        _display.println("Restarting...");

        _display.display();
    }

private:
    Adafruit_SSD1306 _display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

    void _showSplash() {
        _display.clearDisplay();
        _display.setTextSize(1);
        _display.setTextColor(SSD1306_WHITE);

        _display.setCursor(15, 15);
        _display.println(SYSTEM_NAME);

        _display.setCursor(35, 30);
        _display.print("v");
        _display.println(SYSTEM_VERSION);

        _display.setCursor(20, 48);
        _display.println("Starting...");

        _display.display();
    }

    void _printStatus(float val, float critMin, float safeMin,
                      float safeMax, float critMax) {
        if (val < critMin || val > critMax)
            _display.print("[BAD]");
        else if (val < safeMin || val > safeMax)
            _display.print("[LOW]");
        else
            _display.print("[OK]");
    }
};

#endif
