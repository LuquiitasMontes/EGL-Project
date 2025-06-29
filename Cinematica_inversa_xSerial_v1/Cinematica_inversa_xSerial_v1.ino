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

const int startDegreeServo1 = 90;  // Hombro 1  0º - 115º
const int startDegreeServo2 = 90;  // Hombro 2  10º - 180º
const int startDegreeServo3 = 90;  // Codo      0º - 125º
const int startDegreeServo4 = 0;   // Muñeca    0º - 180º
const int startDegreeServo5 = 35;  // Gripper   35º Cerrrado - 115º Abierto

// VELOCIDAD Servos
int vel_servos = 15;

// Largo de Eslabones en mm
int l1 = 70;
int l2 = 70;
int l3 = 157;
int l4 = 180;

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

    Serial.println(F("Attach servos to ports of PCA9685 expander"));
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
  leerPosicionDesdeSerial();  // Para leer una posición (x, y, z)
}

void leerPosicionDesdeSerial() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.charAt(0) == 'P' || input.charAt(0) == 'p') {
      float x, y, z;
      int scanned = sscanf(input.c_str(), "P %f %f %f", &x, &y, &z);
      if (scanned == 3) {
        calcularAngulosDesdePosicion(x, y, z);
      } else {
        Serial.println(F("Formato inválido. Usa: P <x> <y> <z>"));
      }
    }
  }
}

void calcularAngulosDesdePosicion(float x, float y, float z) {

  // Evitar divisiones por cero o dominio inválido
  if (abs(x) > l1) {
    Serial.println(F("Error: x fuera de rango para l1"));
    return;
  }

  // Calcular tita1
  float tita1 = acos(x / l1) - PI / 2;
  float tita1g = degrees(tita1);

  // Contribución en Y de tita1
  float py1 = sqrt(l1 * l1 - x * x);

  // Distancias auxiliares para plano Y-Z
  float auxz = z - l2;
  float auxy = y - py1;
  float d = sqrt(auxz * auxz + auxy * auxy);

  // Calcular tita3
  float cos_tita3 = (l3 * l3 + l4 * l4 - d * d) / (2 * l3 * l4);
  if (cos_tita3 < -1 || cos_tita3 > 1) {
    Serial.println(F("Error: ángulo tita3 fuera de dominio"));
    return;
  }

  float tita3 = acos(cos_tita3);
  float tita3g = degrees(tita3);

  // Calcular tita2
  float cos_tita2 = (l3 * l3 + d * d - l4 * l4) / (2 * l3 * d);
  if (cos_tita2 < -1 || cos_tita2 > 1) {
    Serial.println(F("Error: ángulo tita2 fuera de dominio"));
    return;
  }
  // Angulo entre la horizontal y el punto
  float aux = atan2(auxy, auxz);
  aux = degrees(aux);

  // Offset mecanico en tita2
  int aux_off_2 = 48;

  float tita2 = acos(cos_tita2);
  float tita2g = degrees(tita2) - aux + aux_off_2;  // Ajuste por convención

  // Mostrar resultados

  Serial.printf("== Cinemática Inversa P(%.2f %.2f %.2f) ==\n", x, y, z);
  Serial.printf("Tita1: %.2f\n", tita1g);
  Serial.printf("Tita2: %.2f\n", tita2g);
  Serial.printf("Tita3: %.2f\n", tita3g);

  // Mover servos (si querés hacerlo automático)
  servos[0]->startEaseTo((int)tita1g);
  servos[1]->startEaseTo((int)tita2g);
  servos[2]->startEaseTo((int)tita3g);
}