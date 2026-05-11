"""
Smart Water Station V2.0 - AI Controller Module
Main entry point with FastAPI and enhanced logging
"""

import serial
import time
import json
import threading
import asyncio
import logging
import os
from contextlib import asynccontextmanager
from logging.handlers import RotatingFileHandler
from typing import Optional, List
from security import SecurityMonitor

# FastAPI imports
from fastapi import FastAPI, WebSocket, WebSocketDisconnect, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel, Field
import uvicorn


# ============== Pydantic Models ==============
class CommandRequest(BaseModel):
    """Request model for sending commands to ESP32"""
    cmd: str = Field(..., description="Command name (e.g., EMERGENCY_STOP, SET_PUMP)")
    state: Optional[str] = Field(None, description="Command state (e.g., ON, OFF)")
    value: Optional[float] = Field(None, description="Optional numeric value")


class CommandResponse(BaseModel):
    """Response model for command execution"""
    success: bool
    message: str = "Command sent"


class StatusResponse(BaseModel):
    """Response model for system status"""
    connected: bool
    state: dict
    telemetry: dict
    security: dict
    threat_active: bool


def load_config(config_path: str = 'config.json') -> dict:
    """Load configuration from JSON file"""
    try:
        with open(config_path, 'r') as f:
            return json.load(f)
    except FileNotFoundError:
        print(f"Config file not found: {config_path}, using defaults")
        return {}
    except json.JSONDecodeError as e:
        print(f"Config parse error: {e}, using defaults")
        return {}


def setup_logging(config: dict) -> logging.Logger:
    """Configure logging based on config"""
    log_config = config.get('logging', {})
    
    # Create logs directory if needed
    log_file = log_config.get('file', 'logs/water_station.log')
    log_dir = os.path.dirname(log_file)
    if log_dir and not os.path.exists(log_dir):
        os.makedirs(log_dir)
    
    # Configure logger
    logger = logging.getLogger('water_station')
    logger.setLevel(getattr(logging, log_config.get('level', 'INFO')))
    
    # Console handler
    console_handler = logging.StreamHandler()
    console_handler.setFormatter(logging.Formatter(
        '%(asctime)s | %(levelname)-8s | %(message)s',
        datefmt='%H:%M:%S'
    ))
    logger.addHandler(console_handler)
    
    # File handler (if enabled)
    if log_config.get('enabled', True):
        file_handler = RotatingFileHandler(
            log_file,
            maxBytes=log_config.get('max_size_mb', 10) * 1024 * 1024,
            backupCount=log_config.get('backup_count', 3)
        )
        file_handler.setFormatter(logging.Formatter(
            '%(asctime)s | %(levelname)-8s | %(name)s | %(message)s'
        ))
        logger.addHandler(file_handler)
    
    return logger


