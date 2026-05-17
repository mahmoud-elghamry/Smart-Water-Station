"""Serial link with finite retries."""

import serial
import json
import time
import logging
from typing import Optional

logger = logging.getLogger("water_station.serial")


class SerialLink:
    def __init__(self, config: dict):
        sc = config.get("serial", {})
        self.port = sc.get("port", "COM3")
        self.baud = sc.get("baud_rate", 115200)
        self.timeout = sc.get("timeout", 1.0)
        self.delay = sc.get("reconnect_delay", 3)
        self.max_retries = sc.get("max_retries", 5)
        self._conn: Optional[serial.Serial] = None

    @property
    def connected(self) -> bool:
        return self._conn is not None and self._conn.is_open

    @property
    def in_waiting(self) -> int:
        return self._conn.in_waiting if self.connected else 0

    def connect(self) -> bool:
        for attempt in range(1, self.max_retries + 1):
            try:
                self._conn = serial.Serial(self.port, self.baud, timeout=self.timeout)
                logger.info(f"Serial connected: {self.port}")
                return True
            except serial.SerialException as e:
                logger.error(f"Serial attempt {attempt}/{self.max_retries}: {e}")
                if attempt < self.max_retries:
                    time.sleep(self.delay)

        logger.warning(f"Serial: gave up after {self.max_retries} attempts")
        return False

    def send(self, payload: dict) -> bool:
        if not self.connected:
            return False
        try:
            self._conn.write((json.dumps(payload) + "\n").encode("utf-8"))
            logger.debug(f"TX → {payload}")
            return True
        except Exception as e:
            logger.error(f"TX error: {e}")
            return False

    def readline(self) -> Optional[str]:
        if not self.connected:
            return None
        try:
            return self._conn.readline().decode("utf-8").strip() or None
        except Exception as e:
            logger.error(f"RX error: {e}")
            return None

    def close(self):
        if self._conn:
            self._conn.close()
            self._conn = None
