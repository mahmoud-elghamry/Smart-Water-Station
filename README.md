# SmarterWater

Water quality monitoring system using ESP32 with automated safety features and AI-based human detection.

## Project Structure

```
S1/
├── src/
│   ├── config.h          # Pin definitions and settings
│   └── main.cpp          # ESP32 firmware
├── ai/
│   ├── AI.py             # Face detection safety system
│   └── requirements.txt  # Python dependencies
├── web/
│   └── index.html        # Dashboard
├── diagram.json          # Wokwi simulation
└── platformio.ini        # Build config
```

## Hardware

| Component | Pin | Description |
|-----------|-----|-------------|
| pH Sensor | GPIO 34 | Analog (potentiometer in simulation) |
| Turbidity | GPIO 35 | LDR photoresistor |
| Flow Sensor | GPIO 32/33 | HC-SR04 ultrasonic |
| Pump Relay | GPIO 2 | ON/OFF control |
| OLED Display | I2C 21/22 | SSD1306 128x64 |

## Requirements

### ESP32 Firmware
- PlatformIO or Arduino IDE
- Libraries: Adafruit SSD1306, ArduinoJson, PubSubClient

### AI Safety System
- Python 3.10 or higher
- Webcam

Install Python dependencies:
```bash
cd ai
pip install -r requirements.txt
```

## Running the Simulation

### 1. Start Wokwi
Open the project in Wokwi or run locally with PlatformIO.

### 2. Open Dashboard
Open `web/index.html` in browser and click "Connect to MQTT".

### 3. Run AI System (optional)
```bash
cd ai
python AI.py
```
The camera window shows detection status. Press 'q' to exit.

## MQTT Communication

Broker: `broker.hivemq.com` (port 1883)

| Topic | Direction | Data |
|-------|-----------|------|
| grad/project/sensors | ESP32 -> Dashboard | pH, turbidity, flowRate, pump status |
| grad/project/control | Dashboard -> ESP32 | PUMP_ON, PUMP_OFF, RESET_SAFETY |
| grad/project/alerts | ESP32 -> Dashboard | Safety alerts |

## Safety Thresholds

| Parameter | Safe Range | Critical |
|-----------|-----------|----------|
| pH | 6.5 - 8.5 | < 6.0 or > 9.0 |
| Turbidity | < 500 NTU | > 800 NTU |
| Flow Rate | > 5 L/min | < 2 L/min (dry run) |

When critical values are detected, the pump stops automatically and enters lockout mode.

## Troubleshooting

**MQTT not connecting:** Check internet connection. The broker may be temporarily down.

**Camera not opening:** Make sure no other app is using the webcam.

**Python import errors:** Run `pip install -r requirements.txt` in the ai folder.

**Wokwi simulation slow:** Normal behavior - real hardware is faster.