class AIController:
    """Main controller for AI security system"""
    
    def __init__(self, config: dict):
        self.config = config
        self.logger = logging.getLogger('water_station.controller')
        
        # Serial config
        serial_config = config.get('serial', {})
        self.serial_port = serial_config.get('port', 'COM3')
        self.baud_rate = serial_config.get('baud_rate', 115200)
        self.serial_timeout = serial_config.get('timeout', 1.0)
        self.reconnect_delay = serial_config.get('reconnect_delay', 5)
        
        # Alert config
        alert_config = config.get('alerts', {})
        self.emergency_on_detection = alert_config.get('emergency_stop_on_detection', True)
        self.cooldown_seconds = alert_config.get('cooldown_seconds', 5)
        
        # State
        self.security = SecurityMonitor(config)
        self.serial_conn: Optional[serial.Serial] = None
        self.running = True
        self.last_threat_status = False
        self.last_command_time = 0
        
        # Telemetry storage
        self.latest_telemetry = {}
        self.latest_state = {'state': 'UNKNOWN', 'error': 'NONE'}
        
        # WebSocket clients for real-time updates
        self.ws_clients: List[WebSocket] = []
        
    def connect_serial(self) -> bool:
        """Establish serial connection with retry logic"""
        while self.running:
            try:
                self.serial_conn = serial.Serial(
                    self.serial_port,
                    self.baud_rate,
                    timeout=self.serial_timeout
                )
                self.logger.info(f"Connected to {self.serial_port}")
                return True
            except serial.SerialException as e:
                self.logger.error(f"Serial error: {e}")
                self.logger.info(f"Retrying in {self.reconnect_delay}s...")
                time.sleep(self.reconnect_delay)
        return False
        
    def send_command(self, cmd: str, state: Optional[str] = None, value: Optional[float] = None):
        """Send command to ESP32"""
        if not self.serial_conn or not self.serial_conn.is_open:
            self.logger.warning("Serial not connected, cannot send command")
            return False
            
        payload = {'cmd': cmd}
        if state is not None:
            payload['state'] = state
        if value is not None:
            payload['value'] = value
            
        try:
            data = json.dumps(payload) + '\n'
            self.serial_conn.write(data.encode('utf-8'))
            self.logger.info(f"TX: {data.strip()}")
            return True
        except Exception as e:
            self.logger.error(f"TX Error: {e}")
            return False
            
    def monitor_security(self):
        """Security monitoring thread"""
        self.security.start_camera()
        
        while self.running:
            threat_detected, face_count = self.security.detect_threat()
            
            # Check for Q key press (face_count = -1)
            if face_count == -1:
                self.running = False
                break
            
            current_time = time.time()
            
            # State change detection with cooldown
            if threat_detected and not self.last_threat_status:
                if current_time - self.last_command_time >= self.cooldown_seconds:
                    self.logger.warning(f"ALERT: {face_count} face(s) detected! Sending EMERGENCY_STOP")
                    
                    if self.emergency_on_detection:
                        self.send_command('EMERGENCY_STOP', 'ON')
                        self.last_command_time = current_time
                        
                    self.last_threat_status = True
                    
                    # Broadcast threat to WebSocket clients
                    self._broadcast_ws({
                        'type': 'security_alert',
                        'threat': True,
                        'face_count': face_count,
                        'timestamp': current_time
                    })
                    
            elif not threat_detected and self.last_threat_status:
                self.logger.info("Security clear")
                self.last_threat_status = False
                
                # Broadcast clear to WebSocket clients
                self._broadcast_ws({
                    'type': 'security_alert',
                    'threat': False,
                    'face_count': 0,
                    'timestamp': current_time
                })
                
            time.sleep(0.05)  # Faster loop for smoother video
    
    def _broadcast_ws(self, data: dict):
        """Broadcast data to all connected WebSocket clients"""
        disconnected = []
        for ws in self.ws_clients:
            try:
                asyncio.run(ws.send_json(data))
            except Exception:
                disconnected.append(ws)
        for ws in disconnected:
            self.ws_clients.remove(ws)
            
    def read_telemetry(self):
        """Telemetry reading thread"""
        while self.running:
            if self.serial_conn and self.serial_conn.is_open:
                try:
                    if self.serial_conn.in_waiting:
                        line = self.serial_conn.readline().decode('utf-8').strip()
                        if line:
                            self.process_incoming(line)
                except Exception as e:
                    self.logger.error(f"RX Error: {e}")
            time.sleep(0.01)
            
    def process_incoming(self, data: str):
        """Process incoming JSON data"""
        try:
            json_data = json.loads(data)
            msg_type = json_data.get('type', 'unknown')
            
            if msg_type == 'telemetry':
                sensor = json_data.get('sensor', 'unknown')
                self.latest_telemetry[sensor] = json_data
                self.logger.debug(f"RX Telemetry: {sensor}={json_data.get('value')}")
                
                # Broadcast telemetry to WebSocket clients
                self._broadcast_ws({
                    'type': 'telemetry_update',
                    'sensor': sensor,
                    'data': json_data
                })
                
            elif msg_type == 'state':
                self.latest_state = json_data
                self.logger.info(f"RX State: {json_data.get('state')} (Error: {json_data.get('error')})")
                
                # Broadcast state change to WebSocket clients
                self._broadcast_ws({
                    'type': 'state_update',
                    'data': json_data
                })
                
            elif msg_type == 'pong':
                self.logger.debug("RX Pong")
                
            elif msg_type == 'error':
                self.logger.error(f"Device Error: {json_data.get('error')}")
                
            else:
                self.logger.debug(f"RX: {data}")
                
        except json.JSONDecodeError:
            self.logger.warning(f"RX (non-JSON): {data}")
            
    def get_status(self) -> dict:
        """Get current system status for API"""
        return {
            'connected': self.serial_conn is not None and self.serial_conn.is_open,
            'state': self.latest_state,
            'telemetry': self.latest_telemetry,
            'security': self.security.get_stats(),
            'threat_active': self.last_threat_status
        }
        
    def run(self):
        """Main run loop"""
        self.logger.info("=" * 50)
        self.logger.info("Smart Water Station V2.0 - AI Controller")
        self.logger.info("=" * 50)
        
        # Connect to serial (non-blocking for development)
        # Uncomment next line for production:
        self.connect_serial()
        
        # Start threads
        t_security = threading.Thread(target=self.monitor_security, name='Security')
        t_telemetry = threading.Thread(target=self.read_telemetry, name='Telemetry')
        
        t_security.start()
        t_telemetry.start()
        
        self.logger.info("AI Controller started successfully")
        
        try:
            while self.running:
                time.sleep(1)
        except KeyboardInterrupt:
            self.logger.info("Shutdown requested...")
            
        self.shutdown()
        t_security.join()
        t_telemetry.join()
        
    def shutdown(self):
        """Clean shutdown"""
        self.running = False
        self.security.cleanup()
        if self.serial_conn:
            self.serial_conn.close()
        self.logger.info("AI Controller stopped")


