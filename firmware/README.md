# 🔧 AquaPuer Firmware

Embedded firmware for the Smart Water Station. Two PlatformIO projects communicating over UART:

| Unit | Folder | MCU | Role |
|------|--------|-----|------|
| **MTU** | `MTU/` | ESP32-S3 N16R8 | Edge controller, WiFi AP, dashboard, AI inference |
| **RTU** | `RTU/` | STM32F103C8 | Field sensor acquisition, CRC8, fault detection |

## Quick Start

```bash
# Build both
cd MTU && pio run -e esp32s3_n16r8
cd ../RTU && pio run -e bluepill_f103c8

# Upload both
cd ../MTU && pio run -e esp32s3_n16r8 -t upload
cd ../RTU && pio run -e bluepill_f103c8 -t upload

# Upload dashboard to ESP32 LittleFS
cd ../MTU && pio run -e esp32s3_n16r8 -t uploadfs
```

## UART Wiring

```
STM32 PA9  (TX) ────► ESP32 GPIO17 (RX)
STM32 PA10 (RX) ◄──── ESP32 GPIO18 (TX)
         GND ────────── GND
```

Both 3.3V logic — no level shifter needed.

## Architecture

See [`MTU/ARCHITECTURE.md`](MTU/ARCHITECTURE.md) for the full system design.
