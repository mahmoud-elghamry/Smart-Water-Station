/**
 * Sensor Manager Module
 * Handles reading and averaging all sensors
 */

#ifndef SENSORS_H
#define SENSORS_H

#include "config.h"
#include <Arduino.h>

struct SensorData {
    float ph;
    float turbidity;
    float flowRate;
    float temperature;
};

class SensorManager {
public:
    void begin() {
        analogReadResolution(12);
        analogSetAttenuation(ADC_11db);

        pinMode(ULTRASONIC_TRIG, OUTPUT);
        pinMode(ULTRASONIC_ECHO, INPUT);

        digitalWrite(ULTRASONIC_TRIG, LOW);
    }

    SensorData read() {
        SensorData data;
        data.ph = _readPH();
        data.turbidity = _readTurbidity();
        data.flowRate = _readFlowRate();
        data.temperature = _readTemperature();
        return data;
    }

private:
    // pH: potentiometer 0-4095 → 0-14, averaged
    float _readPH() {
        float sum = 0;
        for (int i = 0; i < SENSOR_SAMPLES; i++) {
            sum += analogRead(PH_SENSOR_PIN);
            delayMicroseconds(200);
        }
        float avg = sum / SENSOR_SAMPLES;
        return (avg / 4095.0f) * 14.0f;
    }

    // Turbidity: LDR high light = clear water, averaged
    float _readTurbidity() {
        float sum = 0;
        for (int i = 0; i < SENSOR_SAMPLES; i++) {
            sum += analogRead(TURBIDITY_LDR_PIN);
            delayMicroseconds(200);
        }
        float avg = sum / SENSOR_SAMPLES;
        return (1.0f - (avg / 4095.0f)) * 1000.0f;
    }

    // Flow rate: ultrasonic distance → flow
    float _readFlowRate() {
        float dist = _readUltrasonic();
        if (dist < 2) dist = 2;
        if (dist > 400) dist = 400;

        float flow = 20.0f - ((dist - 2.0f) / 398.0f * 20.0f);
        return max(0.0f, flow);
    }

    // Temperature: NTC thermistor via Steinhart-Hart
    float _readTemperature() {
        float sum = 0;
        for (int i = 0; i < SENSOR_SAMPLES; i++) {
            sum += analogRead(TEMP_NTC_PIN);
            delayMicroseconds(200);
        }
        float avg = sum / SENSOR_SAMPLES;

        if (avg <= 0 || avg >= 4095) return -999.0f;

        float resistance = NTC_SERIES_R / ((4095.0f / avg) - 1.0f);

        float steinhart = log(resistance / NTC_NOMINAL_R);
        steinhart /= NTC_B_COEFFICIENT;
        steinhart += 1.0f / (NTC_NOMINAL_TEMP + 273.15f);
        steinhart = 1.0f / steinhart;
        steinhart -= 273.15f;

        return steinhart;
    }

    float _readUltrasonic() {
        digitalWrite(ULTRASONIC_TRIG, LOW);
        delayMicroseconds(2);
        digitalWrite(ULTRASONIC_TRIG, HIGH);
        delayMicroseconds(10);
        digitalWrite(ULTRASONIC_TRIG, LOW);

        long duration = pulseIn(ULTRASONIC_ECHO, HIGH, 30000);
        return duration * 0.034f / 2.0f;
    }
};

#endif