# ============== FastAPI Application ==============
def create_app(controller: AIController) -> FastAPI:
    """Create FastAPI application with all routes"""
    
    @asynccontextmanager
    async def lifespan(app: FastAPI):
        """Startup and shutdown events"""
        controller.logger.info("🚀 FastAPI server starting...")
        yield
        controller.logger.info("⛔ FastAPI server shutting down...")
    
    app = FastAPI(
        title="Smart Water Station API",
        description="AI-powered water station monitoring and control system",
        version="2.0.0",
        lifespan=lifespan
    )
    
    # CORS middleware
    app.add_middleware(
        CORSMiddleware,
        allow_origins=["*"],
        allow_credentials=True,
        allow_methods=["*"],
        allow_headers=["*"],
    )
    
    # ── REST Endpoints ──────────────────────────────────
    
    @app.get("/api/status", response_model=StatusResponse, tags=["Monitoring"])
    async def get_status():
        """Get full system status including telemetry, state, and security info"""
        return controller.get_status()
    
    @app.post("/api/command", response_model=CommandResponse, tags=["Control"])
    async def send_command(req: CommandRequest):
        """Send a command to the ESP32 controller"""
        success = controller.send_command(req.cmd, req.state, req.value)
        if not success:
            raise HTTPException(
                status_code=503,
                detail="Serial connection not available"
            )
        return CommandResponse(success=True, message=f"Command '{req.cmd}' sent successfully")
    
    @app.get("/api/telemetry", tags=["Monitoring"])
    async def get_telemetry():
        """Get latest sensor telemetry data"""
        return controller.latest_telemetry
    
    @app.get("/api/security", tags=["Security"])
    async def get_security():
        """Get security detection statistics"""
        return controller.security.get_stats()
    
    @app.get("/api/health", tags=["System"])
    async def health_check():
        """Health check endpoint"""
        return {
            "status": "healthy",
            "serial_connected": controller.serial_conn is not None and controller.serial_conn.is_open,
            "camera_active": controller.security.cap is not None,
            "uptime": "running"
        }
    
    # ── WebSocket for Real-time Updates ─────────────────
    
    @app.websocket("/ws/live")
    async def websocket_live(ws: WebSocket):
        """
        WebSocket endpoint for real-time telemetry and security alerts.
        Connect to ws://host:port/ws/live to receive live updates.
        """
        await ws.accept()
        controller.ws_clients.append(ws)
        controller.logger.info(f"📡 WebSocket client connected ({len(controller.ws_clients)} total)")
        
        try:
            # Send initial status on connect
            await ws.send_json({
                'type': 'initial_status',
                'data': controller.get_status()
            })
            
            # Keep connection alive and stream telemetry
            while controller.running:
                try:
                    # Send periodic updates every second
                    await asyncio.sleep(1)
                    await ws.send_json({
                        'type': 'periodic_update',
                        'data': {
                            'telemetry': controller.latest_telemetry,
                            'state': controller.latest_state,
                            'threat_active': controller.last_threat_status
                        }
                    })
                except WebSocketDisconnect:
                    break
                    
        except WebSocketDisconnect:
            pass
        finally:
            if ws in controller.ws_clients:
                controller.ws_clients.remove(ws)
            controller.logger.info(f"📡 WebSocket client disconnected ({len(controller.ws_clients)} total)")
    
    return app


# ============== Main Entry Point ==============
if __name__ == '__main__':
    # Load configuration
    config = load_config()
    
    # Setup logging
    logger = setup_logging(config)
    
    # Create controller
    controller = AIController(config)
    
    # Check if API should be enabled
    api_config = config.get('api', {})
    if api_config.get('enabled', False):
        # Start controller in background thread
        controller_thread = threading.Thread(target=controller.run, name='MainController')
        controller_thread.start()
        
        # Create and run FastAPI app
        app = create_app(controller)
        
        host = api_config.get('host', '0.0.0.0')
        port = api_config.get('port', 5000)
        
        logger.info(f"📖 API Docs available at: http://{host}:{port}/docs")
        logger.info(f"📡 WebSocket available at: ws://{host}:{port}/ws/live")
        
        try:
            uvicorn.run(app, host=host, port=port, log_level="info")
        except KeyboardInterrupt:
            logger.info("⛔ Shutting down...")
        finally:
            controller.shutdown()
    else:
        # Run standalone without API
        controller.run()
