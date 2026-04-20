/**
 * State Machine Module
 * Manages system states and transitions
 */

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <Arduino.h>

enum SystemState {
    STATE_INIT,
    STATE_CONNECTING,
    STATE_RUNNING,
    STATE_ALERT,
    STATE_ERROR
};

class StateMachine {
public:
    void begin() {
        _state = STATE_INIT;
        _prevState = STATE_INIT;
        _stateStartTime = millis();
        _transitionCount = 0;
    }

    void transition(SystemState newState) {
        if (newState == _state) return;

        _prevState = _state;
        _state = newState;
        _stateStartTime = millis();
        _transitionCount++;

        Serial.print("[STATE] ");
        Serial.print(getName(_prevState));
        Serial.print(" -> ");
        Serial.println(getName(_state));
    }

    SystemState getState() const { return _state; }
    SystemState getPrevState() const { return _prevState; }

    const char* getName(SystemState s) const {
        switch (s) {
            case STATE_INIT:       return "INIT";
            case STATE_CONNECTING: return "CONNECTING";
            case STATE_RUNNING:    return "RUNNING";
            case STATE_ALERT:      return "ALERT";
            case STATE_ERROR:      return "ERROR";
            default:               return "UNKNOWN";
        }
    }

    const char* getCurrentName() const { return getName(_state); }

    unsigned long getStateDuration() const {
        return millis() - _stateStartTime;
    }

    unsigned long getTransitionCount() const { return _transitionCount; }

    bool is(SystemState s) const { return _state == s; }

private:
    SystemState _state = STATE_INIT;
    SystemState _prevState = STATE_INIT;
    unsigned long _stateStartTime = 0;
    unsigned long _transitionCount = 0;
};

#endif
