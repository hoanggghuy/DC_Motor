#include <Arduino.h>
#include <ESP32Encoder.h>

// ===== CẤU HÌNH CHÂN (Theo code test chạy OK) =====
// Motor A (Trái)
#define AIN1 27
#define AIN2 26
#define PWMA_PIN 25

// Motor B (Phải)
#define BIN1 19
#define BIN2 18
#define PWMB_PIN 16

// Standby
#define STBY 21
#define LED_PIN 2

// ===== CẤU HÌNH PWM (ESP32 LEDC) =====
#define PWM_FREQ 20000      // 20 kHz (Chạy êm, không kêu e e)
#define PWM_RES 8           // 8-bit resolution (0-255)
#define PWM_CHANNEL_A 0     // Kênh 0 cho Motor A
#define PWM_CHANNEL_B 1     // Kênh 1 cho Motor B

// ===== CẤU HÌNH ENCODER =====
#define ENC1_A 23 
#define ENC1_B 22
#define ENC2_A 34 
#define ENC2_B 35

ESP32Encoder encoderLeft;
ESP32Encoder encoderRight;

unsigned long lastCmdTime = 0;
unsigned long lastLogTime = 0;

// Hàm điều khiển Motor dùng LEDC
// Lưu ý: Tham số thứ 3 là CHANNEL chứ không phải PIN
void setMotor(int pin1, int pin2, int pwmChannel, float speed) {
  // 1. Giới hạn tốc độ (-1.0 đến 1.0)
  if (speed > 1.0) speed = 1.0;
  if (speed < -1.0) speed = -1.0;

  // 2. Tính Duty Cycle (0-255)
  int duty = (int)(abs(speed) * 255.0);

  // Deadzone: Nhỏ quá thì cắt luôn
  if (duty < 5) {
    duty = 0;
    speed = 0;
  }

  // 3. Điều khiển hướng và tốc độ
  if (speed > 0) {
    digitalWrite(pin1, HIGH); digitalWrite(pin2, LOW);
  } else if (speed < 0) {
    digitalWrite(pin1, LOW); digitalWrite(pin2, HIGH);
  } else {
    // Phanh
    digitalWrite(pin1, LOW); digitalWrite(pin2, LOW);
    duty = 0;
  }

  // QUAN TRỌNG: Ghi vào kênh PWM
  ledcWrite(pwmChannel, duty);
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(10);

  // 1. Setup chân hướng
  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
  pinMode(STBY, OUTPUT); pinMode(LED_PIN, OUTPUT);
  
  digitalWrite(STBY, HIGH); // Bật Driver

  // 2. Setup PWM (LEDC) - KHÁC BIỆT Ở ĐÂY
  // Cấu hình kênh
  ledcSetup(PWM_CHANNEL_A, PWM_FREQ, PWM_RES);
  ledcSetup(PWM_CHANNEL_B, PWM_FREQ, PWM_RES);
  
  // Gắn chân vào kênh
  ledcAttachPin(PWMA_PIN, PWM_CHANNEL_A);
  ledcAttachPin(PWMB_PIN, PWM_CHANNEL_B);

  // 3. Setup Encoder
  encoderLeft.attachHalfQuad(ENC1_A, ENC1_B);
  encoderRight.attachHalfQuad(ENC2_A, ENC2_B);
  encoderLeft.clearCount();
  encoderRight.clearCount();

  // Nháy đèn báo hiệu OK
  for(int i=0; i<3; i++) { digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW); delay(100);}
}

void loop() {
  // --- 1. NHẬN LỆNH TỪ MÁY TÍNH (Format: "v <left> <right>") ---
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    data.trim();

    if (data.startsWith("v")) {
      float vLeft = 0.0;
      float vRight = 0.0;
      
      // Tách chuỗi
      int firstSpace = data.indexOf(' ');
      if (firstSpace != -1) {
        int secondSpace = data.indexOf(' ', firstSpace + 1);
        if (secondSpace != -1) {
           vLeft = data.substring(firstSpace + 1, secondSpace).toFloat();
           vRight = data.substring(secondSpace + 1).toFloat();
        } else {
           vLeft = data.substring(firstSpace + 1).toFloat();
        }
      }

      // Điều khiển Motor (Truyền PWM Channel vào)
      setMotor(AIN1, AIN2, PWM_CHANNEL_A, vLeft);
      setMotor(BIN1, BIN2, PWM_CHANNEL_B, vRight);
      
      lastCmdTime = millis();
      digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Debug LED
    }
  }

  // --- 2. GỬI ENCODER LÊN MÁY TÍNH (20Hz) ---
  if (millis() - lastLogTime > 50) {
    long countLeft = encoderLeft.getCount();
    long countRight = encoderRight.getCount();

    Serial.print("e ");
    Serial.print(countLeft);
    Serial.print(" ");
    Serial.println(countRight);

    lastLogTime = millis();
  }

  // --- 3. AN TOÀN (Watchdog) ---
  if (millis() - lastCmdTime > 1000) {
    // Dừng xe nếu mất kết nối
    ledcWrite(PWM_CHANNEL_A, 0);
    ledcWrite(PWM_CHANNEL_B, 0);
    digitalWrite(AIN1, LOW); digitalWrite(AIN2, LOW);
    digitalWrite(BIN1, LOW); digitalWrite(BIN2, LOW);
  }
}