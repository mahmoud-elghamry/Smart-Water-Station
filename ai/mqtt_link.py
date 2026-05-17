"""MQTT link with auto-reconnect."""

import json
import logging
from typing import Optional, Callable

logger = logging.getLogger("water_station.mqtt")

try:
    import paho.mqtt.client as mqtt
    HAS_MQTT = True
except ImportError:
    HAS_MQTT = False
    logger.warning("paho-mqtt not installed — MQTT disabled")


class MQTTLink:
    def __init__(self, config: dict, on_command: Optional[Callable] = None):
        mc = config.get("mqtt", {})
        self.enabled = mc.get("enabled", False) and HAS_MQTT
        self.broker = mc.get("broker", "localhost")
        self.port = mc.get("port", 1883)
        self.user = mc.get("username", "")
        self.password = mc.get("password", "")
        self.base = mc.get("base_topic", "aquapuer")
        self.on_command = on_command

        self._client: Optional[mqtt.Client] = None
        self.connected = False

    def topic(self, sub: str) -> str:
        return f"{self.base}/{sub}"

    def connect(self):
        if not self.enabled:
            logger.info("MQTT disabled in config")
            return

        self._client = mqtt.Client(
            mqtt.CallbackAPIVersion.VERSION2, client_id="aquapuer-ai"
        )
        if self.user:
            self._client.username_pw_set(self.user, self.password)

        self._client.on_connect = self._on_connect
        self._client.on_disconnect = self._on_disconnect
        self._client.on_message = self._on_message

        try:
            self._client.connect(self.broker, self.port, keepalive=60)
            self._client.loop_start()
            logger.info(f"MQTT connecting to {self.broker}:{self.port}")
        except Exception as e:
            logger.error(f"MQTT error: {e}")

    def _on_connect(self, client, userdata, flags, rc, properties=None):
        self.connected = True
        logger.info(f"MQTT connected (rc={rc})")
        for t in ("command", "control"):
            client.subscribe(self.topic(t))
        logger.info(f"MQTT subscribed: {self.topic('command')}, {self.topic('control')}")

    def _on_disconnect(self, client, userdata, flags, rc, properties=None):
        self.connected = False
        logger.warning(f"MQTT disconnected (rc={rc})")

    def _on_message(self, client, userdata, msg):
        try:
            payload = json.loads(msg.payload.decode("utf-8"))
            logger.info(f"RX MQTT [{msg.topic}] {payload}")
            if self.on_command and payload.get("cmd"):
                self.on_command(payload, source="mqtt")
        except json.JSONDecodeError:
            logger.warning(f"MQTT bad JSON: {msg.payload}")

    def publish(self, subtopic: str, data: dict):
        if self._client and self.connected:
            try:
                self._client.publish(self.topic(subtopic), json.dumps(data), qos=1)
            except Exception as e:
                logger.error(f"MQTT publish error: {e}")

    def close(self):
        if self._client:
            self._client.loop_stop()
            self._client.disconnect()
