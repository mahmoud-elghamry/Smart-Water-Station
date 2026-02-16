"""
Smart Water Station V2.0 - Security Monitor Module
Provides face detection with live camera preview
"""

import cv2
import time
import logging
from typing import Optional, Tuple

logger = logging.getLogger(__name__)


class SecurityMonitor:
    """Monitors camera feed for security threats (face detection)"""
    
    def __init__(self, config: dict):
        self.config = config.get('security', {})
        self.enabled = self.config.get('detection_enabled', True)
        self.mode = self.config.get('detection_mode', 'face')
        self.sensitivity = self.config.get('sensitivity', 1.1)
        self.min_neighbors = self.config.get('min_neighbors', 4)
        self.camera_index = self.config.get('camera_index', 0)
        self.show_preview = self.config.get('show_preview', True)
        
        # Initialize face cascade
        self.face_cascade = cv2.CascadeClassifier(
            cv2.data.haarcascades + 'haarcascade_frontalface_default.xml'
        )
        
        self.cap: Optional[cv2.VideoCapture] = None
        self.last_detection_time = 0
        self.detection_count = 0
        self._last_state = False  # For state change detection
        
    def start_camera(self) -> bool:
        """Initialize camera capture"""
        if not self.enabled:
            logger.info("Security monitoring disabled in config")
            return False
            
        try:
            self.cap = cv2.VideoCapture(self.camera_index)
            
            if not self.cap.isOpened():
                logger.warning("Could not open camera, using simulation mode")
                self.cap = None
                return False
                
            # Set camera properties
            self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
            self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
            self.cap.set(cv2.CAP_PROP_FPS, 30)
            
            logger.info(f"Camera initialized on index {self.camera_index}")
            return True
            
        except Exception as e:
            logger.error(f"Camera initialization error: {e}")
            self.cap = None
            return False
            
    def detect_threat(self) -> Tuple[bool, int]:
        """
        Check for security threats with visual preview.
        Only logs on STATE CHANGE, not every frame!
        
        Returns:
            Tuple of (threat_detected, face_count)
        """
        if not self.enabled or self.cap is None:
            return False, 0
            
        try:
            ret, frame = self.cap.read()
            if not ret:
                return False, 0
                
            # Convert to grayscale for detection
            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            
            # Detect faces
            faces = self.face_cascade.detectMultiScale(
                gray,
                scaleFactor=self.sensitivity,
                minNeighbors=self.min_neighbors,
                minSize=(30, 30)
            )
            
            face_count = len(faces)
            threat_detected = face_count > 0
            
            # Draw rectangles on faces
            for (x, y, w, h) in faces:
                cv2.rectangle(frame, (x, y), (x+w, y+h), (0, 0, 255), 3)
                cv2.putText(frame, "THREAT!", (x, y-10), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.9, (0, 0, 255), 2)
            
            # Add status text
            status = "THREAT DETECTED!" if threat_detected else "MONITORING..."
            color = (0, 0, 255) if threat_detected else (0, 255, 0)
            cv2.putText(frame, status, (10, 30), 
                       cv2.FONT_HERSHEY_SIMPLEX, 1, color, 2)
            cv2.putText(frame, f"Faces: {face_count}", (10, 60), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
            cv2.putText(frame, "Press 'Q' on this window to quit", (10, 460), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (200, 200, 200), 1)
            
            # Show preview window
            if self.show_preview:
                cv2.imshow("Smart Water Station - Security Monitor", frame)
                key = cv2.waitKey(1) & 0xFF
                if key == ord('q') or key == ord('Q'):
                    logger.info("User pressed Q - stopping...")
                    return False, -1  # Special signal to stop
            
            # STATE CHANGE DETECTION - Only update when state changes!
            if threat_detected != self._last_state:
                if threat_detected:
                    self.detection_count += 1
                    self.last_detection_time = time.time()
                self._last_state = threat_detected
            
            return threat_detected, face_count
            
        except Exception as e:
            logger.error(f"Detection error: {e}")
            return False, 0
            
    def get_stats(self) -> dict:
        """Get detection statistics"""
        return {
            'enabled': self.enabled,
            'mode': self.mode,
            'detection_count': self.detection_count,
            'last_detection': self.last_detection_time,
            'camera_active': self.cap is not None and self.cap.isOpened()
        }
        
    def cleanup(self):
        """Release camera resources"""
        if self.cap:
            self.cap.release()
            self.cap = None
        cv2.destroyAllWindows()
        logger.info("Camera released")
