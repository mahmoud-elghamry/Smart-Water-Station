# SmarterWater v2.0

Water quality monitoring system built on ESP32. Reads pH, turbidity, flow rate, and temperature sensors. Automatically stops the pump when readings go critical. Includes a web dashboard for remote monitoring and an AI module that detects humans and stops the pump for safety.

## Architecture

All components communicate through the MQTT broker:

```
                    ┌──────────────────────┐
                    │   MQTT Broker        │
                    │ broker.hivemq.com    │
                    └──────────┬───────────┘
                               │
          ┌────────────────────┼────────────────────┐
          │                    │                    │
          ▼                    ▼                    ▼
┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
│  ESP32 + OLED   │  │  Web Dashboard  │  │   AI Module     │
│                 │  │                 │  │                 │
│ - 4 Sensors     │  │ - Live data     │  │ - Face detection│
│ - State Machine │  │ - Manual control│  │ - MQTT commands │
│ - Safety System │  │ - Subscribes    │  │ - Statistics    │
│ - RGB + Buzzer  │  │                 │  │                 │
└─────────────────┘  └─────────────────┘  └─────────────────┘
         │
         ▼
   ┌──────────┐
   │ Sensors  │  pH, Turbidity, Flow, Temperature
   │ Outputs  │  Relay, Buzzer, RGB LED
   └──────────┘
```

## State Machine

```
  ┌──────┐     WiFi+MQTT OK     ┌─────────┐
  │ INIT │ ──────────────────── │ CONNECT  │
  └──────┘                      └────┬─────┘
                                     │
                    ┌────────────────┤
                    │                ▼
              ┌─────┴────┐    ┌──────────┐
              │  ALERT   │◄───│ RUNNING  │
              │          │───►│          │
              └──────────┘    └──────────┘
              Auto-recover      Danger
              or Reset btn     detected

              ┌──────────┐
              │  ERROR   │ → ESP.restart()
              └──────────┘
```

## Project Structure

```
├── src/
│   ├── main.cpp              # Entry point — thin orchestrator
│   ├── config.h              # All settings and pin definitions
│   ├── state_machine.h       # System state management
│   ├── sensors.h             # Sensor reading with averaging
│   ├── safety.h              # Threshold checks, pump lock, buzzer
│   ├── mqtt_handler.h        # MQTT with exponential backoff
│   └── display_handler.h     # OLED screens per state
├── ai/
│   ├── AI.py                 # Face detection (MQTT only)
│   ├── config.json           # AI module settings
│   ├── requirements.txt
│   └── haarcascade_frontalface_default.xml
├── web/
│   └── index.html            # Dashboard (MQTT WebSocket)
├── diagram.json              # Wokwi simulation circuit
├── wokwi.toml
└── platformio.ini
```

## Hardware Connections

| Component | Pin | Type | Notes |
|-----------|-----|------|-------|
| pH Sensor | GPIO 34 | Analog | Potentiometer simulation |
| Turbidity | GPIO 35 | Analog | LDR photoresistor |
| Temperature | GPIO 36 | Analog | NTC thermistor |
| Ultrasonic Trig | GPIO 32 | Digital | HC-SR04 trigger |
| Ultrasonic Echo | GPIO 33 | Digital | HC-SR04 echo |
| Pump Relay | GPIO 2 | Output | HIGH = pump on |
| Buzzer | GPIO 25 | Output | Alert sound |
| RGB LED Red | GPIO 26 | PWM | Status indicator |
| RGB LED Green | GPIO 27 | PWM | Status indicator |
| RGB LED Blue | GPIO 14 | PWM | Status indicator |
| Reset Button | GPIO 4 | Input | Manual safety reset |
| OLED SDA | GPIO 21 | I2C | Display data |
| OLED SCL | GPIO 22 | I2C | Display clock |

## Running the System

### ESP32 Firmware

```bash
pip install platformio
pio run -t upload
pio device monitor
```

The OLED will show "W:OK M:OK" when WiFi and MQTT are connected.

### Web Dashboard

