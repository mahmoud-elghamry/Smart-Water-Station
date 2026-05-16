# 🔧 AquaPuer RTU — Remote Terminal Unit

STM32 Blue Pill firmware for field sensor acquisition. Reads 4 analog sensors, detects hardware faults, appends CRC8 integrity checks, and streams JSON frames to the MTU over UART.

---

## Features

| Feature | Description |
|---------|-------------|
| 📊 4× ADC Channels | TDS, Pressure, Flow, Water Level |
| 🔄 8-Sample Smoothing | Noise-reduced ADC readings |
| 🔌 Fault Detection | Detects disconnected/shorted sensors (ADC near 0 or 4095) |
| 🛡️ CRC8 Integrity | Every frame includes a CRC8 checksum |
| 🐕 IWDG Watchdog | Hardware watchdog — auto-resets if loop hangs (4s timeout) |
| 📦 Single-Buffer TX | One `snprintf()` + one `Serial.println()` per frame |
| 🔄 Bidirectional | Receives commands from MTU (e.g., change sample rate) |
| 💡 Error LED | Fast blink on sensor fault, normal toggle on healthy |

---

## Sensor Pins

| Signal | Pin | Range | Unit |
|--------|-----|-------|------|
| TDS | PA0 | 0–1000 | PPM |
| Pressure | PA1 | 0–150 | PSI |
| Flow | PA2 | 0–100 | L/min |
| Water Level | PA3 | 0–100 | % |
| Status LED | PC13 | — | — |

---

## MTU Link (UART)

| Blue Pill | Direction | ESP32-S3 MTU |
|-----------|-----------|-------------|
| PA9 (TX) | RTU → MTU | GPIO17 `PIN_RTU_RX` |
| PA10 (RX) | MTU → RTU | GPIO18 `PIN_RTU_TX` |
| GND | Common | GND |

Both boards are 3.3V logic. **Do not use 5V UART levels.**

---

## Frame Format

Every 200ms (configurable at runtime), the RTU sends:

```json
{"type":"rtu_frame","seq":42,"tds":250.00,"pressure":45.00,"flow":0.00,"level":75.00,"err":0,"ts":8400,"crc":123}
```

| Field | Type | Description |
|-------|------|-------------|
| `type` | string | Always `"rtu_frame"` |
| `seq` | uint32 | Frame sequence number (monotonically increasing) |
| `tds` | float | TDS in PPM (or `-1.0` if sensor fault) |
| `pressure` | float | Pressure in PSI (or `-1.0` if sensor fault) |
| `flow` | float | Flow rate in L/min (or `-1.0` if sensor fault) |
| `level` | float | Water level in % (or `-1.0` if sensor fault) |
| `err` | uint8 | Error flags bitmask (see below) |
| `ts` | uint32 | Timestamp in milliseconds since boot |
| `crc` | uint8 | CRC8 of everything before `,"crc":` |

### Error Flags Bitmask

| Bit | Value | Meaning |
|-----|-------|---------|
| 0 | `0x01` | TDS sensor fault |
| 1 | `0x02` | Pressure sensor fault |
| 2 | `0x04` | Flow sensor fault |
| 3 | `0x08` | Level sensor fault |

A sensor is flagged as **faulty** when its raw ADC reading is ≤ 10 (short circuit) or ≥ 4085 (open circuit / disconnected).

---

## Commands from MTU

The RTU listens for JSON commands on UART:

```json
{"cmd":"SET_INTERVAL","value":100}
```

| Command | Description |
|---------|-------------|
| `SET_INTERVAL` | Change frame interval in milliseconds (50–5000) |

---

## Build & Upload

Requires [PlatformIO](https://platformio.org/) and an ST-Link programmer.

```bash
pio run -e bluepill_f103c8            # Build
pio run -e bluepill_f103c8 -t upload  # Flash
```

### Build Stats

```
RAM:   6.3%  (1,292 / 20,480 bytes)
Flash: 33.2% (21,728 / 65,536 bytes)
```

---
