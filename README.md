# Smart Water Station

An IoT-based water quality monitoring and management system built around the ESP32 microcontroller. The system reads multiple water sensors in real time, applies safety logic at the edge, and streams data to a web dashboard over USB Serial. An AI-powered security module adds physical intrusion detection and can trigger emergency shutdowns autonomously.

---
## Project Structure

```
Smart-Water-Station/
├── firmware/
│   ├── src/
│   │   ├── main.cpp
│   │   ├── Config.h
│   │   ├── StateMachine.h
│   │   ├── SensorManager.cpp
│   │   ├── SensorManager.h
│   │   ├── CommsHandler.cpp
│   │   └── CommsHandler.h
│   ├── include/
│   ├── lib/
│   ├── test/
│   └── platformio.ini
├── ai/
│   ├── main.py
│   ├── security.py
│   ├── config.json
│   └── requirements.txt
├── web/
│   └── index.html
└── README.md
```

### Architecture

```
┌──────────────┐   USB Serial (JSON)   ┌──────────────────┐
│   ESP32      │◄─────────────────────►│  Web Dashboard   │
│  (Firmware)  │                       │  (Browser)       │
└──────┬───────┘                       └──────────────────┘
       │
       │  USB Serial (JSON)
       ▼
┌──────────────┐
│  AI Module   │
│  (Python)    │
└──────────────┘
```

The ESP32 acts as the single source of truth. Both the web dashboard and the AI module are clients — they read telemetry and send commands, but the firmware validates everything before execution.

---

## Sensors & Actuators

- **TDS Sensor** — Total Dissolved Solids (water purity measurement)
- **Pressure Sensor** — Line pressure monitoring
- **Flow Sensor** — Water flow rate tracking
- **Water Level Sensor** — Tank level detection
- **Water Pump** — Controlled via relay with software safety limits

---

## Firmware

The firmware is a PlatformIO project targeting the `esp32dev` board. It is structured into separate modules:

| File | Purpose |
|------|---------|
| `main.cpp` | Core loop, state machine orchestration |
| `Config.h` | Pin definitions, thresholds, timing constants |
| `StateMachine.h` | State definitions (IDLE, PUMP_RUNNING, EMERGENCY_STOP, ERROR) |
| `SensorManager.cpp/.h` | ADC reading, calibration, sensor data packaging |
| `CommsHandler.cpp/.h` | JSON serial communication, packet buffering |

### Key Design Decisions

- **Non-blocking** — Uses `millis()` timers exclusively; no `delay()` calls
- **Edge safety** — Sensor limits are checked locally every loop cycle, independent of any external client
- **Fail-safe** — Emergency stop state locks until a manual reset command is received
- **JSON protocol** — All serial communication uses structured JSON with error handling

### Build

Requires [PlatformIO](https://platformio.org/) (VS Code extension or CLI).

```bash
cd firmware
pio run            # compile
pio run -t upload  # flash to ESP32
```

---

## Web Dashboard

A single-page application (`web/index.html`) that connects to the ESP32 directly through the browser's Web Serial API. No server or backend required.

### Features

- Live sensor readings with color-coded status indicators
- Real-time chart history (Chart.js)
- Manual pump control (Start / Stop / Emergency Stop)
- Dark and light theme toggle
- In-app sensor calibration interface
- Connection status heartbeat monitoring
- Debug terminal showing raw serial traffic

### Usage

Open `web/index.html` in a Chromium-based browser (Chrome, Edge) and click **Connect Device**. Select the ESP32 COM port when prompted.

> **Note:** Web Serial API is not supported in Firefox or Safari.

---

## AI Security Module

A Python application that monitors a camera feed for unauthorized access. When a face is detected near the water station, it sends an emergency stop command to the ESP32 over serial.

### Features

- OpenCV-based face detection with configurable sensitivity
- Automatic serial reconnection on disconnect
- Anti-flooding logic — only sends commands on state changes
- Optional Flask REST API for remote status queries
- Configurable via `ai/config.json`

### Setup

```bash
cd ai
pip install -r requirements.txt
python main.py
```

### Configuration

All settings are in `ai/config.json`:

- **Serial** — COM port, baud rate, reconnection delay
- **Security** — Detection mode, sensitivity, camera index
- **Logging** — Log level, file rotation settings
- **API** — Enable/disable Flask REST endpoints

---

## Communication Protocol

All communication between the ESP32 and clients uses JSON over USB Serial at 115200 baud.

**Telemetry (ESP32 → Clients):**
```json
{
  "tds": 320,
  "pressure": 45.2,
  "flow": 12.8,
  "level": 78,
  "state": "IDLE",
  "pump": false
}
```

**Commands (Clients → ESP32):**
```json
{"cmd": "PUMP_START"}
{"cmd": "PUMP_STOP"}
{"cmd": "EMERGENCY_STOP", "state": "ON"}
{"cmd": "RESET"}
{"cmd": "CALIBRATE", "sensor": "tds", "value": 500}
```

---



---

## Safety Mechanisms

1. **Edge computing limits** — The ESP32 checks sensor thresholds locally (e.g., TDS > 500 PPM, Pressure > 100 PSI) and triggers protective actions without waiting for external commands
2. **State machine locking** — Emergency stop cannot be cleared by normal commands; requires explicit reset
3. **Anti-flooding** — The AI module tracks state changes and avoids spamming the serial bus
4. **Buffered communication** — Both the firmware and web dashboard reconstruct fragmented JSON packets to prevent parse errors

---

## Dependencies

### Firmware
- Arduino framework (via PlatformIO)
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson) v7+

### AI Module
- Python 3.8+
- pyserial ≥ 3.5
- opencv-python ≥ 4.8.0
- Flask ≥ 3.0.0
- flask-cors ≥ 4.0.0

### Web Dashboard
- Chart.js (loaded via CDN)
- Chromium-based browser with Web Serial API support

---

## License

MIT
