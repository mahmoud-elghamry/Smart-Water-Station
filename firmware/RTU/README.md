# STM32 RTU Starter

This folder now contains a simple STM32 RTU starter firmware.

The default PlatformIO target is `bluepill_f103c8`, which matches the common
cheap STM32 Blue Pill board used in many graduation projects.

## Current Behavior

- Reads four analog inputs.
- Applies a small smoothing average.
- Sends one JSON line every 200 ms on USART1 at 115200 baud.
- Blinks `PC13` on every frame.

Example frame:

```json
{"type":"rtu_frame","seq":1,"tds":250.00,"pressure":45.00,"flow":0.00,"level":75.00,"ts":1000}
```

## Starter Pins

| Signal | Pin |
| --- | --- |
| TDS | PA0 |
| Pressure | PA1 |
| Flow | PA2 |
| Water level | PA3 |
| Status LED | PC13 |

## MTU Link Pins

Use USART1 between Blue Pill and ESP32-S3 MTU:

| Blue Pill | Direction | ESP32-S3 MTU |
| --- | --- | --- |
| PA9 TX | RTU -> MTU | GPIO17 `PIN_RTU_RX` |
| PA10 RX | MTU -> RTU | GPIO18 `PIN_RTU_TX` |
| GND | common ground | GND |

Both boards are 3.3V logic, so do not use 5V UART levels.

## Upload

The starter config assumes ST-Link:

```bash
pio run -e bluepill_f103c8 -t upload
```

## Why JSON First?

JSON is slower than a binary protocol, but it is much easier for early testing,
serial logging, Edge Impulse data collection, and debugging. Once the hardware is
stable, this can be replaced with a binary and compact CRC-based frame.