Open `web/index.html` in a browser and click "Connect to MQTT Broker". It subscribes to sensor data and can send pump commands.

### AI Safety Module

```bash
cd ai
pip install -r requirements.txt
python AI.py
```

Opens your webcam. When a face is detected, sends PUMP_OFF via MQTT. When clear, sends PUMP_ON. Press 'q' to exit.

## MQTT Topics

Broker: `broker.hivemq.com`
- Port 1883 for ESP32 and Python
- Port 8000 (WebSocket) for browser

| Topic | Publisher | Subscriber | Data |
|-------|-----------|------------|------|
| `grad/project/sensors` | ESP32 | Dashboard, AI | JSON sensor readings |
| `grad/project/control` | Dashboard, AI | ESP32 | PUMP_ON, PUMP_OFF, RESET_SAFETY |
| `grad/project/alerts` | ESP32 | Dashboard | Safety alerts with sensor data |
| `grad/project/heartbeat` | ESP32 | Dashboard | Alive signal every 10s |
| `grad/project/status` | ESP32 | Dashboard | Full system status |

Sensor payload:
```json
{
  "ph": 7.2,
  "turbidity": 350,
  "flowRate": 8.3,
  "temperature": 24.5,
  "pump": 1,
  "locked": 0,
  "alert": 0,
  "uptime": 120
}
```

## Safety Thresholds

| Parameter | Safe | Critical | Action |
|-----------|------|----------|--------|
| pH | 6.5 - 8.5 | < 6.0 or > 9.0 | Pump locks |
| Turbidity | < 500 NTU | > 800 NTU | Pump locks |
| Flow Rate | > 5 L/min | < 2 L/min | Dry run protection |
| Temperature | 5 - 40 °C | < 2 or > 45 °C | Pump locks |

When any critical threshold is hit:
1. Pump stops and locks immediately
2. Buzzer sounds (pulsing alarm)
3. RGB LED turns red
4. Alert published to MQTT
5. Auto-unlocks when all values return to safe range, or press RESET button

## Firmware Modules

| Module | Responsibility |
|--------|---------------|
| `state_machine.h` | System states (INIT→CONNECTING→RUNNING↔ALERT→ERROR) |
| `sensors.h` | Read and average all 4 sensors |
| `safety.h` | Threshold checks, pump lockout, buzzer, RGB LED, reset button |
| `mqtt_handler.h` | WiFi/MQTT with exponential backoff, heartbeat, JSON publishing |
| `display_handler.h` | OLED screens per state (splash, connecting, running, alert, error) |

## Wokwi Simulation

Load `diagram.json` in Wokwi. The circuit includes:
- ESP32 DevKit V4
- SSD1306 OLED (I2C)
- Potentiometer (pH control)
- LDR module (turbidity)
- HC-SR04 ultrasonic (flow simulation)
- NTC thermistor (water temperature)
- Relay module (pump)
- Passive buzzer (alert sound)
- RGB LED (status colors: green/yellow/red)
- Push button (manual safety reset)

MQTT works with the public broker, so you can run the dashboard and AI module alongside the simulation.

## Dependencies

ESP32 (platformio.ini):
- PubSubClient 2.8
- Adafruit SSD1306 2.5.7
- Adafruit GFX 1.11.5
- ArduinoJson 6.21.3

Python (ai/requirements.txt):
- opencv-python >= 4.5.0
- paho-mqtt >= 1.6.0

## Troubleshooting

**OLED blank**: Check GPIO 21 (SDA) and 22 (SCL) connections.

**WiFi not connecting**: The default SSID is "Wokwi-GUEST" which only works in simulation. Change it in config.h for real hardware.

**MQTT disconnects**: Public broker can be unstable. The firmware uses exponential backoff — it will reconnect automatically.

**Camera not opening**: Close other apps using the webcam.

**Buzzer not sounding**: Check GPIO 25 connection. Wokwi uses passive buzzer with `tone()`.

**RGB LED wrong colors**: Verify GPIO 26 (R), 27 (G), 14 (B) connections.

## License

MIT
