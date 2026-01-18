import serial
import time
import cv2
import serial.tools.list_ports

# إعدادات الاتصال
BAUD_RATE = 115200
FACE_CASCADE_FILE = "haarcascade_frontalface_default.xml"

def find_esp32_port():
    ports = serial.tools.list_ports.comports()
    print("Searching for ESP32...")
    for port in ports:
        # البحث عن التعريفات المشهورة للـ ESP32
        if "CP210x" in port.description or "CH340" in port.description or "Serial" in port.description or "CH9102" in port.description:
            print(f"Found ESP32 at: {port.device}")
            return port.device
    return None

def main():
    # 1. الاتصال بالـ ESP32
    esp32_port = find_esp32_port()
    controller_connected = False
    ser = None
    if esp32_port is None:
        print("Warning: ESP32 not found! Running in camera test mode.")
    else:
        try:
            ser = serial.Serial(esp32_port, BAUD_RATE, timeout=1)
            time.sleep(2) # انتظار استقرار الاتصال
            controller_connected = True
            print("ESP32 connected successfully.")
        except Exception as e:
            print(f"Connection Error: {e}. Running in camera test mode.")

    # 2. تحميل ملف التعرف على الوجوه
    # تأكد إن ملف xml جنب ملف البايثون ده
    face_cascade = cv2.CascadeClassifier(FACE_CASCADE_FILE)
    if face_cascade.empty():
        # لو ملقاش الملف جنبه، هيحاول يجيبه من ملفات المكتبة الأصلية
        face_cascade = cv2.CascadeClassifier(cv2.data.haarcascades + FACE_CASCADE_FILE)
    431
    if face_cascade.empty():
        print("Error: Could not load face cascade xml file.")
        return

    cap = cv2.VideoCapture(0)
    
    # متغير لتخزين حالة المضخة عشان ميبعتش الأمر مليون مرة
    # بنفترض في البداية إننا مش عارفين الحالة
    current_pump_state = None 

    print("--- AI SAFETY SYSTEM RUNNING ---")
    if controller_connected:
        print("Controller connected: NO FACE -> PUMP ON | FACE DETECTED -> PUMP OFF")
    else:
        print("Camera test mode: NO FACE -> SIMULATE PUMP ON | FACE DETECTED -> SIMULATE PUMP OFF")

    try:
        while True:
            ret, frame = cap.read()
            if not ret: break
            
            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            faces = face_cascade.detectMultiScale(gray, 1.1, 4)
            
            # === المنطق الجديد ===
            
            if len(faces) > 0:
                # الحالة: فيه إنسان (خطر) -> لازم نقفل
                if current_pump_state != "OFF":
                    if controller_connected and ser:
                        print(f"⚠️ HUMAN DETECTED! Sending PUMP:OFF") 
                        ser.write(b"PUMP:OFF\n") 
                    else:
                        print(f"⚠️ HUMAN DETECTED! (Simulated PUMP:OFF)")
                    current_pump_state = "OFF"
                
                # رسم مربع أحمر على الوش
                for (x, y, w, h) in faces:
                    cv2.rectangle(frame, (x, y), (x+w, y+h), (0, 0, 255), 2)
                    cv2.putText(frame, "SAFETY STOP", (x, y-10), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0,0,255), 2)
            
            else:
                # الحالة: مفيش حد (أمان) -> نشغل المضخة
                if current_pump_state != "ON":
                    if controller_connected and ser:
                        print(f"✅ AREA CLEAR. Sending PUMP:ON")
                        ser.write(b"PUMP:ON\n")
                    else:
                        print(f"✅ AREA CLEAR. (Simulated PUMP:ON)")
                    current_pump_state = "ON"
                
                # نكتب على الشاشة إن الدنيا أمان
                cv2.putText(frame, "RUNNING...", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0,255,0), 2)

            cv2.imshow('Water Station AI Guard', frame)

            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

    finally:
        # عند الخروج، نبعت أمر قفل للأمان
        if controller_connected and ser and ser.is_open:
            ser.write(b"PUMP:OFF\n")
            ser.close()
        cap.release()
        cv2.destroyAllWindows()
        print("System Stopped.")

if __name__ == "__main__":
    main()