# Dashboard Frontend Source

Use this folder for the real dashboard source code, for example React, Vite, or
plain HTML/CSS/JS.

The ESP32 does not serve this folder directly.

Professional workflow:

1. Edit dashboard source here.
2. Build the frontend on the PC.
3. Copy the build output into `../data/`.
4. Upload LittleFS:

```bash
pio run -e esp32s3_n16r8 -t uploadfs
```

5. Upload firmware:

```bash
pio run -e esp32s3_n16r8 -t upload
```

The firmware-side server is implemented in `src/WebDashboard.*`. It is required
because the ESP32 needs code to serve `data/`, receive control commands, and
broadcast live telemetry through WebSocket.
