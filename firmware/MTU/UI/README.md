# MTU Web Folder

The ESP32 does not serve this folder directly.

The real dashboard source is:

```text
web/Smart-Water-Station-main/
```

The deployed dashboard files live in:

```text
firmware/MTU/data/
```

Recommended workflow:

```bash
cd web/Smart-Water-Station-main
npm run build:esp32

cd ../../firmware/MTU
pio run -e esp32s3_n16r8 -t uploadfs
```

`npm run build:esp32` runs the copy script:

```text
web/Smart-Water-Station-main/scripts/copy-dist-to-platformio.cjs
```

That script copies Vite `dist/` into `firmware/MTU/data` and creates gzip
assets for the ESP32 web server.

The firmware-side server is implemented in:

```text
firmware/MTU/src/WebDashboard.cpp
firmware/MTU/src/WebDashboard.h
```

Current served interfaces:

| Interface | Address |
|---|---|
| Dashboard | `http://192.168.4.1/` |
| REST API | `http://192.168.4.1/api/*` |
| WebSocket | `ws://192.168.4.1:81/` |
