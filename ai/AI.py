"""
AI Safety System - Face Detection (MQTT Only)
Detects human presence via camera and controls pump through MQTT.

Architecture:
  Camera → Face Detection → MQTT → ESP32 → Pump OFF

Designed for Wokwi simulation — all communication via MQTT broker.
"""

import time
import json
import logging
import signal
import sys
import os
import cv2

# ── Configuration ───────────────────────────────────────────

def load_config() -> dict:
    """Load config from config.json or use defaults"""
    config_path = os.path.join(os.path.dirname(__file__), 'config.json')
    try:
        with open(config_path, 'r') as f:
            return json.load(f)
    except (FileNotFoundError, json.JSONDecodeError):
        return {}


# ── Logging Setup ───────────────────────────────────────────

def setup_logging(config: dict) -> logging.Logger:
    """Configure structured logging"""
    log_config = config.get('logging', {})
    level = getattr(logging, log_config.get('level', 'INFO'))

    logger = logging.getLogger('ai_safety')
    logger.setLevel(level)

    formatter = logging.Formatter(
        '%(asctime)s | %(levelname)-8s | %(message)s',
        datefmt='%H:%M:%S'
    )

    # Console handler
    console = logging.StreamHandler()
    console.setFormatter(formatter)
    logger.addHandler(console)

    # File handler (optional)
    log_file = log_config.get('file')
    if log_file:
        log_dir = os.path.dirname(log_file)
        if log_dir and not os.path.exists(log_dir):
            os.makedirs(log_dir)
        from logging.handlers import RotatingFileHandler
        file_handler = RotatingFileHandler(log_file, maxBytes=5*1024*1024, backupCount=2)
        file_handler.setFormatter(formatter)
        logger.addHandler(file_handler)

    return logger


# ── MQTT Connection ─────────────────────────────────────────

class MQTTConnection:
    """Handles MQTT connection and messaging"""

    def __init__(self, config: dict, logger: logging.Logger):
        mqtt_config = config.get('mqtt', {})
        self.broker = mqtt_config.get('broker', 'broker.hivemq.com')
        self.port = mqtt_config.get('port', 1883)
        self.topic_control = mqtt_config.get('topic_control', 'grad/project/control')
        self.topic_sensors = mqtt_config.get('topic_sensors', 'grad/project/sensors')
        self.logger = logger
        self.client = None
        self.connected = False
        self.latest_sensors = {}

    def connect(self) -> bool:
        """Connect to MQTT broker"""
        try:
            import paho.mqtt.client as mqtt

            client_id = f"AI_Safety_{int(time.time())}"
            self.client = mqtt.Client(client_id=client_id)
            self.client.on_connect = self._on_connect
            self.client.on_disconnect = self._on_disconnect
            self.client.on_message = self._on_message

            self.logger.info(f"Connecting to MQTT: {self.broker}:{self.port}")
            self.client.connect(self.broker, self.port, 60)
            self.client.loop_start()

            # Wait for connection
            timeout = 5
            while not self.connected and timeout > 0:
                time.sleep(0.5)
                timeout -= 0.5

            return self.connected

        except ImportError:
            self.logger.error("paho-mqtt not installed. Run: pip install paho-mqtt")
            return False
        except Exception as e:
            self.logger.error(f"MQTT connection failed: {e}")
            return False

    def _on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            self.connected = True
            self.logger.info("✅ MQTT connected")
            client.subscribe(self.topic_sensors)
        else:
            self.logger.error(f"MQTT connection failed (rc={rc})")

    def _on_disconnect(self, client, userdata, rc):
        self.connected = False
        self.logger.warning("MQTT disconnected")

    def _on_message(self, client, userdata, msg):
        try:
            self.latest_sensors = json.loads(msg.payload.decode())
        except Exception:
            pass

    def send_command(self, command: str):
        """Send pump control command via MQTT"""
        if self.connected and self.client:
            try:
                self.client.publish(self.topic_control, command)
                self.logger.info(f"📡 TX: {command}")
            except Exception as e:
                self.logger.error(f"MQTT publish failed: {e}")
        else:
            self.logger.debug(f"[OFFLINE] Command: {command}")

    def disconnect(self):
        """Clean disconnect"""
        if self.client:
            self.client.loop_stop()
            self.client.disconnect()
            self.logger.info("MQTT disconnected")


# ── AI Security System ──────────────────────────────────────

