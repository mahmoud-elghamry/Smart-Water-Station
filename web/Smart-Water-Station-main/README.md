# 🖥️ AquaPuer Web Dashboard

Real-time water station monitoring dashboard built with **React 19**, **Vite 7**, **TypeScript**, and **Tailwind CSS 4**. Runs standalone for development and deploys to the ESP32-S3 as a LittleFS filesystem.

---

## Pages

| Route | Page | Description |
|-------|------|-------------|
| `/` | Landing | Hero section with project photos and feature showcase |
| `/login` | Login | Authentication page |
| `/home` | Dashboard | Main monitoring dashboard |
| `/sensors` | Sensors | Live sensor cards with area charts (TDS, Pressure, Flow, pH) |
| `/control` | Control | Pump toggle, Manual/AI mode, pressure readout |
| `/ai` | AI Insights | Performance forecast chart, efficiency donut, alert cards |
| `/prototype` | Prototype | Direct ESP32 demo with live sensor readout and flow bar |
| `/alerts` | Alerts | System alert history |
| `/reports` | Reports | Report generation |
| `/about` | About | Team information |
| `/contact` | Contact | Contact form |

---

## Tech Stack

| Package | Version | Purpose |
|---------|---------|---------|
| React | 19.2.3 | UI framework |
| Vite | 7.3.0 | Build tool + dev server |
| TypeScript | 5.x | Type safety |
| Tailwind CSS | 4.x | Utility-first styling |
| Radix UI | 1.4.3 | Accessible primitives (Switch, Toggle, etc.) |
| shadcn/ui | 3.8.4 | Pre-built component system |
| Lucide React | 0.564.0 | Icon library |
| vaul | 1.1.2 | Drawer component |

---

## ESP32 Connection

The `useWaterStation()` hook in `lib/use-water-station.ts` manages the WebSocket connection:

- **Connected**: `ws://{host}/ws` → receives live telemetry, sends pump/mode commands
- **Disconnected**: Falls back to **simulation mode** with realistic drifting values
- **Auto-reconnect**: Attempts reconnection every 2 seconds
- **Stale detection**: Closes socket if no message received in 3.5 seconds

### Data Flow

```
ESP32 AP (192.168.4.1)
    │
    ├── ws://192.168.4.1:81     → WebSocket telemetry + control
    ├── http://192.168.4.1/     → Dashboard (LittleFS)
    └── http://192.168.4.1/api/ → REST endpoints
         ├── GET  /api/status
         ├── POST /api/control
         ├── GET  /api/config
         └── GET  /api/ai
```

---

## Development

```bash
# Install dependencies
npm install

# Start dev server (accessible on local network)
npm run dev
# → http://localhost:5173

# Preview production build
npm run preview
```

The dev server runs in simulation mode automatically — no ESP32 needed for UI development.

---

## Build & Deploy to ESP32

```bash
# Build and copy to firmware data folder
npm run build:esp32
```

This runs:
1. `tsc -b` — TypeScript type check
2. `vite build` — Production bundle → `dist/`
3. `copy-dist-to-platformio.mjs` — Copies `dist/` → `firmware/MTU/data/`, gzips compressible files

Then flash to ESP32:
```bash
cd ../../firmware/MTU
pio run -e esp32s3_n16r8 -t uploadfs   # Upload LittleFS
pio run -e esp32s3_n16r8 -t upload     # Upload firmware
```

---

## Project Structure

```
Smart-Water-Station-main/
├── app/
│   ├── main.tsx                    # Vite entry point
│   ├── App.tsx                     # Client-side router
│   ├── page.tsx                    # Landing page (/)
│   ├── globals.css                 # Tailwind globals
│   ├── root-layout-client.tsx      # Layout with sidebar
│   └── (pages)/
│       ├── home/page.tsx           # Dashboard
│       ├── sensors/page.tsx        # Live sensor cards
│       ├── control/page.tsx        # Pump control
│       ├── ai/page.tsx             # AI insights
│       ├── prototype/page.tsx      # ESP32 demo
│       ├── alerts/page.tsx
│       ├── reports/page.tsx
│       ├── login/page.tsx
│       ├── about/page.tsx
│       └── contact/page.tsx
├── _components/
│   ├── fixed-components/           # NavBar, SideBar, Footer, PageHeader
│   ├── light-charts.tsx            # Lightweight chart components
│   └── ui/                         # shadcn/ui primitives
├── hooks/
│   └── use-mobile.ts               # Mobile breakpoint hook
├── lib/
│   ├── use-water-station.ts        # WebSocket + simulation hook
│   └── utils.ts                    # clsx/tailwind-merge utility
├── Interfaces/                     # TypeScript interfaces
├── Project photos/                 # Project images
├── shims/                          # Next.js → Vite compatibility shims
├── scripts/
│   └── copy-dist-to-platformio.mjs # Build deploy script
├── vite.config.ts
├── tsconfig.json
└── package.json
```

---
