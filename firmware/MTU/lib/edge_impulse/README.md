# Edge Impulse Model

Place the exported Arduino library contents here after training.

This folder is the integration point between Edge Impulse Studio and the
Smart Water Station firmware.

## Complete Workflow

### Step 1: Collect Training Data

1. Open `src/Config.h` and set:
   ```cpp
   #define ENABLE_DATA_FORWARDER 1
   #define ENABLE_EDGE_IMPULSE   0
   ```

2. Build and upload the firmware:
   ```bash
   pio run -e esp32s3_n16r8 -t upload
   ```

3. On your PC, install the Edge Impulse CLI (if not already):
   ```bash
   npm install -g edge-impulse-cli
   ```

4. Run the data forwarder:
   ```bash
   edge-impulse-data-forwarder --frequency 20
   ```
   - It will auto-detect the serial port and baud rate.
   - When prompted, name the axes: `tds, pressure, flow, level`
   - Name the device: `AquaPuer-MTU`

5. Collect samples for each class (e.g., `normal`, `pump_fault`, `high_tds`, etc.)
   in Edge Impulse Studio.

### Step 2: Train the Model

1. Go to [Edge Impulse Studio](https://studio.edgeimpulse.com)
2. Design your impulse (suggested):
   - **Input block**: Time series data (4 axes, window size ~2s at 20Hz = 40 samples)
   - **Processing block**: Flatten or Spectral Analysis
   - **Learning block**: Classification (Neural Network or K-NN)
3. Train and evaluate the model.

### Step 3: Export the Model

1. Go to **Deployment** tab in Edge Impulse Studio.
2. Select **Arduino library**.
3. Click **Build** → download the `.zip` file.
4. Extract the ZIP contents into **this folder** (`lib/edge_impulse/`).

The folder should look like:
```
lib/edge_impulse/
├── src/
│   ├── edge-impulse-sdk/
│   ├── model-parameters/
│   └── tflite-model/
├── library.properties
└── README.md (from Edge Impulse)
```

### Step 4: Enable Inference

1. Open `src/Config.h` and set:
   ```cpp
   #define ENABLE_DATA_FORWARDER 0
   #define ENABLE_EDGE_IMPULSE   1
   ```

2. Open `src/AiService.cpp` and:
   - Uncomment the `#include <your_project_inference.h>` line.
   - Change `your_project` to match the actual project name from Edge Impulse.
   - Uncomment the inference code block inside `AiService::run()`.

3. Build and upload:
   ```bash
   pio run -e esp32s3_n16r8 -t upload
   ```

4. The AI results will appear in:
   - Serial telemetry JSON
   - `GET /api/ai` REST endpoint
   - WebSocket broadcast on port 81
   - Dashboard UI

## Sensor Mapping

| Feature Index | Sensor | Unit | Range |
|---------------|--------|------|-------|
| 0 | TDS | PPM | 50–500 |
| 1 | Pressure | PSI | 5–100 |
| 2 | Flow Rate | L/min | 0–50 |
| 3 | Water Level | % | 10–100 |

## Notes

- Data forwarder mode and Edge Impulse inference are **mutually exclusive**.
  Do not enable both at the same time.
- The ESP32-S3 with 8MB PSRAM can handle most Edge Impulse models comfortably.
- If the model is too large, consider quantizing to INT8 in Edge Impulse Studio.
