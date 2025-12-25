#include <Arduino.h>
#include <ESP32Encoder.h>
const bool INVERT_A = false; // Motor Trái
const bool INVERT_B = true;  // Motor Phải

// Motor A (Trái)
#define AIN1 27u
#define AIN2 26
#define PWMA_PIN 25

// Motor B (Phải)
#define BIN1 19
#define BIN2 18
#define PWMB_PIN 5

#define STBY 21
#define LED_PIN 2


#define PWM_FREQ 20000
#define PWM_RES 8
#define PWM_CHANNEL_A 0
#define PWM_CHANNEL_B 1

#define ENC1_A 23 
#define ENC1_B 22
#define ENC2_A 17 
#define ENC2_B 16

ESP32Encoder encoderLeft;
ESP32Encoder encoderRight;

unsigned long lastCmdTime = 0;
unsigned long lastLogTime = 0;

void setMotor(int pin1, int pin2, int pwmChannel, float speed, bool invert) {

  if (invert) {
    speed = -speed;
  }

  if (speed > 1.0) speed = 1.0;
  if (speed < -1.0) speed = -1.0;

  int duty = (int)(abs(speed) * 255.0);

  if (duty < 5) {
    duty = 0;
    speed = 0;
  }

  if (speed > 0) {
    digitalWrite(pin1, HIGH); digitalWrite(pin2, LOW);
  } else if (speed < 0) {
    digitalWrite(pin1, LOW); digitalWrite(pin2, HIGH);
  } else {
    digitalWrite(pin1, LOW); digitalWrite(pin2, LOW);
    duty = 0;
  }

  ledcWrite(pwmChannel, duty);
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(10);

  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
  pinMode(STBY, OUTPUT); pinMode(LED_PIN, OUTPUT);
  
  digitalWrite(STBY, HIGH);

  ledcSetup(PWM_CHANNEL_A, PWM_FREQ, PWM_RES);
  ledcSetup(PWM_CHANNEL_B, PWM_FREQ, PWM_RES);
  ledcAttachPin(PWMA_PIN, PWM_CHANNEL_A);
  ledcAttachPin(PWMB_PIN, PWM_CHANNEL_B);
  encoderLeft.attachHalfQuad(ENC1_A, ENC1_B);
  encoderRight.attachHalfQuad(ENC2_A, ENC2_B);
  encoderLeft.clearCount();
  encoderRight.clearCount();

  for(int i=0; i<3; i++) { digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW); delay(100);}
}

void loop() {

  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    data.trim();

    if (data.startsWith("v")) {
      float vLeft = 0.0;
      float vRight = 0.0;
      
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

      setMotor(AIN1, AIN2, PWM_CHANNEL_A, vLeft, INVERT_A);
      setMotor(BIN1, BIN2, PWM_CHANNEL_B, vRight, INVERT_B);
      
      lastCmdTime = millis();
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
  if (millis() - lastCmdTime > 2000) {
    ledcWrite(PWM_CHANNEL_A, 0);
    ledcWrite(PWM_CHANNEL_B, 0);
    digitalWrite(AIN1, LOW); digitalWrite(AIN2, LOW);
    digitalWrite(BIN1, LOW); digitalWrite(BIN2, LOW);
  }
}