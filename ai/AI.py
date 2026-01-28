"""
AI Safety System - Face Detection
Face detection based safety system for water station.

Detects human presence via camera and controls pump accordingly:
- Human detected -> PUMP_OFF (safety stop)
- Area clear -> PUMP_ON (normal operation)

Supports:
- Real ESP32 via Serial (USB connection)
- Wokwi Simulation via MQTT (broker.hivemq.com)
"""

import time
import cv2
import sys

# Configuration
FACE_CASCADE_FILE = "haarcascade_frontalface_default.xml"

# MQTT Settings
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
MQTT_TOPIC_CONTROL = "grad/project/control"
MQTT_TOPIC_SENSORS = "grad/project/sensors"

# Global variables
mqtt_client = None
use_mqtt = False
use_serial = False
ser = None


def setup_mqtt():
    """Setup MQTT connection for simulation control"""
    global mqtt_client, use_mqtt
    
    try:
        import paho.mqtt.client as mqtt
        
        def on_connect(client, userdata, flags, rc):
            if rc == 0:
                print("[OK] Connected to MQTT Broker")
                client.subscribe(MQTT_TOPIC_SENSORS)
            else:
                print(f"[ERR] MQTT Connection failed: {rc}")
        
        def on_message(client, userdata, msg):
            pass  # Sensor data received - no need to print
        
        mqtt_client = mqtt.Client(client_id=f"AI_Safety_{int(time.time())}")
        mqtt_client.on_connect = on_connect
        mqtt_client.on_message = on_message
        
        print(f"[...] Connecting to MQTT: {MQTT_BROKER}")
        mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
        mqtt_client.loop_start()
        
        time.sleep(2)
        use_mqtt = True
        return True
        
    except ImportError:
        print("[WARN] paho-mqtt not installed. Run: pip install paho-mqtt")
        return False
    except Exception as e:
        print(f"[ERR] MQTT Error: {e}")
        return False


def setup_serial():
    """Setup Serial connection to real ESP32"""
    global ser, use_serial
    
    try:
        import serial
        import serial.tools.list_ports
        
        ports = serial.tools.list_ports.comports()
        print("[...] Searching for ESP32")
        
        for port in ports:
            if any(chip in port.description for chip in ["CP210x", "CH340", "CH9102", "Serial"]):
                print(f"[OK] Found ESP32 at: {port.device}")
                ser = serial.Serial(port.device, 115200, timeout=1)
                time.sleep(2)
                use_serial = True
                return True
        
        print("[WARN] ESP32 not found on any COM port")
        return False
        
    except ImportError:
        print("[WARN] pyserial not installed. Run: pip install pyserial")
        return False
    except Exception as e:
        print(f"[ERR] Serial Error: {e}")
        return False


def send_pump_command(command):
    """Send pump control command"""
    global mqtt_client, ser, use_mqtt, use_serial
    
    if use_mqtt and mqtt_client:
        try:
            mqtt_client.publish(MQTT_TOPIC_CONTROL, command)
            print(f"[TX] MQTT: {command}")
        except Exception as e:
            print(f"[ERR] MQTT Publish: {e}")
    
    if use_serial and ser and ser.is_open:
        try:
            serial_cmd = command.replace("_", ":")
            ser.write(f"{serial_cmd}\n".encode())
            print(f"[TX] Serial: {serial_cmd}")
        except Exception as e:
            print(f"[ERR] Serial Send: {e}")
    
    if not use_mqtt and not use_serial:
        print(f"[SIM] Command: {command}")


def main():
    """Main program"""
    print("=" * 40)
    print("  AI SAFETY SYSTEM")
    print("=" * 40)
    print()
    
    # Try MQTT (for simulation)
    mqtt_ok = setup_mqtt()
    
    # Try Serial (for real ESP32)
    serial_ok = setup_serial()
    
    if not mqtt_ok and not serial_ok:
        print()
        print("[WARN] Running in DISPLAY-ONLY mode (no ESP32 connection)")
        print()
    
    # Load face detection model
    face_cascade = cv2.CascadeClassifier(FACE_CASCADE_FILE)
    if face_cascade.empty():
        face_cascade = cv2.CascadeClassifier(cv2.data.haarcascades + FACE_CASCADE_FILE)
    
    if face_cascade.empty():
        print("[ERR] Could not load face cascade xml file")
        print("      Make sure 'haarcascade_frontalface_default.xml' exists")
        return
    
    # Open camera
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("[ERR] Could not open camera")
        return
    
    print()
    print("[OK] Camera opened")
    print("Safety Logic:")
    print("  FACE DETECTED -> PUMP_OFF (Safety Stop)")
    print("  AREA CLEAR    -> PUMP_ON  (Normal)")
    print()
    print("Press 'q' to quit")
    print("-" * 40)
    
    current_pump_state = None
    
    try:
        while True:
            ret, frame = cap.read()
            if not ret:
                break
            
            # Convert to grayscale for face detection
            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            faces = face_cascade.detectMultiScale(gray, 1.1, 4)
            
            if len(faces) > 0:
                # Human detected -> stop pump
                if current_pump_state != "OFF":
                    print("[!] HUMAN DETECTED - Stopping pump")
                    send_pump_command("PUMP_OFF")
                    current_pump_state = "OFF"
                
                # Draw red box
                for (x, y, w, h) in faces:
                    cv2.rectangle(frame, (x, y), (x+w, y+h), (0, 0, 255), 3)
                    cv2.putText(frame, "SAFETY STOP!", (x, y-10), 
                               cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 2)
                
                # Red warning bar
                cv2.rectangle(frame, (0, 0), (frame.shape[1], 40), (0, 0, 200), -1)
                cv2.putText(frame, "HUMAN DETECTED - PUMP STOPPED", (10, 28),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
            
            else:
                # Area clear -> allow pump
                if current_pump_state != "ON":
                    print("[OK] Area clear - Pump can operate")
                    send_pump_command("PUMP_ON")
                    current_pump_state = "ON"
                
                # Green bar
                cv2.rectangle(frame, (0, 0), (frame.shape[1], 40), (0, 150, 0), -1)
                cv2.putText(frame, "AREA CLEAR - SYSTEM RUNNING", (10, 28),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
            
            # Show connection status
            status_y = frame.shape[0] - 10
            status_text = f"MQTT: {'OK' if use_mqtt else 'NO'} | Serial: {'OK' if use_serial else 'NO'}"
            cv2.putText(frame, status_text, (10, status_y),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (200, 200, 200), 1)
            
            cv2.imshow('AI Safety System', frame)
            
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
    
    finally:
        # Stop pump on exit
        print()
        print("[...] Shutting down")
        send_pump_command("PUMP_OFF")
        
        if use_serial and ser and ser.is_open:
            ser.close()
        
        if use_mqtt and mqtt_client:
            mqtt_client.loop_stop()
            mqtt_client.disconnect()
        
        cap.release()
        cv2.destroyAllWindows()
        print("[OK] System stopped")


if __name__ == "__main__":
    main()