class AISecuritySystem:
    """Face detection safety system for water station"""

    def __init__(self, config: dict, logger: logging.Logger, mqtt_conn: MQTTConnection):
        self.config = config
        self.logger = logger
        self.mqtt = mqtt_conn

        # Detection settings
        detect_config = config.get('detection', {})
        self.scale_factor = detect_config.get('scale_factor', 1.1)
        self.min_neighbors = detect_config.get('min_neighbors', 4)
        self.cooldown = detect_config.get('cooldown_seconds', 2)
        self.camera_index = detect_config.get('camera_index', 0)

        # State
        self.pump_state = None
        self.last_command_time = 0
        self.running = False
        self.start_time = time.time()

        # Statistics
        self.stats = {
            'total_detections': 0,
            'pump_stops': 0,
            'pump_resumes': 0,
            'frames_processed': 0
        }

        # Face cascade
        cascade_file = detect_config.get('cascade_file', 'haarcascade_frontalface_default.xml')
        self.face_cascade = self._load_cascade(cascade_file)

    def _load_cascade(self, filename: str):
        """Load Haar cascade for face detection"""
        cascade = cv2.CascadeClassifier(filename)
        if cascade.empty():
            # Try OpenCV's built-in path
            cascade = cv2.CascadeClassifier(
                os.path.join(cv2.data.haarcascades, filename)
            )
        if cascade.empty():
            self.logger.error(f"Cannot load cascade: {filename}")
            return None
        self.logger.info(f"✅ Cascade loaded: {filename}")
        return cascade

    def run(self):
        """Main detection loop"""
        if not self.face_cascade:
            self.logger.error("No face cascade loaded — cannot start")
            return

        cap = cv2.VideoCapture(self.camera_index)
        if not cap.isOpened():
            self.logger.error("Cannot open camera")
            return

        self.logger.info("✅ Camera opened")
        self.logger.info("Safety Logic:")
        self.logger.info("  👤 FACE DETECTED → PUMP_OFF (Safety Stop)")
        self.logger.info("  ✅ AREA CLEAR    → PUMP_ON  (Normal)")
        self.logger.info("Press 'q' to quit")
        self.logger.info("─" * 40)

        self.running = True

        try:
            while self.running:
                ret, frame = cap.read()
                if not ret:
                    self.logger.warning("Camera frame read failed")
                    time.sleep(0.1)
                    continue

                self.stats['frames_processed'] += 1

                # Detect faces
                gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
                faces = self.face_cascade.detectMultiScale(
                    gray, self.scale_factor, self.min_neighbors
                )

                now = time.time()
                face_count = len(faces)

                if face_count > 0:
                    self.stats['total_detections'] += 1
                    self._handle_threat(face_count, now)
                    self._draw_threat(frame, faces)
                else:
                    self._handle_clear(now)
                    self._draw_clear(frame)

                # Connection & stats bar
                self._draw_status_bar(frame, face_count)

                cv2.imshow('AI Safety System', frame)
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break

        finally:
            self.running = False
            self.logger.info("Shutting down...")
            self.mqtt.send_command("PUMP_OFF")

            cap.release()
            cv2.destroyAllWindows()

            self._print_stats()

    def _handle_threat(self, face_count: int, now: float):
        """Handle human detection — stop pump with cooldown"""
        if now - self.last_command_time < self.cooldown:
            return

        if self.pump_state != "OFF":
            self.logger.warning(f"⚠️  {face_count} face(s) detected — PUMP_OFF")
            self.mqtt.send_command("PUMP_OFF")
            self.pump_state = "OFF"
            self.last_command_time = now
            self.stats['pump_stops'] += 1

    def _handle_clear(self, now: float):
        """Handle area clear — resume pump with cooldown"""
        if now - self.last_command_time < self.cooldown:
            return

        if self.pump_state != "ON":
            self.logger.info("✅ Area clear — PUMP_ON")
            self.mqtt.send_command("PUMP_ON")
            self.pump_state = "ON"
            self.last_command_time = now
            self.stats['pump_resumes'] += 1

    def _draw_threat(self, frame, faces):
        """Draw red warning overlay"""
        for (x, y, w, h) in faces:
            cv2.rectangle(frame, (x, y), (x+w, y+h), (0, 0, 255), 3)
            cv2.putText(frame, "SAFETY STOP!", (x, y-10),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 2)

        cv2.rectangle(frame, (0, 0), (frame.shape[1], 40), (0, 0, 200), -1)
        cv2.putText(frame, "HUMAN DETECTED - PUMP STOPPED", (10, 28),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)

    def _draw_clear(self, frame):
        """Draw green safe overlay"""
        cv2.rectangle(frame, (0, 0), (frame.shape[1], 40), (0, 150, 0), -1)
        cv2.putText(frame, "AREA CLEAR - SYSTEM RUNNING", (10, 28),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)

    def _draw_status_bar(self, frame, face_count: int):
        """Draw bottom status bar"""
        h = frame.shape[0]
        w = frame.shape[1]

        cv2.rectangle(frame, (0, h-30), (w, h), (40, 40, 40), -1)

        mqtt_status = "MQTT: OK" if self.mqtt.connected else "MQTT: --"
        uptime = int(time.time() - self.start_time)
        status = f"{mqtt_status} | Faces: {face_count} | Stops: {self.stats['pump_stops']} | Up: {uptime}s"
        cv2.putText(frame, status, (10, h-10),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.45, (200, 200, 200), 1)

    def _print_stats(self):
        """Print session statistics"""
        uptime = int(time.time() - self.start_time)
        self.logger.info("─" * 40)
        self.logger.info("Session Statistics:")
        self.logger.info(f"  Uptime:      {uptime}s")
        self.logger.info(f"  Frames:      {self.stats['frames_processed']}")
        self.logger.info(f"  Detections:  {self.stats['total_detections']}")
        self.logger.info(f"  Pump stops:  {self.stats['pump_stops']}")
        self.logger.info(f"  Pump resumes:{self.stats['pump_resumes']}")
        self.logger.info("─" * 40)

    def stop(self):
        """Signal the system to stop"""
        self.running = False


# ── Main ────────────────────────────────────────────────────

def main():
    config = load_config()
    logger = setup_logging(config)

    logger.info("=" * 40)
    logger.info("  AI SAFETY SYSTEM")
    logger.info("=" * 40)

    # MQTT connection
    mqtt_conn = MQTTConnection(config, logger)
    mqtt_ok = mqtt_conn.connect()

    if not mqtt_ok:
        logger.warning("Running in DISPLAY-ONLY mode (no MQTT connection)")

    # AI System
    system = AISecuritySystem(config, logger, mqtt_conn)

    # Graceful shutdown
    def signal_handler(sig, frame):
        logger.info("Interrupt received")
        system.stop()

    signal.signal(signal.SIGINT, signal_handler)

    # Run
    try:
        system.run()
    finally:
        mqtt_conn.disconnect()
        logger.info("System stopped")


if __name__ == "__main__":
    main()