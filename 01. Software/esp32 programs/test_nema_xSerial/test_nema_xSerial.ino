#include <AccelStepper.h>

// Pines motor 1
#define STEP_PIN_M1 18
#define DIR_PIN_M1 19

// Pines motor 2
#define STEP_PIN_M2 21
#define DIR_PIN_M2 22

// Motores
AccelStepper motor1(AccelStepper::DRIVER, STEP_PIN_M1, DIR_PIN_M1);
AccelStepper motor2(AccelStepper::DRIVER, STEP_PIN_M2, DIR_PIN_M2);

// Buffer de lectura serial
String inputString = "";
bool newInput = false;

void setup() {
  Serial.begin(115200);
  Serial.println("Listo para recibir pasos por Serial...");

  motor1.setMaxSpeed(1000);
  motor1.setAcceleration(300);

  motor2.setMaxSpeed(1000);
  motor2.setAcceleration(300);
}

void loop() {
  // Ejecutar movimiento si está en curso
  motor1.run();
  motor2.run();

  // Leer del serial
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      newInput = true;
    } else {
      inputString += c;
    }
  }

  // Procesar el valor ingresado
  if (newInput) {
    long pasos = inputString.toInt();

    Serial.print("Moviendo ambos motores ");
    Serial.print(pasos);
    Serial.println(" pasos...");

    motor1.move(pasos);
    motor2.move(-pasos);

    inputString = "";
    newInput = false;
  }
}
