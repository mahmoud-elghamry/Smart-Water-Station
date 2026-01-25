"""
AI Safety System for SmarterWater
================================
Face Detection based safety system using OpenCV.

This script:
1. Detects faces using camera (human presence)
2. If human detected near water station -> Sends PUMP_OFF for safety
3. If area is clear -> Allows pump to run (PUMP_ON)

Works with:
- Real ESP32 via Serial (USB)
- Wokwi Simulation via MQTT (broker.hivemq.com)
"""

import time
import cv2
import sys

# إعدادات
FACE_CASCADE_FILE = "haarcascade_frontalface_default.xml"

# MQTT إعدادات
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
MQTT_TOPIC_CONTROL = "grad/project/control"
MQTT_TOPIC_SENSORS = "grad/project/sensors"

# متغيرات عامة
mqtt_client = None
use_mqtt = False
use_serial = False
ser = None


def setup_mqtt():
    """إعداد الاتصال بـ MQTT للتحكم في المحاكاة"""
    global mqtt_client, use_mqtt
    
    try:
        import paho.mqtt.client as mqtt
        
        def on_connect(client, userdata, flags, rc):
            if rc == 0:
                print("✅ Connected to MQTT Broker!")
                client.subscribe(MQTT_TOPIC_SENSORS)
            else:
                print(f"❌ MQTT Connection failed with code {rc}")
        
        def on_message(client, userdata, msg):
            print(f"📩 Received: {msg.topic} -> {msg.payload.decode()}")
        
        mqtt_client = mqtt.Client(client_id=f"AI_Safety_{int(time.time())}")
        mqtt_client.on_connect = on_connect
        mqtt_client.on_message = on_message
        
        print(f"🔌 Connecting to MQTT broker: {MQTT_BROKER}...")
        mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
        mqtt_client.loop_start()
        
        time.sleep(2)  # انتظار الاتصال
        use_mqtt = True
        return True
        
    except ImportError:
        print("⚠️ paho-mqtt not installed. Run: pip install paho-mqtt")
        return False
    except Exception as e:
        print(f"❌ MQTT Error: {e}")
        return False


def setup_serial():
    """إعداد الاتصال بـ ESP32 الحقيقي عبر Serial"""
    global ser, use_serial
    
    try:
        import serial
        import serial.tools.list_ports
        
        ports = serial.tools.list_ports.comports()
        print("🔍 Searching for ESP32...")
        
        for port in ports:
            if any(chip in port.description for chip in ["CP210x", "CH340", "CH9102", "Serial"]):
                print(f"✅ Found ESP32 at: {port.device}")
                ser = serial.Serial(port.device, 115200, timeout=1)
                time.sleep(2)
                use_serial = True
                return True
        
        print("⚠️ ESP32 not found on any COM port")
        return False
        
    except ImportError:
        print("⚠️ pyserial not installed. Run: pip install pyserial")
        return False
    except Exception as e:
        print(f"❌ Serial Error: {e}")
        return False


def send_pump_command(command):
    """إرسال أمر للتحكم في البامب"""
    global mqtt_client, ser, use_mqtt, use_serial
    
    if use_mqtt and mqtt_client:
        try:
            mqtt_client.publish(MQTT_TOPIC_CONTROL, command)
            print(f"📤 MQTT Published: {command}")
        except Exception as e:
            print(f"❌ MQTT Publish Error: {e}")
    
    if use_serial and ser and ser.is_open:
        try:
            # تحويل الأمر للصيغة القديمة للـ Serial
            serial_cmd = command.replace("_", ":")
            ser.write(f"{serial_cmd}\n".encode())
            print(f"📤 Serial Sent: {serial_cmd}")
        except Exception as e:
            print(f"❌ Serial Send Error: {e}")
    
    if not use_mqtt and not use_serial:
        print(f"🔇 [SIMULATION] Command: {command}")


