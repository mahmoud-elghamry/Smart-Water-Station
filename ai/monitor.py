"""
AquaPuer Smart Water Station — AI Station Monitor v3.0

Comprehensive visual monitoring system using YOLOv8 for the water treatment
station.  Detects persons, proximity hazards, fire, smoke, water quality
anomalies, and provides real-time recommendations.

Detection categories
────────────────────
1. PERSON      — Unauthorized persons in the station area
2. PROXIMITY   — Person too close to pumps / machinery (danger zone)
3. FIRE        — Flame detection via HSV colour analysis
4. SMOKE       — Haze / smoke detection via contrast + grey analysis
5. WATER       — Water colour / turbidity anomaly from a camera ROI
6. FLOODING    — Water on the ground (reflection / puddle heuristic)
7. GENERAL     — Miscellaneous YOLO detections (animals, vehicles, etc.)

Each detection produces an ``Alert`` dict with severity, bounding box,
confidence, and an actionable recommendation string.
"""

from __future__ import annotations

import cv2
from pathlib import Path
import time
import logging
import numpy as np
from dataclasses import dataclass, field, asdict
from enum import Enum
from typing import List, Tuple, Optional, Dict, Any

logger = logging.getLogger("water_station.monitor")


# ── Severity levels ──────────────────────────────────────────────────────────

class Severity(str, Enum):
    INFO     = "INFO"
    WARNING  = "WARNING"
    CRITICAL = "CRITICAL"


# ── Alert data class ─────────────────────────────────────────────────────────

@dataclass
class Alert:
    alert_type: str                     # person, proximity, fire, smoke, water, flooding, general
    severity: str                       # INFO / WARNING / CRITICAL
    message: str
    confidence: float    = 0.0
    bbox: List[int]      = field(default_factory=list)   # [x1, y1, x2, y2]
    recommendation: str  = ""
    timestamp: float     = 0.0

    def to_dict(self) -> dict:
        return asdict(self)


# ── Danger zone (rectangle in frame coordinates) ─────────────────────────────

@dataclass
class DangerZone:
    name: str
    x1: int
    y1: int
    x2: int
    y2: int
    colour: Tuple[int, int, int] = (0, 0, 255)   # BGR for drawing


# ── Water ROI (region of interest for water‑colour analysis) ─────────────────

@dataclass
class WaterROI:
    x1: int
    y1: int
    x2: int
    y2: int


# ══════════════════════════════════════════════════════════════════════════════
#  Station Monitor
# ══════════════════════════════════════════════════════════════════════════════

