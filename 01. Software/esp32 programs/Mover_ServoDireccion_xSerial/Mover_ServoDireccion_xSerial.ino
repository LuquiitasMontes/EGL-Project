#include <ServoEasing.hpp>

#define SERVO_PIN 23  // Cambialo si querés

// 65 MAX

ServoEasing miServo;

void setup() {
  Serial.begin(115200);
  Serial.println("ServoEasing listo para MG996R sin PCA");

  miServo.attach(SERVO_PIN);
  miServo.setEasingType(EASE_LINEAR);  // Podés probar otros como EASE_CUBIC_IN_OUT
  miServo.setSpeed(30);  // Grados por segundo

  miServo.write(30);  // Posición inicial rápida
  delay(1000);
  
}

void loop() {
  // Obligatorio para que avance el easing
  miServo.update();

  // Ejemplo: mover a 180° con easing al recibir comando
  if (Serial.available()) {
    String s = Serial.readStringUntil('\n');
    int angulo = s.toInt();
    angulo = constrain(angulo, 0, 180);
    Serial.print("Moviendo suavemente a ");
    Serial.println(angulo);
    miServo.startEaseTo(angulo);
  }
}
