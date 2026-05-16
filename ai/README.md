# 🤖 AquaPuer AI Security Module

Python-based physical security system for the Smart Water Station. Monitors a camera feed using **YOLOv8 object detection** to detect unauthorized persons near the water station, and automatically triggers an emergency stop on the ESP32 controller.

---

## Features

| Feature | Description |
|---------|-------------|
| 🎯 Person Detection | YOLOv8n (nano) for real-time person detection |
| 📹 Live Preview | Camera window with bounding boxes and status overlay |
| 🔌 Serial Integration | Reads ESP32 telemetry, sends emergency stop commands |
| 🌐 FastAPI Server | REST API + WebSocket for remote status monitoring |
| 📝 Structured Logging | Rotating file logs with configurable levels |
| ⚡ Anti-Flooding | Cooldown timer prevents duplicate emergency commands |
| 🔄 Auto-Reconnect | Automatic serial reconnection on disconnect |

---

## Quick Start

```bash
# Install dependencies
pip install -r requirements.txt

# Run standalone (no API server)
python main.py

# Run with FastAPI server (set api.enabled=true in config.json)
python main.py
# → API docs: http://localhost:5000/docs
# → WebSocket: ws://localhost:5000/ws/live
```

---

## Configuration — `config.json`

```jsonc
{
    "serial": {
        "port": "COM3",          // ESP32 serial port
        "baud_rate": 115200,
        "timeout": 1.0,
        "reconnect_delay": 5     // Seconds between reconnect attempts
    },
    "security": {
        "detection_enabled": true,
        "detection_mode": "person",    // "person" (YOLO) or "face" (Haar Cascade)
        "confidence": 0.5,             // YOLO confidence threshold
        "camera_index": 0,             // 0 = default webcam
        "show_preview": true           // Show live camera window
    },
    "logging": {
        "enabled": true,
        "level": "INFO",               // DEBUG, INFO, WARNING, ERROR
        "file": "logs/water_station.log",
        "max_size_mb": 10,
        "backup_count": 3
    },
    "api": {
        "enabled": false,              // Set true to start FastAPI server
        "host": "0.0.0.0",
        "port": 5000
    },
    "alerts": {
        "emergency_stop_on_detection": true,
        "cooldown_seconds": 5          // Minimum seconds between commands
    }
}
```

---

## API Endpoints

Available when `api.enabled` is `true` in config.json.

| Method | Endpoint | Description |
|--------|----------|-------------|
| `GET` | `/api/status` | Full system status (serial, telemetry, security) |
| `POST` | `/api/command` | Send command to ESP32 (`{"cmd": "EMERGENCY_STOP", "state": "ON"}`) |
| `GET` | `/api/telemetry` | Latest sensor readings |
| `GET` | `/api/security` | Detection statistics |
| `GET` | `/api/health` | Health check |
| `WS` | `/ws/live` | Real-time telemetry + security alerts |

Interactive API documentation is available at `http://localhost:5000/docs` (Swagger UI).

---

## Detection Modes

### YOLOv8 (Recommended)
- Model: `yolov8n.pt` (nano — 6MB, fast on CPU)
- Detects: full persons at any angle
- Confidence threshold: configurable in `config.json`
- Install: `pip install ultralytics`

### Haar Cascade (Fallback)
- Built-in OpenCV classifier
- Detects: frontal faces only
- Less accurate, more false positives
- No additional download needed

---

## Dependencies

```
pyserial>=3.5
opencv-python>=4.8.0
ultralytics>=8.0.0
fastapi>=0.115.0
uvicorn[standard]>=0.30.0
pydantic>=2.0.0
```

---

## How It Works

```
Camera → YOLOv8 Detection → Person found?
                                 │
                         ┌───────┴────────┐
                         │ Yes            │ No
                         ▼                ▼
                  Cooldown check?    Continue monitoring
                         │
                    ┌────┴─────┐
                    │ Passed   │ In cooldown
                    ▼          ▼
              Send EMERGENCY   Skip
              STOP to ESP32    (already sent)
                    │
                    ▼
              Broadcast alert
              to WebSocket clients
```

---
