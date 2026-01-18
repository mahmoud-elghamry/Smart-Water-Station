# SmarterWater - Intelligent Water Treatment System

**SmarterWater** is a smart water treatment unit prototype that combines embedded systems, AI-powered safety monitoring, and a web-based control interface. The system automates ultrafiltration water treatment while prioritizing safety through real-time human detection and pressure monitoring.

## 🌟 Key Features

- **AI-Powered Safety**: Real-time human detection using face recognition prevents pump operation when personnel are detected
- **Pressure Safety Cutoff**: Automatic pump shutdown if system pressure exceeds 2.8 bar
- **Real-Time Monitoring**: Live sensor data visualization (TDS, pressure, flow rate)
- **Dual Control Modes**: Both AI-assisted safety mode and manual override capability
- **Web Dashboard**: Responsive Bootstrap interface for remote monitoring and control
- **Edge Computing**: Intelligent processing on ESP32 microcontroller
- **Serial Communication**: Secure JSON-based command protocol between components

## 📋 Project Overview

SmarterWater is a three-tier system architecture:

```
┌─────────────────┐
│  Web Interface  │ (HTML5/CSS3/JavaScript + Bootstrap)
│   Dashboard     │
└────────┬────────┘
         │ Serial Communication (115200 baud)
         │
┌────────▼────────┐          ┌─────────────────┐
│ ESP32 Firmware  │◄─────────│ AI Safety System│
│  Pump Control   │          │  (Python + OpenCV)
│ Sensor Data     │          │  Face Detection │
└─────────────────┘          └─────────────────┘
         │
         ▼
   ┌──────────┐
   │   Pump   │
   │ Sensors  │
   └──────────┘
```

## 📁 Project Structure

```
Graduation Project/
├── platformio.ini          # PlatformIO project configuration
├── README.md              # This file
├── src/
│   └── main.cpp          # ESP32 firmware (Arduino-based)
├── ai/
│   ├── AI.py             # Python AI safety monitoring system
│   └── haarcascade_frontalface_default.xml  # Face detection model
├── web/
│   └── index.html        # Web dashboard interface
├── include/              # Header files (if needed)
├── lib/                  # Custom libraries
└── test/                 # Test files
```

## 🔧 Component Details

### 1. ESP32 Firmware (`src/main.cpp`)

**Purpose**: Microcontroller firmware for pump control and sensor data management

**Key Features**:

- Pump control via GPIO Pin 2 (HIGH/LOW states)
- Real-time sensor data simulation:
  - **TDS (Total Dissolved Solids)**: 300-600 ppm
  - **Pressure Sensor**: 0.5-3.0 bar
  - **Flow Rate**: 0-10 units
  - **Flow Progress**: 0-100%
- Automatic pressure cutoff at 2.8 bar
- JSON-formatted serial output (1-second intervals)
- Command-based pump control (PUMP:ON/PUMP:OFF)

**Technologies**: Arduino Framework, C++, ESP32 SDK

**Serial Protocol**:

- Baud Rate: 115200
- Data Format: JSON (sensor data)
- Commands: Text-based (PUMP:ON, PUMP:OFF)

### 2. AI Safety System (`ai/AI.py`)

**Purpose**: Real-time computer vision-based human detection for safety

**Key Features**:

- Haar Cascade face detection (haarcascade_frontalface_default.xml)
- Automatic ESP32 port detection (supports CP210x, CH340, CH9102 drivers)
- Safety logic:
  - Human detected → PUMP:OFF (immediate safety response)
  - Area clear → PUMP:ON (resume operation)
- Dual operation modes:
  - **Connected Mode**: Direct communication with ESP32
  - **Test Mode**: Simulated operation for development
- Live video feed with visual feedback:
  - Green "RUNNING..." text when area is clear
  - Red "SAFETY STOP" warning when human detected
- Graceful shutdown (ensures pump OFF on exit)

**Technologies**: Python 3.6+, OpenCV (cv2), PySerial

**Dependencies**:

```
opencv-python
pyserial
```

### 3. Web Interface (`web/index.html`)

**Purpose**: User dashboard for system monitoring and control

**Key Features**:

