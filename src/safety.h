/**
 * Safety System Module
 * Monitors sensor thresholds, controls pump lockout, and buzzer alerts
 */

#ifndef SAFETY_H
#define SAFETY_H

#include "config.h"
#include "sensors.h"
#include <Arduino.h>

enum AlertCode {
    ALERT_NONE          = 0,
    ALERT_PH_LOW        = 1,
    ALERT_PH_HIGH       = 2,
    ALERT_TURBIDITY     = 3,
    ALERT_DRY_RUN       = 4,
    ALERT_TEMP_LOW      = 5,
    ALERT_TEMP_HIGH     = 6
};

class SafetySystem {
public:
    void begin() {
        pinMode(PUMP_RELAY_PIN, OUTPUT);
        pinMode(BUZZER_PIN, OUTPUT);
        pinMode(RGB_RED_PIN, OUTPUT);
        pinMode(RGB_GREEN_PIN, OUTPUT);
        pinMode(RGB_BLUE_PIN, OUTPUT);
        pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

        setPump(false);
        _setBuzzer(false);
        setRGB(0, 0, 0);
    }

    /**
     * Check safety thresholds. Returns current alert code.
     */
    AlertCode check(const SensorData& data) {
        AlertCode alert = ALERT_NONE;

        if (data.ph < PH_CRITICAL_MIN) {
            alert = ALERT_PH_LOW;
        } else if (data.ph > PH_CRITICAL_MAX) {
            alert = ALERT_PH_HIGH;
        } else if (data.turbidity > TURBIDITY_CRITICAL) {
            alert = ALERT_TURBIDITY;
        } else if (_pumpOn && data.flowRate < FLOW_CRITICAL_MIN) {
            alert = ALERT_DRY_RUN;
        } else if (data.temperature > TEMP_CRITICAL_MAX) {
            alert = ALERT_TEMP_HIGH;
        } else if (data.temperature < TEMP_CRITICAL_MIN && data.temperature > -100) {
            alert = ALERT_TEMP_LOW;
        }

        // Danger detected
        if (alert != ALERT_NONE) {
            if (_pumpOn) {
                setPump(false);
                Serial.println("[!] Emergency stop");
            }
            _locked = true;
            _alertCode = alert;

            // Buzzer alarm (pulsing)
            unsigned long now = millis();
            if (now - _lastBuzzerToggle >= 300) {
                _buzzerState = !_buzzerState;
                _setBuzzer(_buzzerState);
                _lastBuzzerToggle = now;
            }

            // Red LED
            setRGB(255, 0, 0);
        }
        // Check auto-unlock
        else if (_locked) {
            bool phOk = (data.ph >= PH_SAFE_MIN && data.ph <= PH_SAFE_MAX);
            bool turbOk = (data.turbidity < TURBIDITY_SAFE);
            bool flowOk = (data.flowRate >= FLOW_NORMAL_MIN || !_pumpOn);
            bool tempOk = (data.temperature >= TEMP_SAFE_MIN && data.temperature <= TEMP_SAFE_MAX);

            if (phOk && turbOk && flowOk && tempOk) {
                _locked = false;
                _alertCode = ALERT_NONE;
                _setBuzzer(false);
                Serial.println("[OK] Auto-unlocked");
            } else {
                // Yellow LED = recovering
                setRGB(255, 150, 0);
                _setBuzzer(false);
            }
        }
        // All good
        else {
            setRGB(0, 255, 0);
        }

        return _alertCode;
    }

    /**
     * Check reset button (with debounce)
     */
    bool checkResetButton() {
        if (digitalRead(RESET_BUTTON_PIN) == LOW) {
            unsigned long now = millis();
            if (now - _lastButtonPress >= DEBOUNCE_MS) {
                _lastButtonPress = now;
                if (_locked) {
                    manualReset();
                    return true;
                }
            }
        }
        return false;
    }

    void manualReset() {
        _locked = false;
        _alertCode = ALERT_NONE;
        _setBuzzer(false);
        Serial.println("[OK] Manual reset");
    }

    void setPump(bool on) {
        if (on && _locked) return;
        _pumpOn = on;
        digitalWrite(PUMP_RELAY_PIN, on ? HIGH : LOW);
    }

    void setRGB(uint8_t r, uint8_t g, uint8_t b) {
        analogWrite(RGB_RED_PIN, r);
        analogWrite(RGB_GREEN_PIN, g);
        analogWrite(RGB_BLUE_PIN, b);
    }

    const char* getAlertName() const {
        switch (_alertCode) {
            case ALERT_NONE:      return "NONE";
            case ALERT_PH_LOW:    return "pH LOW";
            case ALERT_PH_HIGH:   return "pH HIGH";
            case ALERT_TURBIDITY: return "TURBIDITY";
            case ALERT_DRY_RUN:   return "DRY RUN";
            case ALERT_TEMP_LOW:  return "TEMP LOW";
            case ALERT_TEMP_HIGH: return "TEMP HIGH";
            default:              return "UNKNOWN";
        }
    }

    bool isPumpOn() const { return _pumpOn; }
    bool isLocked() const { return _locked; }
    AlertCode getAlertCode() const { return _alertCode; }

private:
    bool _pumpOn = false;
    bool _locked = false;
    AlertCode _alertCode = ALERT_NONE;
    bool _buzzerState = false;
    unsigned long _lastBuzzerToggle = 0;
    unsigned long _lastButtonPress = 0;

    void _setBuzzer(bool on) {
        _buzzerState = on;
        if (on) {
            tone(BUZZER_PIN, 2000);
        } else {
            noTone(BUZZER_PIN);
        }
    }
};

#endif
