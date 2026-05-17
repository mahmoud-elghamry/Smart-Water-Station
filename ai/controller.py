"""AIController — orchestrates Serial + MQTT + FastAPI/WS with DataBus sync."""

import time
import json
import threading
import asyncio
import logging
from typing import Optional, List

from fastapi import WebSocket

from monitor import StationMonitor, Severity
from serial_link import SerialLink
from mqtt_link import MQTTLink

logger = logging.getLogger("water_station.controller")


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

        # WebSocket
        self.ws_clients: List[WebSocket] = []
        self._loop: Optional[asyncio.AbstractEventLoop] = None

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
                self.latest_telemetry[s] = msg
                self.broadcast_telemetry(msg)
            elif t == "state":
                self.latest_state = msg
                self.mqtt.publish("state", msg)
                self._ws_broadcast({"type": "state_update", "data": msg})
        except json.JSONDecodeError:
            pass

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
