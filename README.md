<div align="center">

# 🌊 AquaPuer — Smart Water Station

**AI-Powered Water Quality Monitoring & Management System**

![ESP32-S3](https://img.shields.io/badge/ESP32--S3-N16R8-blue?style=flat-square&logo=espressif)
![STM32](https://img.shields.io/badge/STM32-Blue%20Pill-03234B?style=flat-square&logo=stmicroelectronics)
![React](https://img.shields.io/badge/React-19-61DAFB?style=flat-square&logo=react)
![Python](https://img.shields.io/badge/Python-3.10+-3776AB?style=flat-square&logo=python)
![YOLO](https://img.shields.io/badge/YOLOv8-Detection-00FFFF?style=flat-square)
![License](https://img.shields.io/badge/License-Academic-green?style=flat-square)

</div>

---

## Overview

AquaPuer is a full-stack IoT water treatment monitoring system built as a graduation project. It combines **embedded firmware**, a **real-time web dashboard**, and an **AI security module** to monitor water quality, control actuators, and detect unauthorized physical access — all in real time.

```
┌──────────────────┐       UART (JSON + CRC8)       ┌──────────────────────┐
│   STM32 RTU      │ ──────────────────────────────► │   ESP32-S3 MTU+AI.ML│
│   (Field Sensors) │                                │   (Edge Controller)  │
└──────────────────┘                                 └──────────┬───────────┘
                                                                │
                                                   ┌────────────┼────────────┐
                                                   │            │            │
                                              WiFi AP      USB Serial   LittleFS
                                              ws://81      115200 baud  Dashboard
                                                   │            │            │
                                                   ▼            ▼            ▼
                                            ┌───────────┐ ┌──────────┐ ┌──────────┐
                                            │   Web     │ │    AI    │ │ On-board │
                                            │ Dashboard │ │  Module  │ │ Dashboard│
                                            │ (Browser) │ │ (Python) │ │ (ESP32)  │
                                            └───────────┘ └──────────┘ └──────────┘
```

---

## Project Structure

```
Graduation Project/
├── firmware/                   # Embedded firmware
│   ├── MTU/                    # ESP32-S3 — Master Terminal Unit
│   │   ├── src/                # FreeRTOS tasks, sensors, web server, AI
│   │   ├── data/               # Built dashboard (LittleFS)
│   │   ├── lib/edge_impulse/   # Edge Impulse model (after training)
│   │   └── ARCHITECTURE.md
│   └── RTU/                    # STM32 Blue Pill — Remote Terminal Unit
│       └── src/                # Sensor acquisition, CRC8, IWDG
├── web/                        # Web dashboard source
│   └── Smart-Water-Station-main/
│       ├── app/                # React pages (10 routes)
│       ├── _components/        # Reusable UI components
│       ├── lib/                # WebSocket hook
│       └── scripts/            # Build → ESP32 deploy script
├── ai/                         # AI security module
│   ├── main.py                 # FastAPI server + AIController
│   ├── security.py             # YOLO/OpenCV detection engine
│   ├── config.json             # Runtime configuration
│   └── requirements.txt
└── README.md                   # ← You are here
```

---

## Components

### 🔧 Firmware — `firmware/`

Two PlatformIO projects communicating over UART:

| Unit | MCU | Role | Key Features |
|------|-----|------|-------------|
| **MTU** | ESP32-S3 N16R8 | Edge controller + WiFi AP | FreeRTOS, WebSocket, REST API, Watchdog, Edge Impulse, NVS |
| **RTU** | STM32F103C8 | Field sensor acquisition | 4× ADC, CRC8 integrity, IWDG watchdog, fault detection |

**Sensors monitored:**
- 🧪 **TDS** — Total Dissolved Solids (water purity)
- ⚡ **Pressure** — Pipeline pressure
- 💧 **Flow Rate** — Water throughput
- 📏 **Water Level** — Tank level percentage

See [`firmware/MTU/ARCHITECTURE.md`](firmware/MTU/ARCHITECTURE.md) and [`firmware/RTU/README.md`](firmware/RTU/README.md) for details.

---

### 🖥️ Web Dashboard — `web/`

A modern single-page application built with **React 19**, **Vite 7**, **TypeScript**, and **Tailwind CSS 4**.

| Feature | Description |
|---------|-------------|
| 📊 Live Sensors | Real-time charts for TDS, pressure, flow, and level |
| 🎮 Control Panel | Pump toggle, Manual/AI mode switch |
| 🧠 AI Insights | Performance forecast, efficiency donut, alert cards |
| 🔌 Prototype Demo | Direct ESP32 AP connection with live sensor readout |
| 🔐 Login | Authentication page |
| 📋 Reports & Alerts | System reports and alert history |
| 📱 Responsive | Mobile-friendly layout with sidebar navigation |

**Live connection:**  The `useWaterStation()` hook connects via WebSocket to the ESP32 AP. When no device is connected, it falls back to realistic simulation mode for development.

**Deploy to ESP32:**
```bash
cd web/Smart-Water-Station-main
npm run build:esp32     # Build + copy + gzip → firmware/MTU/data/
cd ../../firmware/MTU
pio run -t uploadfs     # Flash LittleFS to ESP32
```

See [`web/Smart-Water-Station-main/README.md`](web/Smart-Water-Station-main/README.md) for setup.

---

### 🤖 AI Security Module — `ai/`

A Python application that monitors a camera feed for unauthorized access using **YOLOv8 object detection**. When a person is detected near the water station, it sends an emergency stop command to the ESP32.

| Feature | Description |
|---------|-------------|
| 🎯 Object Detection | YOLOv8 person detection (or Haar Cascade fallback) |
| 🔌 Serial Link | Reads ESP32 telemetry, sends commands |
| 🌐 FastAPI Server | REST API + WebSocket for remote monitoring |
| 📝 Rotating Logs | Configurable log levels and file rotation |
| ⚡ Anti-Flooding | Cooldown timer prevents command spam |

```bash
cd ai
pip install -r requirements.txt
python main.py
```

See [`ai/README.md`](ai/README.md) for configuration details.

---

## Communication Protocol

All communication uses **JSON over Serial** at 115200 baud.

### RTU → MTU (UART with CRC8)
```json
{"type":"rtu_frame","seq":42,"tds":250.00,"pressure":45.00,"flow":0.00,"level":75.00,"err":0,"ts":8400,"crc":123}
```

### MTU → Web Dashboard (WebSocket port 81)
```json
{"device":"Smart Water Station","firmware":"3.1.0","state":"IDLE","sensors":{"tds":{"value":250,"valid":true},"pressure":{"value":45,"valid":true}}}
```

### Web/AI → MTU (Commands)
```json
{"cmd":"SET_PUMP","state":"ON"}
{"cmd":"EMERGENCY_STOP","state":"ON"}
{"cmd":"RESET"}
{"cmd":"CALIBRATE_TDS","value":500}
```

---

## Safety Mechanisms

1. **Edge Safety** — ESP32 checks sensor thresholds locally every 200ms, independent of any client
2. **CRC8 Integrity** — Every RTU frame is CRC-verified; corrupted data is silently dropped
3. **Watchdog Timers** — Both ESP32 (10s) and STM32 (4s) auto-reset if any task hangs
4. **Sensor Fault Detection** — RTU detects disconnected/shorted sensors (ADC near 0 or 4095)
5. **State Machine Locking** — Emergency stop requires explicit reset command
6. **Anti-Flooding** — AI module tracks state changes with cooldown to prevent serial spam
7. **NVS Persistence** — Calibration data survives power cycles

---

## Quick Start

### 1. Flash Firmware

```bash
# MTU (ESP32-S3)
cd firmware/MTU
pio run -e esp32s3_n16r8 -t upload

# RTU (STM32)
cd firmware/RTU
pio run -e bluepill_f103c8 -t upload
```

### 2. Deploy Dashboard

```bash
cd web/Smart-Water-Station-main
npm install
npm run build:esp32

cd ../../firmware/MTU
pio run -e esp32s3_n16r8 -t uploadfs
```

### 3. Connect

1. Connect to WiFi: **AquaPuer-MTU** (password: `12345678`)
2. Open: **http://192.168.4.1**

### 4. Start AI Module (Optional)

```bash
cd ai
pip install -r requirements.txt
python main.py
```

---

## Tech Stack

| Layer | Technology |
|-------|-----------|
| MCU (Master) | ESP32-S3 N16R8, Arduino + FreeRTOS, PlatformIO |
| MCU (Remote) | STM32F103C8 (Blue Pill), Arduino, PlatformIO |
| Communication | JSON over UART (CRC8), WebSocket, REST API |
| Frontend | React 19, Vite 7, TypeScript, Tailwind CSS 4, Radix UI, shadcn/ui |
| AI/Security | Python 3.10+, YOLOv8 (Ultralytics), FastAPI, OpenCV |
| Edge AI | Edge Impulse (data collection + inference) |
| Storage | LittleFS (dashboard), NVS (calibration), Rotating logs |

---

## Team

AquaPuer — Graduation Project 2026

---
