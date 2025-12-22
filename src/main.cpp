#include <Arduino.h>
#include <ESP32Encoder.h>

// --- CẤU HÌNH CHÂN MOTOR ---
#define AIN1 27
#define AIN2 26
#define PWMA 25

#define BIN1 19
#define BIN2 18
#define PWMB 5

#define STBY 21
#define LED_PIN 2

// --- CẤU HÌNH CHÂN ENCODER ---
#define ENC1_A 23 
#define ENC1_B 22
#define ENC2_A 34 
#define ENC2_B 35
ESP32Encoder encoderLeft;
ESP32Encoder encoderRight;

unsigned long lastCmdTime = 0;
unsigned long lastLogTime = 0;

void setMotor(int pin1, int pin2, int pinPWM, float speed) {
  if (speed > 1.0) speed = 1.0;
  if (speed < -1.0) speed = -1.0;
  int pwm = (int)(abs(speed) * 255.0);
  
  if (speed > 0) {
    digitalWrite(pin1, HIGH); digitalWrite(pin2, LOW);
  } else if (speed < 0) {
    digitalWrite(pin1, LOW); digitalWrite(pin2, HIGH);
  } else {
    digitalWrite(pin1, LOW); digitalWrite(pin2, LOW);
    pwm = 0;
  }
  analogWrite(pinPWM, pwm);
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(10);

  // Setup Motor
  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT); pinMode(PWMA, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT); pinMode(PWMB, OUTPUT);
  pinMode(STBY, OUTPUT); pinMode(LED_PIN, OUTPUT);
  digitalWrite(STBY, HIGH);

  // Gắn chân
  encoderLeft.attachHalfQuad(ENC1_A, ENC1_B);
  encoderRight.attachHalfQuad(ENC2_A, ENC2_B);
  
  // Reset về 0
  encoderLeft.clearCount();
  encoderRight.clearCount();

  // Nháy đèn báo sẵn sàng
  for(int i=0; i<3; i++) { digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW); delay(100);}
}

void loop() {
  // 1. NHẬN LỆNH TỪ MÁY TÍNH (v 0.5 0.5)
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    data.trim();

    if (data.startsWith("v")) {
      float vLeft = 0.0;
      float vRight = 0.0;
      
      // Tách chuỗi thủ công để lấy 2 số tốc độ
      int firstSpace = data.indexOf(' ');
      if (firstSpace != -1) {
        int secondSpace = data.indexOf(' ', firstSpace + 1);
        if (secondSpace != -1) {
           vLeft = data.substring(firstSpace + 1, secondSpace).toFloat();
           vRight = data.substring(secondSpace + 1).toFloat();
        } else {
           // Fallback nếu chuỗi lỗi
           vLeft = data.substring(firstSpace + 1).toFloat();
        }
      }

      setMotor(AIN1, AIN2, PWMA, vLeft);
      setMotor(BIN1, BIN2, PWMB, vRight);
      lastCmdTime = millis();
      
      // Nháy đèn khi nhận lệnh điều khiển
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
  }
  if (millis() - lastLogTime > 50) {
    long countLeft = encoderLeft.getCount();
    long countRight = encoderRight.getCount();

    Serial.print("e ");
    Serial.print(countLeft);
    Serial.print(" ");
    Serial.println(countRight);

    lastLogTime = millis();
  }

  // 3. AN TOÀN: Dừng xe nếu mất kết nối quá 1s
  if (millis() - lastCmdTime > 1000) {
    setMotor(AIN1, AIN2, PWMA, 0);
    setMotor(BIN1, BIN2, PWMB, 0);
  }
}