# 🤖 AquaPuer AI Station Monitor v3.0

Comprehensive visual monitoring system for the Smart Water Station. Uses **YOLOv8** object detection combined with **computer vision analysis** to monitor every aspect of the station in real time.

---

## What It Monitors

| Detection | Method | Severity | Action |
|-----------|--------|----------|--------|
| 👤 **Person** | YOLOv8 person class | ⚠️ WARNING | Log + alert dashboard |
| ⚡ **Proximity Hazard** | Person bbox ∩ danger zone | 🔴 CRITICAL | **EMERGENCY STOP** |
| 🔥 **Fire** | HSV colour analysis (orange/red/yellow) | 🔴 CRITICAL | **EMERGENCY STOP** + fire alert |
| 💨 **Smoke** | Grey haze + low contrast detection | 🔴 CRITICAL | **EMERGENCY STOP** + ventilation alert |
| 🟢 **Algae** | Water ROI green tint (hue 35-85) | ⚠️ WARNING | Check chlorination |
| 🔴 **Contamination** | Water ROI red/brown (hue 0-15, 165+) | 🔴 CRITICAL | **STOP distribution** |
| 🟡 **Turbidity** | Water ROI yellow/murky (hue 15-35) | ⚠️ WARNING | Check filtration |
| ⚪ **Foam** | Water ROI bright + desaturated | ⚠️ WARNING | Check for chemical spill |
| ⚫ **Dark Water** | Water ROI very low brightness | ⚠️ WARNING | Check pipe blockage |
| 🐕 **Animals** | YOLOv8 (cat, dog) | ℹ️ INFO | Log for awareness |
| 🚗 **Vehicles** | YOLOv8 (car, truck, motorcycle) | ℹ️ INFO | Check if authorized |

---

## How It Works

```
Camera Frame (30 fps)
    │
    ├─► YOLOv8 Detection ─────► Person? ──► In danger zone? ──► EMERGENCY STOP
    │                           Animal?     (pump, chemicals)
    │                           Vehicle?
    │
    ├─► HSV Fire Analysis ────► Orange/red flame pixels > 0.3%? ──► CRITICAL
    │
    ├─► Smoke Analysis ───────► Grey haze > 2% + low contrast? ──► CRITICAL
    │
    └─► Water ROI Analysis ───► Colour anomaly? ──► Green (algae)
                                                    Red (contamination)
                                                    Yellow (turbidity)
                                                    White (foam)
                                                    Dark (blockage)
    │
    ▼
┌──────────────────────────────────────────────────┐
│  Alert Engine                                     │
│  • Severity classification (INFO/WARNING/CRITICAL)│
│  • Actionable recommendations                     │
│  • Cooldown timer (anti-spam)                     │
│  • WebSocket broadcast to dashboard               │
│  • Serial command to ESP32 on CRITICAL            │
└──────────────────────────────────────────────────┘
```

---

## Quick Start

```bash
pip install -r requirements.txt
python main.py
```

On first run, YOLOv8 automatically downloads `yolov8n.pt` (~6 MB).

**With API server:**  Set `"api": { "enabled": true }` in `config.json`:
- API docs: http://localhost:5000/docs
- WebSocket: ws://localhost:5000/ws/live

---

## Configuration — `config.json`

### Danger Zones

Define rectangular areas in the camera frame where person proximity triggers CRITICAL alerts:

```json
"danger_zones": [
    { "name": "Main Pump",        "x1": 100, "y1": 200, "x2": 300, "y2": 400 },
    { "name": "Chemical Storage",  "x1": 400, "y1": 150, "x2": 580, "y2": 350 }
]
```

> **Tip:** Run with `show_preview: true` to see the danger zones drawn on the camera feed. Adjust coordinates to match your actual pump/equipment positions.

### Water ROI

Define where water is visible in the camera for colour analysis:

```json
"water": {
    "enabled": true,
    "roi": { "x1": 0, "y1": 350, "x2": 640, "y2": 480 }
}
```

### Fire & Smoke Sensitivity

```json
"fire":  { "enabled": true, "min_area_pct": 0.3 },
"smoke": { "enabled": true, "min_area_pct": 2.0 }
```

- `min_area_pct` — Minimum percentage of the frame that must match fire/smoke colours to trigger an alert. Lower = more sensitive.

---

## API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| `GET` | `/api/status` | Full system status + active alerts + recommendations |
| `POST` | `/api/command` | Send command to ESP32 |
| `GET` | `/api/alerts` | Current alerts + recommendations + threat status |
| `GET` | `/api/telemetry` | Latest sensor readings from ESP32 |
| `GET` | `/api/security/stats` | Detection counters + camera/model status |
| `GET` | `/api/health` | Health check (serial, camera, model) |
| `WS` | `/ws/live` | Real-time stream: telemetry + security + alerts |

---

## Alert Format

Every alert includes:

```json
{
    "alert_type": "proximity",
    "severity": "CRITICAL",
    "message": "⚠️ Person in danger zone: Main Pump!",
    "confidence": 0.92,
    "bbox": [120, 210, 280, 390],
    "recommendation": "EMERGENCY: Person too close to Main Pump. Activate emergency stop immediately!",
    "timestamp": 1716000000.0
}
```

---

## Camera Preview

When `show_preview: true`, the monitor shows a live annotated view:

- **Red rectangles** — CRITICAL alerts (fire, proximity)
- **Orange rectangles** — WARNING alerts (person, water anomaly)
- **Cyan rectangles** — INFO alerts (animals, vehicles)
- **Red dashed zones** — Configured danger zones
- **Blue ROI** — Water analysis region
- **Status bar** — ALL CLEAR / WARNING / CRITICAL with alert count

Press **Q** to quit.

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