- Real-time connection status indicator
- Sensor readings display (TDS, Pressure, Flow Rate)
- Pump ON/OFF toggle control
- AI Safety Mode vs. Manual Mode selection
- Flow progress visualization
- Responsive design (mobile, tablet, desktop)
- Modern gradient UI with cyan-blue theme
- Professional navigation and footer

**Technologies**: HTML5, CSS3, JavaScript, Bootstrap 5.3, Font Awesome 6.4

**Styling**:

- Responsive layout with mobile-first design
- Color scheme: Cyan (#06b6d4) to Blue (#2563eb)
- Font: Segoe UI / system fonts

## 📦 Prerequisites

### Hardware Requirements

- **Microcontroller**: ESP32 development board
- **Communication**: USB cable (USB-to-Serial for ESP32)
- **Camera**: Webcam or USB camera (for AI face detection)
- **Actuator**: Pump with relay control capability
- **Sensors**: Pressure and flow rate sensors (or simulated in firmware)

### Software Requirements

#### For ESP32 Firmware:

- **PlatformIO IDE** (recommended) or Arduino IDE
- **ESP32 Board Support Package**
- **Python 3.6+** (for PlatformIO CLI)

#### For AI Safety System:

- **Python 3.6 or higher**
- **pip** (Python package manager)
- **opencv-python** (computer vision library)
- **pyserial** (serial communication library)

#### For Web Interface:

- Modern web browser (Chrome, Firefox, Edge, Safari)
- Optional: Local web server (Python `http.server` or Node.js `http-server`)

#### System-Level:

- Windows, Linux, or macOS
- USB drivers for ESP32 (CP210x or CH340 - usually auto-installed)

## 🚀 Installation & Setup

### Step 1: ESP32 Firmware Upload

1. **Install PlatformIO**:

   ```bash
   pip install platformio
   ```

2. **Navigate to project directory**:

   ```bash
   cd "c:\work\Graduation Project"
   ```

3. **Upload firmware to ESP32**:

   ```bash
   pio run -t upload -e esp32dev
   ```

   Or using PlatformIO IDE:
   - Open the project folder in VS Code with PlatformIO extension
   - Click "Upload" in the PlatformIO toolbar

4. **Verify serial output**:
   ```bash
   pio device monitor
   ```
   You should see JSON sensor data appearing every second.

### Step 2: AI Safety System Setup

1. **Install Python dependencies**:

   ```bash
   cd "c:\work\Graduation Project\ai"
   pip install opencv-python pyserial
   ```

2. **Connect ESP32 via USB**

3. **Run the AI system**:

   ```bash
   python AI.py
   ```

   The system will:
   - Auto-detect ESP32 port
   - Open camera feed
   - Start face detection monitoring
   - Send PUMP:ON/OFF commands based on detection

4. **Test mode** (without ESP32):
   - Modify AI.py to enable test mode
   - Useful for testing camera and face detection

### Step 3: Web Interface Deployment

1. **Local File Access** (Simplest):
   - Open `web/index.html` directly in your browser
   - Note: Serial communication may require a local server

2. **Local Web Server** (Recommended):

   ```bash
   cd "c:\work\Graduation Project\web"
   python -m http.server 8000
   ```

   Then visit: `http://localhost:8000`

3. **Web Interface Features**:
   - Connection Status: Shows ESP32 connection state
   - Sensor Display: Real-time TDS, pressure, flow data
   - Pump Control: Manual toggle switch
   - Mode Selection: Switch between AI Safety and Manual modes
   - Flow Progress: Visual progress bar (0-100%)

## 🔄 System Communication Flow

```
1. USER OPENS WEB INTERFACE
   ├─ Connects to localhost
   └─ Establishes monitoring dashboard

2. AI SAFETY SYSTEM RUNS
   ├─ Detects ESP32 via USB
   ├─ Opens webcam feed
   ├─ Analyzes for human presence
   └─ Sends commands based on detection

3. ESP32 RECEIVES COMMANDS
   ├─ PUMP:ON  → GPIO 2 HIGH (Pump starts)
   └─ PUMP:OFF → GPIO 2 LOW  (Pump stops)

4. SENSORS GENERATE DATA
   ├─ TDS Level
   ├─ System Pressure
   ├─ Flow Rate
   └─ Flow Progress

5. ESP32 SENDS DATA
   ├─ Format: JSON via Serial
   ├─ Baud Rate: 115200
   └─ Interval: 1 second

6. WEB INTERFACE DISPLAYS DATA
   ├─ Updates sensor readings
   ├─ Shows connection status
   ├─ Allows manual pump control
   └─ Visualizes system state
```

## 🛡️ Safety Features

| Feature               | Description                      | Trigger                      |
| --------------------- | -------------------------------- | ---------------------------- |
| **Human Detection**   | AI-powered face detection        | When face detected in camera |
| **Pressure Cutoff**   | Automatic pump shutdown          | Pressure > 2.8 bar           |
| **Graceful Shutdown** | Ensures pump OFF on exit         | AI system termination        |
| **Serial Validation** | Command verification             | Before pump control          |
| **Status Monitoring** | Continuous system state tracking | Real-time sensor feedback    |
| **Manual Override**   | Emergency pump control           | Via web dashboard            |

## 📖 Usage Instructions

### Normal Operation

1. **Start ESP32**: Ensure USB connection to power and serial
2. **Run AI System**: Execute `python AI.py` in `ai/` folder
3. **Open Dashboard**: Navigate to web interface (local file or server)
4. **Select Mode**:
   - **AI Safety Mode**: Automatic pump control based on face detection
   - **Manual Mode**: Direct pump control via web interface toggle
5. **Monitor**: Watch sensor readings and connection status in real-time

### Mode Switching

- **To AI Safety Mode**: Click "AI Safety Mode" button → System uses face detection
- **To Manual Mode**: Click "Manual Mode" button → Direct web dashboard control

### Emergency Stop

- Press `Ctrl+C` in AI system terminal (ensures pump OFF)
- Or toggle pump OFF in web dashboard (Manual Mode)
- Or system auto-stops if pressure exceeds 2.8 bar

## 🔍 Troubleshooting

### ESP32 Not Detected

- **Issue**: Serial port not found
- **Solution**:
  - Check USB cable connection
  - Install USB drivers (CP210x for ESP32-WROOM)
  - List available ports: `pio device list`

### AI System Can't Connect to ESP32

- **Issue**: "Port not found" error
- **Solution**:
  - Verify ESP32 is running and connected
  - Enable test mode in AI.py to verify camera works
  - Manually specify port in AI.py (change `auto_detect_port()` logic)

### Webcam Not Working

- **Issue**: Camera feed shows black or no camera detected
- **Solution**:
  - Check camera permissions (Windows/Linux/Mac)
  - Verify no other application is using the camera
  - Test camera with: `python -c "import cv2; cap = cv2.VideoCapture(0)"`

### Web Interface Not Updating

- **Issue**: Sensor data static or connection failed
- **Solution**:
  - Ensure AI system is running (sends data via serial)
  - Check browser console for JavaScript errors (F12)
  - Verify ESP32 is outputting data (check serial monitor)
  - Restart AI system: `Ctrl+C` then `python AI.py`

### Pressure Cutoff Too Sensitive

- **Issue**: Pump shutting down unexpectedly
- **Solution**:
  - Modify cutoff threshold in `src/main.cpp` (currently 2.8 bar)
  - Check if pressure sensor is faulty
  - Recompile and upload firmware

## 📝 Configuration

### ESP32 Configuration (`platformio.ini`)

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
monitor_rts = 0
monitor_dtr = 0
```

### AI System Configuration (`ai/AI.py`)

Key constants to modify:

- `BAUD_RATE`: Serial communication speed (default: 115200)
- `TIMEOUT`: Serial read timeout (default: 1 second)
- Face detection cascade path: `haarcascade_frontalface_default.xml`

### Web Interface Configuration (`web/index.html`)

Customize:

- Color scheme (CSS variables in `<style>`)
- Sensor display thresholds
- UI text and labels
- Bootstrap breakpoints for responsiveness

## 🎓 Project Context

This project was developed as part of a graduation thesis demonstrating:

- **IoT Systems Integration**: Multiple platforms communicating over Serial
- **Edge Computing**: Safety decisions made on microcontroller level
- **Computer Vision**: Real-time human detection using AI
- **Embedded Systems**: Firmware development for ESP32
- **Full-Stack Development**: Frontend (Web) + Backend (Python) + Firmware (C++)
- **Safety-Critical Systems**: Multi-layer safety mechanisms
