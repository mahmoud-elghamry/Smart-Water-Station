"""AIController — orchestrates Serial + MQTT + FastAPI/WS with DataBus sync."""

import time
import json
import csv
import os
import threading
import asyncio
import logging
from pathlib import Path
from typing import Optional, List

from fastapi import WebSocket

from monitor import StationMonitor, Severity
from serial_link import SerialLink
from mqtt_link import MQTTLink

logger = logging.getLogger("water_station.controller")

AI_DIR = Path(__file__).resolve().parent
DATASET_SENSORS = [
    "turb1",
    "turb2",
    "ph1",
    "ph2",
    "flow1",
    "flow2",
    "press1",
    "press2",
    "temp1",
    "temp2",
    "pump_current",
]
REQUIRED_DATASET_SENSORS = [
    "turb1",
    "turb2",
    "ph1",
    "ph2",
    "flow1",
    "flow2",
    "press1",
    "press2",
]
DATASET_COLUMNS = [
    "timestamp_ms",
    *DATASET_SENSORS,
    "pump_on",
    "state",
    "error",
    "label",
]


class AIController:

    def __init__(self, config: dict):
        self.config = config

        # Transports
        self.serial = SerialLink(config)
        self.mqtt = MQTTLink(config, on_command=self._mqtt_command)
        self.monitor = StationMonitor(config)

        # Alerts config
        ac = config.get("alerts", {})
        self.emergency_on_critical = ac.get("emergency_stop_on_critical", True)
        self.cooldown_seconds = ac.get("cooldown_seconds", 5)

        # State
        self.running = True
        self.last_threat_status = False
        self.last_command_time = 0.0
        self.latest_telemetry: dict = {}
        self.latest_state: dict = {"state": "UNKNOWN", "error": "NONE"}
        self._telemetry_lock = threading.Lock()

        # Edge Impulse dataset capture
        dc = config.get("dataset", {})
        self.dataset_enabled = dc.get("enabled", True)
        self.dataset_label = dc.get("label", "unlabeled")
        self.dataset_interval = float(dc.get("interval_seconds", 1.0))
        self.dataset_required_sensors = dc.get(
            "required_sensors", REQUIRED_DATASET_SENSORS
        )
        self.dataset_path = self._resolve_dataset_path(
            dc.get("file", "logs/edge_impulse_dataset.csv")
        )
        self.dataset_rows_written = self._count_dataset_rows()
        self._last_dataset_write = 0.0

        # WebSocket
        self.ws_clients: List[WebSocket] = []
        self._loop: Optional[asyncio.AbstractEventLoop] = None

    def _resolve_dataset_path(self, configured_path: str) -> Path:
        path = Path(configured_path)
        if not path.is_absolute():
            path = AI_DIR / path
        path.parent.mkdir(parents=True, exist_ok=True)
        return path

    def _count_dataset_rows(self) -> int:
        if not self.dataset_path.exists():
            return 0
        try:
            with self.dataset_path.open("r", newline="", encoding="utf-8") as f:
                return max(sum(1 for _ in f) - 1, 0)
        except OSError:
            return 0

    def set_dataset_label(self, label: str) -> str:
        cleaned = label.strip().replace(" ", "_")
        self.dataset_label = cleaned or "unlabeled"
        return self.dataset_label

    def get_dataset_status(self) -> dict:
        with self._telemetry_lock:
            present = [
                name
                for name in DATASET_SENSORS
                if isinstance(self.latest_telemetry.get(name, {}).get("value"), (int, float))
            ]
            missing_required = [
                name for name in self.dataset_required_sensors if name not in present
            ]

        return {
            "enabled": self.dataset_enabled,
            "file": str(self.dataset_path),
            "rows": self.dataset_rows_written,
            "label": self.dataset_label,
            "interval_seconds": self.dataset_interval,
            "sensors": DATASET_SENSORS,
            "required_sensors": self.dataset_required_sensors,
            "present_sensors": present,
            "missing_required_sensors": missing_required,
            "ready": len(missing_required) == 0,
        }

    # ── MQTT command callback (called from MQTT thread) ──────────────────────

    def _mqtt_command(self, payload: dict, source: str = "mqtt"):
        self.handle_command(payload, source=source)

    # ══════════════════════════════════════════════════════════════════════════
    #  DataBus — sync across all channels
    # ══════════════════════════════════════════════════════════════════════════

    def handle_command(self, payload: dict, source: str = "api"):
        """Command from any source → forward to all other channels."""
        self.serial.send(payload)
        if source != "mqtt":
            self.mqtt.publish("command/echo", {"source": source, **payload})
        self._ws_broadcast({"type": "command_sent", "source": source, "data": payload})

    def broadcast_telemetry(self, data: dict):
        self.mqtt.publish("telemetry", data)
        self._ws_broadcast({"type": "telemetry_update", "data": data})

    def broadcast_alert(self, alerts: list, recs: list, threat: bool):
        payload = {"type": "security_alert", "threat": threat,
                   "alerts": alerts, "recommendations": recs,
                   "timestamp": time.time()}
        self.mqtt.publish("alerts", payload)
        self._ws_broadcast(payload)

    def broadcast_status(self):
        self.mqtt.publish("status", {
            "serial": self.serial.connected,
            "mqtt": self.mqtt.connected,
            "threat": self.last_threat_status,
            "stats": self.monitor.get_stats(),
        })

    # ── WebSocket (thread-safe) ──────────────────────────────────────────────

    def _ws_broadcast(self, data: dict):
        if self._loop and self.ws_clients:
            asyncio.run_coroutine_threadsafe(self._async_broadcast(data), self._loop)

    async def _async_broadcast(self, data: dict):
        gone = []
        for ws in self.ws_clients:
            try:
                await ws.send_json(data)
            except Exception:
                gone.append(ws)
        for ws in gone:
            if ws in self.ws_clients:
                self.ws_clients.remove(ws)

    # ══════════════════════════════════════════════════════════════════════════
    #  Threads
    # ══════════════════════════════════════════════════════════════════════════

    def monitor_security(self):
        self.monitor.start_camera()
        while self.running:
            threat, alerts = self.monitor.detect()
            if alerts and alerts[0].alert_type == "quit":
                self.running = False
                break
            now = time.time()
            if threat and not self.last_threat_status:
                if now - self.last_command_time >= self.cooldown_seconds:
                    crit = [a for a in alerts if a.severity == Severity.CRITICAL]
                    logger.warning(f"CRITICAL: {'; '.join(a.message for a in crit[:3])}")
                    if self.emergency_on_critical:
                        self.handle_command({"cmd": "EMERGENCY_STOP", "state": "ON"}, "ai")
                        self.last_command_time = now
                    self.last_threat_status = True
                    self.broadcast_alert([a.to_dict() for a in crit],
                                         self.monitor.get_recommendations(), True)
            elif not threat and self.last_threat_status:
                logger.info("All clear")
                self.last_threat_status = False
                self.broadcast_alert([], [], False)
            time.sleep(0.02)

    def read_telemetry(self):
        while self.running:
            if self.serial.in_waiting:
                line = self.serial.readline()
                if line:
                    self._process(line)
            time.sleep(0.01)

    def _process(self, data: str):
        try:
            msg = json.loads(data)
            t = msg.get("type", "")
            if t == "telemetry":
                s = msg.get("sensor", "unknown")
                with self._telemetry_lock:
                    self.latest_telemetry[s] = msg
                self.broadcast_telemetry(msg)
                self._maybe_write_dataset_row()

            elif t == "state":
                with self._telemetry_lock:
                    self.latest_state = msg
                self.mqtt.publish("state", msg)
                self._ws_broadcast({"type": "state_update", "data": msg})
        except json.JSONDecodeError:
            pass

    def _dataset_value(self, sensor_name: str):
        telemetry = self.latest_telemetry.get(sensor_name, {})
        value = telemetry.get("value")
        if isinstance(value, (int, float)):
            return value
        return ""

    def _maybe_write_dataset_row(self):
        if not self.dataset_enabled:
            return

        now = time.time()
        if now - self._last_dataset_write < self.dataset_interval:
            return

        with self._telemetry_lock:
            missing = [
                name for name in self.dataset_required_sensors
                if not isinstance(self.latest_telemetry.get(name, {}).get("value"), (int, float))
            ]
            if missing:
                return

            row = {
                "timestamp_ms": int(now * 1000),
                "pump_on": self._dataset_value("pump_on"),
                "state": self.latest_state.get("state", "UNKNOWN"),
                "error": self.latest_state.get("error", "NONE"),
                "label": self.dataset_label,
            }
            for sensor_name in DATASET_SENSORS:
                row[sensor_name] = self._dataset_value(sensor_name)

        try:
            file_exists = self.dataset_path.exists()
            with self.dataset_path.open("a", newline="", encoding="utf-8") as f:
                writer = csv.DictWriter(f, fieldnames=DATASET_COLUMNS)
                if not file_exists or os.path.getsize(self.dataset_path) == 0:
                    writer.writeheader()
                writer.writerow(row)
            self.dataset_rows_written += 1
            self._last_dataset_write = now
        except OSError as e:
            logger.error(f"Error saving dataset row: {e}")

    def status_loop(self):
        while self.running:
            self.broadcast_status()
            time.sleep(10)

    # ── Status ───────────────────────────────────────────────────────────────

    def get_status(self) -> dict:
        return {
            "connected": self.serial.connected,
            "state": self.latest_state,
            "telemetry": self.latest_telemetry,
            "dataset": self.get_dataset_status(),
            "security": self.monitor.get_stats(),
            "active_alerts": self.monitor.get_active_alerts(),
            "recommendations": self.monitor.get_recommendations(),
            "threat_active": self.last_threat_status,
            "mqtt_connected": self.mqtt.connected,
        }

    # ── Lifecycle ────────────────────────────────────────────────────────────

    def run(self):
        logger.info("=" * 50)
        logger.info("  AquaPuer v3.1 — AI Controller")
        logger.info("  Serial + MQTT + FastAPI/WS")
        logger.info("=" * 50)

        self.serial.connect()
        self.mqtt.connect()

        for t in [
            threading.Thread(target=self.monitor_security, name="Monitor", daemon=True),
            threading.Thread(target=self.read_telemetry, name="Telemetry", daemon=True),
            threading.Thread(target=self.status_loop, name="Status", daemon=True),
        ]:
            t.start()

        logger.info("Controller started")
        try:
            while self.running:
                time.sleep(1)
        except KeyboardInterrupt:
            logger.info("Shutdown…")
        self.shutdown()

    def shutdown(self):
        self.running = False
        self.monitor.cleanup()
        self.serial.close()
        self.mqtt.close()
        logger.info("Stopped")