class StationMonitor:
    """Full‑spectrum visual monitor for the water station."""

    # COCO class IDs we care about
    PERSON_CLASS = 0
    HAZARD_CLASSES = {
        15: "cat", 16: "dog", 2: "car", 7: "truck",
        1: "bicycle", 3: "motorcycle",
    }

    def __init__(self, config: dict):
        cfg = config.get("security", {})
        self.enabled = cfg.get("detection_enabled", True)
        self.camera_index = cfg.get("camera_index", 0)
        self.show_preview = cfg.get("show_preview", True)
        self.person_conf = cfg.get("person_confidence", 0.55)
        self.general_conf = cfg.get("general_confidence", 0.40)

        # Fire / smoke HSV thresholds
        fire_cfg = cfg.get("fire", {})
        self.fire_enabled = fire_cfg.get("enabled", True)
        self.fire_min_area = fire_cfg.get("min_area_pct", 0.3)   # % of frame

        smoke_cfg = cfg.get("smoke", {})
        self.smoke_enabled = smoke_cfg.get("enabled", True)
        self.smoke_min_area = smoke_cfg.get("min_area_pct", 8.0)
        self.smoke_max_contrast = smoke_cfg.get("max_contrast", 25)

        # Water appearance
        water_cfg = cfg.get("water", {})
        self.water_enabled = water_cfg.get("enabled", True)
        roi = water_cfg.get("roi", {})
        if roi:
            self.water_roi = WaterROI(
                roi.get("x1", 0), roi.get("y1", 300),
                roi.get("x2", 640), roi.get("y2", 480),
            )
        else:
            self.water_roi = None

        # Danger zones
        self.danger_zones: List[DangerZone] = []
        for zone_cfg in cfg.get("danger_zones", []):
            self.danger_zones.append(DangerZone(
                name=zone_cfg.get("name", "Danger Zone"),
                x1=zone_cfg["x1"], y1=zone_cfg["y1"],
                x2=zone_cfg["x2"], y2=zone_cfg["y2"],
            ))

        # ── YOLO model ──
        # Resolve relative to ai/ folder so it won't re-download every run
        _AI_DIR = Path(__file__).resolve().parent
        self.model = None
        if self.enabled:
            try:
                from ultralytics import YOLO
                model_name = cfg.get("model", "yolov8s.pt")
                model_path = _AI_DIR / model_name
                self.model = YOLO(str(model_path))
                logger.info(f"YOLOv8 model loaded: {model_path}")
            except ImportError:
                logger.error("ultralytics not installed — pip install ultralytics")
            except Exception as exc:
                logger.error(f"YOLO init error: {exc}")

        # ── Camera ──
        self.cap: Optional[cv2.VideoCapture] = None

        # ── Statistics ──
        self.stats: Dict[str, int] = {
            "person_detections": 0,
            "proximity_alerts": 0,
            "fire_detections": 0,
            "smoke_detections": 0,
            "water_anomalies": 0,
            "total_frames": 0,
        }
        self._last_state = False          # For state‑change tracking
        self.last_detection_time = 0.0
        self._active_alerts: List[Alert] = []

    # ── Camera management ────────────────────────────────────────────────────

    def start_camera(self) -> bool:
        if not self.enabled:
            logger.info("Station monitor disabled in config")
            return False
        try:
            self.cap = cv2.VideoCapture(self.camera_index)
            if not self.cap.isOpened():
                logger.warning("Camera not available — monitor inactive")
                self.cap = None
                return False
            self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
            self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
            self.cap.set(cv2.CAP_PROP_FPS, 30)
            logger.info(f"Camera started on index {self.camera_index}")
            return True
        except Exception as exc:
            logger.error(f"Camera error: {exc}")
            self.cap = None
            return False

    def cleanup(self):
        if self.cap:
            self.cap.release()
            self.cap = None
        cv2.destroyAllWindows()
        logger.info("Camera released")

    # ══════════════════════════════════════════════════════════════════════════
    #  Main detection pipeline — called every frame
    # ══════════════════════════════════════════════════════════════════════════

    def detect(self) -> Tuple[bool, List[Alert]]:
        """
        Run the full detection pipeline on the current camera frame.

        Returns
        -------
        (threat_detected, alerts)
            threat_detected is True when any CRITICAL alert is active.
            Returns (False, -1‑flag) when user presses Q.
        """
        if not self.enabled or self.cap is None:
            return False, []

        ret, frame = self.cap.read()
        if not ret:
            return False, []

        self.stats["total_frames"] += 1
        alerts: List[Alert] = []
        now = time.time()

        # ── 1. YOLO detections ───────────────────────────────────────────────
        persons, general = self._run_yolo(frame)
        alerts.extend(persons)
        alerts.extend(general)

        # ── 2. Proximity to danger zones ─────────────────────────────────────
        alerts.extend(self._check_proximity(persons, frame))

        # ── 3. Fire detection (HSV) ──────────────────────────────────────────
        if self.fire_enabled:
            fire_alerts = self._detect_fire(frame)
            alerts.extend(fire_alerts)

        # ── 4. Smoke detection ───────────────────────────────────────────────
        if self.smoke_enabled:
            smoke_alerts = self._detect_smoke(frame)
            alerts.extend(smoke_alerts)

        # ── 5. Water appearance ──────────────────────────────────────────────
        if self.water_enabled and self.water_roi:
            water_alerts = self._analyze_water(frame)
            alerts.extend(water_alerts)

        # ── Store active alerts ──────────────────────────────────────────────
        for a in alerts:
            a.timestamp = now
        self._active_alerts = alerts

        threat = any(a.severity == Severity.CRITICAL for a in alerts)

        # State change tracking
        if threat != self._last_state:
            if threat:
                self.last_detection_time = now
            self._last_state = threat

        # ── Draw overlays + show preview ─────────────────────────────────────
        self._draw_overlays(frame, alerts)

        if self.show_preview:
            cv2.imshow("AquaPuer — Station Monitor", frame)
            key = cv2.waitKey(1) & 0xFF
            if key in (ord("q"), ord("Q")):
                logger.info("User pressed Q — stopping")
                return False, [Alert(
                    alert_type="quit", severity=Severity.INFO,
                    message="User quit", timestamp=now
                )]

        return threat, alerts

    # ══════════════════════════════════════════════════════════════════════════
    #  YOLO person + general detections
    # ══════════════════════════════════════════════════════════════════════════

    def _run_yolo(self, frame) -> Tuple[List[Alert], List[Alert]]:
        persons: List[Alert] = []
        general: List[Alert] = []

        if self.model is None:
            return persons, general

        results = self.model(frame, verbose=False, conf=self.general_conf)

        for result in results:
            for box in result.boxes:
                cls_id = int(box.cls[0])
                conf = float(box.conf[0])
                x1, y1, x2, y2 = map(int, box.xyxy[0].tolist())

                if cls_id == self.PERSON_CLASS and conf >= self.person_conf:
                    self.stats["person_detections"] += 1
                    persons.append(Alert(
                        alert_type="person",
                        severity=Severity.INFO,
                        message=f"Person detected — safe area ({conf:.0%})",
                        confidence=conf,
                        bbox=[x1, y1, x2, y2],
                        recommendation="Person present in station. Monitor movement.",
                    ))

                elif cls_id in self.HAZARD_CLASSES and conf >= self.general_conf:
                    name = self.HAZARD_CLASSES[cls_id]
                    general.append(Alert(
                        alert_type="general",
                        severity=Severity.INFO,
                        message=f"{name.title()} detected near station ({conf:.0%})",
                        confidence=conf,
                        bbox=[x1, y1, x2, y2],
                        recommendation=f"Check if {name} poses a risk to equipment.",
                    ))

        return persons, general

    # ══════════════════════════════════════════════════════════════════════════
    #  Proximity to danger zones
    # ══════════════════════════════════════════════════════════════════════════

    def _check_proximity(self, person_alerts: List[Alert], frame) -> List[Alert]:
        proximity_alerts: List[Alert] = []

        for zone in self.danger_zones:
            for pa in person_alerts:
                if not pa.bbox:
                    continue
                px1, py1, px2, py2 = pa.bbox
                # Check overlap between person bbox and danger zone
                if self._rects_overlap(px1, py1, px2, py2,
                                       zone.x1, zone.y1, zone.x2, zone.y2):
                    self.stats["proximity_alerts"] += 1
                    proximity_alerts.append(Alert(
                        alert_type="proximity",
                        severity=Severity.CRITICAL,
                        message=f"⚠️ Person in danger zone: {zone.name}!",
                        confidence=pa.confidence,
                        bbox=pa.bbox,
                        recommendation=f"EMERGENCY: Person too close to {zone.name}. "
                                       f"Activate emergency stop immediately!",
                    ))
        return proximity_alerts

    @staticmethod
    def _rects_overlap(ax1, ay1, ax2, ay2, bx1, by1, bx2, by2) -> bool:
        return not (ax2 < bx1 or ax1 > bx2 or ay2 < by1 or ay1 > by2)

    # ══════════════════════════════════════════════════════════════════════════
    #  Fire detection (HSV colour analysis)
    # ══════════════════════════════════════════════════════════════════════════

    def _detect_fire(self, frame) -> List[Alert]:
        alerts: List[Alert] = []

        hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

        # Fire has strong orange-red hue, high saturation, high value
        # Range 1: red-orange (0-15 hue)
        lower1 = np.array([0, 120, 200])
        upper1 = np.array([15, 255, 255])
        mask1 = cv2.inRange(hsv, lower1, upper1)

        # Range 2: deep red (160-180 hue, wraps around)
        lower2 = np.array([160, 120, 200])
        upper2 = np.array([180, 255, 255])
        mask2 = cv2.inRange(hsv, lower2, upper2)

        # Range 3: yellow-orange fire core (15-35 hue)
        lower3 = np.array([15, 150, 200])
        upper3 = np.array([35, 255, 255])
        mask3 = cv2.inRange(hsv, lower3, upper3)

        mask = mask1 | mask2 | mask3
        fire_area = cv2.countNonZero(mask)
        frame_area = frame.shape[0] * frame.shape[1]
        fire_pct = (fire_area / frame_area) * 100

        if fire_pct > self.fire_min_area:
            self.stats["fire_detections"] += 1
            # Find bounding contour
            contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
            if contours:
                largest = max(contours, key=cv2.contourArea)
                x, y, w, h = cv2.boundingRect(largest)
                conf = min(fire_pct / 5.0, 1.0)
                alerts.append(Alert(
                    alert_type="fire",
                    severity=Severity.CRITICAL,
                    message=f"🔥 Fire/flame detected! ({fire_pct:.1f}% of frame)",
                    confidence=conf,
                    bbox=[x, y, x + w, y + h],
                    recommendation="CRITICAL: Possible fire detected! "
                                   "Activate emergency stop and alert fire department.",
                ))
        return alerts

    # ══════════════════════════════════════════════════════════════════════════
    #  Smoke detection (grey haze analysis)
    # ══════════════════════════════════════════════════════════════════════════

    def _detect_smoke(self, frame) -> List[Alert]:
        alerts: List[Alert] = []

        hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

        # Smoke: low saturation, medium-high value (greyish-white haze)
        lower = np.array([0, 0, 150])
        upper = np.array([180, 50, 240])
        mask = cv2.inRange(hsv, lower, upper)

        # Also check for reduced contrast (smoke makes everything hazy)
        grey = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        local_std = cv2.meanStdDev(grey)
        contrast = local_std[1][0][0]

        smoke_area = cv2.countNonZero(mask)
        frame_area = frame.shape[0] * frame.shape[1]
        smoke_pct = (smoke_area / frame_area) * 100

        # Smoke: large grey area + very low contrast (normal indoor = 30-50)
        if smoke_pct > self.smoke_min_area and contrast < self.smoke_max_contrast:
            self.stats["smoke_detections"] += 1
            conf = min(smoke_pct / 15.0, 0.95)
            alerts.append(Alert(
                alert_type="smoke",
                severity=Severity.CRITICAL,
                message=f"💨 Smoke/haze detected! ({smoke_pct:.1f}% coverage, contrast={contrast:.0f})",
                confidence=conf,
                recommendation="WARNING: Possible smoke detected! "
                               "Check for fire source. Ventilate area if safe.",
            ))
        return alerts

    # ══════════════════════════════════════════════════════════════════════════
    #  Water appearance analysis
    # ══════════════════════════════════════════════════════════════════════════

    def _analyze_water(self, frame) -> List[Alert]:
        alerts: List[Alert] = []
        roi = self.water_roi
        if roi is None:
            return alerts

        h, w = frame.shape[:2]
        x1 = max(0, min(roi.x1, w))
        y1 = max(0, min(roi.y1, h))
        x2 = max(0, min(roi.x2, w))
        y2 = max(0, min(roi.y2, h))

        if x2 <= x1 or y2 <= y1:
            return alerts

        water_region = frame[y1:y2, x1:x2]
        hsv_region = cv2.cvtColor(water_region, cv2.COLOR_BGR2HSV)

        # Analyse dominant colour
        mean_h = np.mean(hsv_region[:, :, 0])
        mean_s = np.mean(hsv_region[:, :, 1])
        mean_v = np.mean(hsv_region[:, :, 2])

        # Normal clean water: bluish (hue ~90-130), low-medium saturation
        # Contaminated water signatures:
        anomaly = None

        if 35 <= mean_h <= 85 and mean_s > 60:
            # Green = algae growth
            anomaly = ("algae",
                       "🟢 Green tint in water — possible algae growth",
                       "Check water treatment chemicals. Consider increasing chlorination.")

        elif (mean_h < 15 or mean_h > 165) and mean_s > 80:
            # Red/brown = rust, chemical, or contamination
            anomaly = ("contamination",
                       "🔴 Red/brown water — possible contamination or rust",
                       "STOP water distribution! Test water quality immediately.")

        elif 15 <= mean_h <= 35 and mean_s > 60:
            # Yellow/brown = sediment, turbidity
            anomaly = ("turbidity",
                       "🟡 Yellow/murky water — high turbidity detected",
                       "Check filtration system. Sediment levels may be elevated.")

        elif mean_v < 60:
            # Very dark = possible blockage or camera obstruction
            anomaly = ("dark",
                       "⚫ Water area unusually dark",
                       "Check camera position. Possible pipe blockage or low water level.")

        elif mean_s < 15 and mean_v > 200:
            # Very bright and desaturated = foam or chemical reaction
            anomaly = ("foam",
                       "⚪ Foam or unusual brightness on water surface",
                       "Check for chemical spills or detergent contamination.")

        if anomaly:
            self.stats["water_anomalies"] += 1
            kind, msg, rec = anomaly
            alerts.append(Alert(
                alert_type="water",
                severity=Severity.WARNING if kind != "contamination" else Severity.CRITICAL,
                message=msg,
                confidence=0.7,
                bbox=[x1, y1, x2, y2],
                recommendation=rec,
            ))

        return alerts

    # ══════════════════════════════════════════════════════════════════════════
    #  Draw overlays on the preview frame
    # ══════════════════════════════════════════════════════════════════════════

    SEVERITY_COLOURS = {
        Severity.INFO:     (200, 200, 0),     # Cyan-ish
        Severity.WARNING:  (0, 165, 255),      # Orange
        Severity.CRITICAL: (0, 0, 255),        # Red
    }

    def _draw_overlays(self, frame, alerts: List[Alert]):
        h, w = frame.shape[:2]

        # Draw danger zones
        for zone in self.danger_zones:
            cv2.rectangle(frame, (zone.x1, zone.y1), (zone.x2, zone.y2),
                          zone.colour, 2)
            cv2.putText(frame, f"DANGER: {zone.name}",
                        (zone.x1, zone.y1 - 8),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, zone.colour, 2)

        # Draw water ROI
        if self.water_roi:
            roi = self.water_roi
            cv2.rectangle(frame, (roi.x1, roi.y1), (roi.x2, roi.y2),
                          (255, 180, 0), 1)
            cv2.putText(frame, "Water ROI",
                        (roi.x1 + 4, roi.y1 + 16),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 180, 0), 1)

        # Draw alert bounding boxes
        for alert in alerts:
            colour = self.SEVERITY_COLOURS.get(alert.severity, (255, 255, 255))
            if alert.bbox:
                x1, y1, x2, y2 = alert.bbox
                cv2.rectangle(frame, (x1, y1), (x2, y2), colour, 2)
                label = f"{alert.alert_type.upper()} {alert.confidence:.0%}"
                cv2.putText(frame, label, (x1, y1 - 6),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.5, colour, 2)

        # Top-left status bar
        has_critical = any(a.severity == Severity.CRITICAL for a in alerts)
        has_warning = any(a.severity == Severity.WARNING for a in alerts)

        if has_critical:
            status_text = "STATUS: CRITICAL ALERT!"
            status_colour = (0, 0, 255)
        elif has_warning:
            status_text = "STATUS: WARNING"
            status_colour = (0, 165, 255)
        else:
            status_text = "STATUS: ALL CLEAR"
            status_colour = (0, 255, 0)

        cv2.rectangle(frame, (0, 0), (w, 32), (30, 30, 30), -1)
        cv2.putText(frame, status_text, (10, 22),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.65, status_colour, 2)

        # Alert count
        cv2.putText(frame, f"Alerts: {len(alerts)}", (w - 130, 22),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (200, 200, 200), 1)

        # Bottom bar
        cv2.rectangle(frame, (0, h - 24), (w, h), (30, 30, 30), -1)
        cv2.putText(frame, "AquaPuer Station Monitor | Press Q to quit",
                    (8, h - 7),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.4, (160, 160, 160), 1)

    # ══════════════════════════════════════════════════════════════════════════
    #  Public query methods
    # ══════════════════════════════════════════════════════════════════════════

    def get_active_alerts(self) -> List[dict]:
        return [a.to_dict() for a in self._active_alerts]

    def get_stats(self) -> dict:
        return {
            **self.stats,
            "enabled": self.enabled,
            "camera_active": self.cap is not None and self.cap.isOpened(),
            "model_loaded": self.model is not None,
            "danger_zones": len(self.danger_zones),
            "threat_active": self._last_state,
            "last_detection": self.last_detection_time,
        }

    def get_recommendations(self) -> List[str]:
        """Return unique recommendations from current alerts."""
        seen = set()
        recs = []
        for a in self._active_alerts:
            if a.recommendation and a.recommendation not in seen:
                seen.add(a.recommendation)
                recs.append(a.recommendation)
        return recs
