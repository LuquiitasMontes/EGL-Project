#include <AccelStepper.h>

// Pines motor 1
#define STEP_PIN_M1 18
#define DIR_PIN_M1 19

// Pines motor 2
#define STEP_PIN_M2 21
#define DIR_PIN_M2 22

// Creamos los motores
AccelStepper motor1(AccelStepper::DRIVER, STEP_PIN_M1, DIR_PIN_M1);
AccelStepper motor2(AccelStepper::DRIVER, STEP_PIN_M2, DIR_PIN_M2);

// Variables para alternar dirección
long destino = 1000;

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando prueba de motores...");

  // Configuración motores
  motor1.setMaxSpeed(1000);
  motor1.setAcceleration(300);

  motor2.setMaxSpeed(1000);
  motor2.setAcceleration(300);

  // Movimiento inicial
  motor1.moveTo(destino);
  motor2.moveTo(-destino);
}

void loop() {
  motor1.run();
  motor2.run();

  // Si ambos motores llegaron a destino
  if (!motor1.isRunning() && !motor2.isRunning()) {
    destino = -destino;

    motor1.moveTo(motor1.currentPosition() + destino);
    motor2.moveTo(motor2.currentPosition() + destino);

    delay(500);  // Pequeña pausa entre cambios
  }
}