def main():
    """البرنامج الرئيسي"""
    print("=" * 50)
    print("  🤖 AI SAFETY SYSTEM - SmarterWater")
    print("=" * 50)
    print()
    
    # محاولة الاتصال بـ MQTT (للمحاكاة)
    mqtt_ok = setup_mqtt()
    
    # محاولة الاتصال بـ Serial (للـ ESP32 الحقيقي)
    serial_ok = setup_serial()
    
    if not mqtt_ok and not serial_ok:
        print()
        print("⚠️ Running in DISPLAY-ONLY mode (no ESP32 connection)")
        print()
    
    # تحميل ملف التعرف على الوجوه
    face_cascade = cv2.CascadeClassifier(FACE_CASCADE_FILE)
    if face_cascade.empty():
        face_cascade = cv2.CascadeClassifier(cv2.data.haarcascades + FACE_CASCADE_FILE)
    
    if face_cascade.empty():
        print("❌ Error: Could not load face cascade xml file.")
        print("   Make sure 'haarcascade_frontalface_default.xml' exists")
        return
    
    # فتح الكاميرا
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("❌ Error: Could not open camera")
        return
    
    print()
    print("📷 Camera opened successfully")
    print("🛡️  Safety Logic:")
    print("   👤 FACE DETECTED -> PUMP_OFF (Safety Stop)")
    print("   ✅ AREA CLEAR    -> PUMP_ON  (Normal Operation)")
    print()
    print("Press 'q' to quit")
    print("-" * 50)
    
    current_pump_state = None
    
    try:
        while True:
            ret, frame = cap.read()
            if not ret:
                break
            
            # تحويل للـ Grayscale للتعرف على الوجوه
            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            faces = face_cascade.detectMultiScale(gray, 1.1, 4)
            
            if len(faces) > 0:
                # ⚠️ فيه إنسان -> إيقاف البامب للسلامة
                if current_pump_state != "OFF":
                    print(f"⚠️ HUMAN DETECTED! Stopping pump for safety...")
                    send_pump_command("PUMP_OFF")
                    current_pump_state = "OFF"
                
                # رسم مربع أحمر حول الوجه
                for (x, y, w, h) in faces:
                    cv2.rectangle(frame, (x, y), (x+w, y+h), (0, 0, 255), 3)
                    cv2.putText(frame, "SAFETY STOP!", (x, y-10), 
                               cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 2)
                
                # شريط تحذير أحمر
                cv2.rectangle(frame, (0, 0), (frame.shape[1], 40), (0, 0, 200), -1)
                cv2.putText(frame, "!! HUMAN DETECTED - PUMP STOPPED !!", (10, 28),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
            
            else:
                # ✅ المنطقة آمنة -> تشغيل البامب
                if current_pump_state != "ON":
                    print(f"✅ Area clear. Pump can operate normally.")
                    send_pump_command("PUMP_ON")
                    current_pump_state = "ON"
                
                # شريط أخضر للأمان
                cv2.rectangle(frame, (0, 0), (frame.shape[1], 40), (0, 150, 0), -1)
                cv2.putText(frame, "AREA CLEAR - SYSTEM RUNNING", (10, 28),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
            
            # عرض حالة الاتصال
            status_y = frame.shape[0] - 10
            status_text = f"MQTT: {'OK' if use_mqtt else 'NO'} | Serial: {'OK' if use_serial else 'NO'}"
            cv2.putText(frame, status_text, (10, status_y),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (200, 200, 200), 1)
            
            cv2.imshow('SmarterWater AI Safety System', frame)
            
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
    
    finally:
        # إيقاف البامب للأمان عند الخروج
        print()
        print("🛑 Shutting down...")
        send_pump_command("PUMP_OFF")
        
        if use_serial and ser and ser.is_open:
            ser.close()
        
        if use_mqtt and mqtt_client:
            mqtt_client.loop_stop()
            mqtt_client.disconnect()
        
        cap.release()
        cv2.destroyAllWindows()
        print("✅ System stopped safely.")


if __name__ == "__main__":
    main()