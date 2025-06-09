#include <Arduino.h>

#define USE_PCA9685_SERVO_EXPANDER  // Activating this enables the use of the PCA9685 I2C expander chip/board.
#define USE_SERVO_LIB               // If USE_PCA9685_SERVO_EXPANDER is defined, Activating this enables force additional using of regular servo library.
//#define MAX_EASING_SERVOS 16
#include "ServoEasing.hpp"
#include "PinDefinitionsAndMore.h"

ServoEasing Servo1AtPCA9685(PCA9685_DEFAULT_ADDRESS);  // Hombro 1
ServoEasing Servo2AtPCA9685(PCA9685_DEFAULT_ADDRESS);  // Hombro 2
ServoEasing Servo3AtPCA9685(PCA9685_DEFAULT_ADDRESS);  // Codo
ServoEasing Servo4AtPCA9685(PCA9685_DEFAULT_ADDRESS);  // Muñeca
ServoEasing Servo5AtPCA9685(PCA9685_DEFAULT_ADDRESS);  // Gripper

const int startDegreeServo1 = 90;  // Hombro 1 0º - 115º
const int startDegreeServo2 = 90;  // Hombro 2 0º - 180º
const int startDegreeServo3 = 90;  // Codo     0º - 125º
const int startDegreeServo4 = 0;   // Muñeca   0º - 180º
const int startDegreeServo5 = 35;  // Gripper 35º Cerrrado - 115º Abierto

// VELOCIDAD Servos
int vel_servos = 50;

// Arreglo de punteros a los servos así:
ServoEasing* servos[] = { &Servo1AtPCA9685, &Servo2AtPCA9685, &Servo3AtPCA9685, &Servo4AtPCA9685, &Servo5AtPCA9685 };
const int NUM_SERVOS = 5;

void setup() {
  Serial.begin(115200);

  if (Servo1AtPCA9685.InitializeAndCheckI2CConnection(&Serial)) {
    while (true) {
    }
  } else {
    // Seteo tipo de movimiento y velocidad de los servos
    Servo1AtPCA9685.setEasingType(EASE_CUBIC_IN_OUT);
    Servo1AtPCA9685.setSpeed(vel_servos);
    Servo2AtPCA9685.setEasingType(EASE_CUBIC_IN_OUT);
    Servo2AtPCA9685.setSpeed(vel_servos);
    Servo3AtPCA9685.setEasingType(EASE_CUBIC_IN_OUT);
    Servo3AtPCA9685.setSpeed(vel_servos);
    Servo4AtPCA9685.setEasingType(EASE_CUBIC_IN_OUT);
    Servo4AtPCA9685.setSpeed(vel_servos);
    Servo5AtPCA9685.setEasingType(EASE_CUBIC_IN_OUT);
    Servo5AtPCA9685.setSpeed(vel_servos);

    Serial.println(F("Attach servo to port 0 of PCA9685 expander"));
    Servo1AtPCA9685.attach(0, startDegreeServo1);
    Servo2AtPCA9685.attach(1, startDegreeServo2);
    Servo3AtPCA9685.attach(2, startDegreeServo3);
    Servo4AtPCA9685.attach(3, startDegreeServo4);
    Servo5AtPCA9685.attach(4, startDegreeServo5);
  }
  // Wait for servos to reach start position.
  delay(500);
}

void loop() {
  moveServoFromSerial();
}

void moveServoFromSerial() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.length() < 3) return;  // Comando muy corto, ignorar

    // Verificar que el primer carácter sea 'S' o 's'
    if (input.charAt(0) != 'S' && input.charAt(0) != 's') {
      Serial.println(F("Invalid command format. Use: S<num> <angle>"));
      return;
    }

    // Obtener número de servo (asumimos un solo dígito entre '1' y '4')
    char servoChar = input.charAt(1);
    if (servoChar < '1' || servoChar > '5') {
      Serial.println(F("Servo number out of range (1-5)"));
      return;
    }
    int servoIndex = servoChar - '1';

    // Buscar el espacio que separa el servo del ángulo
    int spaceIndex = input.indexOf(' ');
    if (spaceIndex == -1) {
      Serial.println(F("Invalid command format. Missing angle."));
      return;
    }

    // Extraer el ángulo en string y convertir a int
    String angleStr = input.substring(spaceIndex + 1);
    angleStr.trim();
    int angle = angleStr.toInt();

    if (angle < 0 || angle > 180) {
      Serial.println(F("Angle out of range (0-180)"));
      return;
    }

    // Mover el servo correspondiente
    Serial.print(F("Moving servo "));
    Serial.print(servoIndex + 1);
    Serial.print(F(" to "));
    Serial.print(angle);
    Serial.println(F(" degrees"));

    servos[servoIndex]->startEaseTo(angle);
  }
}
