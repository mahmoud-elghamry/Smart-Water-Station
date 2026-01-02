#include <Arduino.h>

// تعريف الأرجل
#define PUMP_PIN 2

// متغيرات التوقيت (عشان نبعت البيانات كل ثانية بدون تعطيل الجهاز)
unsigned long previousMillis = 0;
const long interval = 1000;

// متغيرات لتخزين القيم الحالية (عشان المحاكاة تكون ناعمة وواقعية)
float currentTDS = 450.0;
float currentPressure = 1.5;
float currentFlow = 10.0;
int flowProgress = 50;
bool pumpState = false;

void setup()
{
  // الاتصال بنفس سرعة الموقع
  Serial.begin(115200);
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW); // التأكد إن المضخة مطفية في البداية
}

void loop()
{
  // ================================================================
  // 1. استقبال الأوامر من الموقع (بدون أي طباعة للديباج)
  // ==============I==================================================
  if (Serial.available() > 0)
  {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command == "PUMP:ON")
    {
      pumpState = true;
      digitalWrite(PUMP_PIN, HIGH);
    }
    else if (command == "PUMP:OFF")
    {
      pumpState = false;
      digitalWrite(PUMP_PIN, LOW);
    }
    // لو حبيت تضيف أوامر تانية (زي وضع AI) ممكن تضيفها هنا
  }

  // ================================================================
  // 2. منطق الحماية الذكي (Edge Computing Feature)
  // ================================================================
  // لو الضغط زاد عن حد الأمان (2.8 بار) والمضخة شغالة -> افصلها فوراً
  if (currentPressure > 2.8 && pumpState == true)
  {
    pumpState = false;
    digitalWrite(PUMP_PIN, LOW);
    // ملحوظة: الموقع هيحدث حالة الزرار لما نبعتله الحالة الجديدة تحت
  }

  // ================================================================
  // 3. تحديث القراءات وإرسالها (كل ثانية)
  // ================================================================
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;

    // --- (أ) محاكاة واقعية (تغيير تدريجي للقراءات) ---

    // TDS: تغيير أسرع شوية (+/- 5) عشان يبان التغيير أكتر
    currentTDS += random(-50, 50) / 10.0;
    currentTDS = constrain(currentTDS, 300, 600); // إجبار القيمة تفضل في نطاق منطقي

    // Pressure: لو المضخة شغالة الضغط بيزيد، لو مطفية بيقل (زيادة السرعة شوية)
    if (pumpState)
    {
      currentPressure += random(10, 30) / 100.0; // زيادة أسرع شوية
    }
    else
    {
      currentPressure -= random(10, 30) / 100.0; // نقصان أسرع شوية
    }
    currentPressure = constrain(currentPressure, 0.5, 3.0); // حدود الضغط

    // Flow Rate: مرتبط بالمضخة (لو شغالة فيه تدفق، لو لا مفيش)
    if (pumpState)
    {
      currentFlow = 9.0 + (random(-10, 10) / 10.0); // تدفق متغير بسيط حول الـ 9
      flowProgress += 5;                            // التقدم بيزيد
    }
    else
    {
      currentFlow = 0.0; // مفيش تدفق
      // flowProgress مش هنصفره عشان يبان إن العملية وقفت مكانها
    }
    if (flowProgress > 100)
      flowProgress = 0; // إعادة الدورة

    // --- (ب) تجهيز رسالة الـ JSON ---
    // الرسالة نضيفة جداً ومفهومة
    String jsonMessage = "{\"tds\":";
    jsonMessage += String(currentTDS, 1);
    jsonMessage += ",\"pressure\":";
    jsonMessage += String(currentPressure, 2);
    jsonMessage += ",\"flowRate\":";
    jsonMessage += String(currentFlow, 1);
    jsonMessage += ",\"flowProgress\":";
    jsonMessage += String(flowProgress);

    // أهم جزء: بنبعت حالة المضخة الحقيقية (عشان لو فصلت أوتوماتيك الموقع يعرف)
    jsonMessage += ",\"pump\":";
    jsonMessage += pumpState ? "1" : "0";

    jsonMessage += "}";

    // إرسال للموقع
    Serial.println(jsonMessage);
  }
}