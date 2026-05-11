# Smart Water Station MTU Architecture  v3.0.0

This project is kept intentionally practical for a graduation project:
professional structure, but no heavy dependencies until the hardware needs them.

## Target Hardware

- ESP32-S3 N16R8 style module.
- 16 MB flash.
- 8 MB PSRAM.
- PlatformIO environment: `esp32s3_n16r8`.
- PlatformIO board target: `esp32-s3-devkitc-1`.
- Filesystem: LittleFS.
- Custom partition table: `partitions_16mb_ota_littlefs.csv`.

The pin map in `src/Config.h` avoids GPIO26-GPIO37 because they are commonly
used internally by flash/PSRAM on ESP32-S3 N16R8 modules.

## Current MTU Responsibilities

- Runs on ESP32-S3 using Arduino + FreeRTOS.
- Hosts the dashboard from `data/` through LittleFS.
- Starts a WiFi access point:
  - SSID: `AquaPuer-MTU`
  - Password: `12345678`
  - Default IP: `192.168.4.1`
- Owns the control state machine for pump, valve, emergency stop, reset, and calibration.
- Publishes telemetry through Serial JSON.
- Provides REST APIs for commands/config.
- Broadcasts live dashboard telemetry through WebSocket port `81`.
- Receives commands via WebSocket (bidirectional).
- Receives RTU sensor frames via Serial1 UART link.
- Runs Edge Impulse inference (or rule-based placeholder when model is not loaded).
- Supports Edge Impulse data forwarder mode for training data collection.
- Task Watchdog Timer protects all tasks from getting stuck.

## Feature Flags

All toggleable features are controlled by `#define` flags in `Config.h`:

| Flag | Default | Purpose |
| --- | --- | --- |
| `ENABLE_WEB_DASHBOARD` | 1 | HTTP server, REST API, WebSocket |
| `ENABLE_RTU_LINK` | 1 | UART link to STM32 RTU |
| `ENABLE_EDGE_IMPULSE` | 0 | Edge Impulse model inference |
| `ENABLE_DATA_FORWARDER` | 0 | CSV output for `edge-impulse-data-forwarder` CLI |
| `ENABLE_HMI_DISPLAY` | 0 | Reserved for small display |
| `SIMULATION_MODE` | true | Simulated sensor values for testing without hardware |

## FreeRTOS Tasks

| Task | Core | Priority | Stack | Purpose |
| --- | --- | --- | --- | --- |
| `ControlTask` | 1 | 3 | 4KB | State machine, safety, pump/valve outputs, sensor snapshot |
| `SerialTask` | 0 | 2 | 4KB | Serial JSON command input |
| `RtuTask` | 0 | 2 | 4KB | RTU UART link (when `ENABLE_RTU_LINK=1`) |
| `TelemetryTask` | 0 | 1 | 4KB | Serial telemetry and state heartbeat |
| `WebTask` | 0 | 1 | 6KB | Dashboard HTTP/WebSocket server |
| `AiTask` | 1 | 1 | 4KB | Edge Impulse / rule-based analysis |
| `DataFwdTask` | 0 | 1 | 4KB | Edge Impulse data forwarder (when `ENABLE_DATA_FORWARDER=1`) |
| `HmiTask` | 1 | 1 | 4KB | Reserved for the small display |

### Synchronization

- **`portMUX_TYPE snapshotMux`**: Critical section protecting `TelemetrySnapshot`,
  `currentState`, `pumpRunning`, `valveOpen`, and all shared global state.
- **`SemaphoreHandle_t serialMutex`**: Mutex protecting Serial port from concurrent access.
- **`QueueHandle_t controlQueue`**: FreeRTOS Queue (capacity 10) for control commands.

### Watchdog Timer

All tasks register with the ESP32 Task Watchdog Timer (timeout: 10 seconds).
If any task hangs without calling `esp_task_wdt_reset()`, the system reboots automatically.

## Sensor Architecture

Sensors are read **once per control cycle** via `SensorManager::readAll()`.
The resulting `SensorSnapshot` is passed by reference to both `performSafetyCheck()`
and `updateTelemetrySnapshot()` — no double reads.

## RTU Link

When `ENABLE_RTU_LINK=1`:
- `RtuTask` reads JSON frames from `Serial1` (GPIO17 RX, GPIO18 TX).
- Parses `rtu_frame` JSON messages from the STM32 Blue Pill.
- Updates the shared `TelemetrySnapshot` with RTU sensor values.
- Detects RTU offline status if no frame is received within `RTU_TIMEOUT_MS`.

## Edge Impulse Integration

### Data Collection Mode (`ENABLE_DATA_FORWARDER=1`)

- `DataFwdTask` outputs sensor readings as CSV: `tds,pressure,flow,level\n`
- Normal telemetry is suppressed to keep Serial clean.
- Use `edge-impulse-data-forwarder --frequency 20` on the PC to capture data.

### Inference Mode (`ENABLE_EDGE_IMPULSE=1`)

- Place the exported Arduino library in `lib/edge_impulse/`.
- `AiService::run()` calls `run_classifier()` with a 4-feature buffer.
- Results are broadcast via Serial, REST API, and WebSocket.

See `lib/edge_impulse/README.md` for the complete step-by-step workflow.

## Dashboard API

The dashboard is split into two parts:

- `data/`: the built frontend files that are uploaded to LittleFS.
- `src/WebDashboard.*`: the firmware-side HTTP/API/WebSocket server that serves
  the files and connects the UI to the control system.

`GET /api/status`

Returns firmware, state, sensor readings, actuator status, RTU status, and AI status.

`POST /api/control`

Example:

```json
{"cmd":"SET_PUMP","state":"ON"}
```

Supported commands:

- `SET_PUMP`
- `SET_VALVE`
- `EMERGENCY_STOP`
- `RESET`
- `SET_MODE`
- `CALIBRATE_TDS`
- `CALIBRATE_PRESSURE`

`GET /api/config`

Returns compile-time feature flags and thresholds.

`GET /api/ai`

Returns the latest AI/model result.

`GET /health`

Simple health check.

## Dashboard WebSocket

Live telemetry is broadcast on:

```text
ws://192.168.4.1:81
```

The payload is the same JSON shape returned by `GET /api/status`.

The WebSocket is **bidirectional**: clients can send control commands in the same
JSON format as `POST /api/control` and receive acknowledgment responses.

Use REST for one-off actions and WebSocket for live sensor/state updates + control.

## Build and Upload

1. Build and upload filesystem:

```bash
pio run -e esp32s3_n16r8 -t uploadfs
```

2. Build and upload firmware:

```bash
pio run -e esp32s3_n16r8 -t upload
```

3. Connect to `AquaPuer-MTU` and open:

```text
http://192.168.4.1
```

4. The RTU folder contains the STM32 starter that streams JSON frames via USART1.

5. For Edge Impulse, see `lib/edge_impulse/README.md`.
