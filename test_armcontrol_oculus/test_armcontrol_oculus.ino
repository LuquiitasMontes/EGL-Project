// LIBRERIAS PARA CONTROL BRAZO
#include <Arduino.h>
#define USE_PCA9685_SERVO_EXPANDER  // Activating this enables the use of the PCA9685 I2C expander chip/board.
#define USE_SERVO_LIB               // If USE_PCA9685_SERVO_EXPANDER is defined, Activating this enables force additional using of regular servo library.
#include "ServoEasing.hpp"
#include "PinDefinitionsAndMore.h"
// LIBRERIAS CONEXION
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <WiFiUdp.h>

// ---VARIABLES CONTROL BRAZO---
ServoEasing Servo1AtPCA9685(PCA9685_DEFAULT_ADDRESS);  // Hombro 1
ServoEasing Servo2AtPCA9685(PCA9685_DEFAULT_ADDRESS);  // Hombro 2
ServoEasing Servo3AtPCA9685(PCA9685_DEFAULT_ADDRESS);  // Codo
ServoEasing Servo4AtPCA9685(PCA9685_DEFAULT_ADDRESS);  // Muñeca
ServoEasing Servo5AtPCA9685(PCA9685_DEFAULT_ADDRESS);  // Gripper
const int startDegreeServo1 = 90;  // Hombro 1  0º - 115º
int s1_min = 0 , s1_max = 115 , s1_pos = startDegreeServo1;
const int startDegreeServo2 = 90;  // Hombro 2  10º - 180º
int s2_min = 0 , s2_max = 175 , s2_pos = startDegreeServo2;
const int startDegreeServo3 = 90;  // Codo      0º - 125º
int s3_min = 0 , s3_max = 140 , s3_pos = startDegreeServo3;
const int startDegreeServo4 = 0;   // Muñeca    0º - 180º
int s4_min = 0 , s4_max = 180 , s4_pos = startDegreeServo4;
const int startDegreeServo5 = 115;  // Gripper   35º Cerrrado - 115º Abierto
int s5_cerrado = 35 , s5_abierto = 115 , s5_pos = startDegreeServo5;
// VELOCIDAD Servos
int vel_servos = 6;
// Arreglo de punteros a los servos así:
ServoEasing* servos[] = { &Servo1AtPCA9685, &Servo2AtPCA9685, &Servo3AtPCA9685, &Servo4AtPCA9685, &Servo5AtPCA9685 };
const int NUM_SERVOS = 5;
int movimiento = 3;
//Pal gripper
double gripValue = 0;
int lastgrip = 0;
volatile int moviendo = 0;
// ---VARIABLES CONEXION---
using namespace websockets;

// Red
const char* ssid = "Elu Niverso";
const char* password = "callmeelu123";

// WebSocket
WebsocketsServer server;
WebsocketsClient client;
bool cliente_conectado = false;

// UDP broadcast
WiFiUDP udp;
const int udpPort = 8888;
bool seguir_broadcast = true;

// Tareas
void tareaMotores(void * pvParameters);
void tareaServidor(void * pvParameters);
void tareaBroadcast(void * pvParameters);
void procesarComando(String comando);



void setup() {
  Serial.begin(115200);
  
  // CONECTAR SERVOS AL DRIVER
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

    Serial.println(F("AÑADIR SERVOS AL DRIVER PCA9685"));
    Servo1AtPCA9685.attach(0, startDegreeServo1);
    Servo2AtPCA9685.attach(1, startDegreeServo2);
    Servo3AtPCA9685.attach(2, startDegreeServo3);
    Servo4AtPCA9685.attach(3, startDegreeServo4);
    Servo5AtPCA9685.attach(4, startDegreeServo5);
  }
  // Wait for servos to reach start position.
  delay(500);

  // Conexión WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado. IP: " + WiFi.localIP().toString());

  // WebSocket
  server.listen(82);
  Serial.println("Servidor WebSocket iniciado.");

  // Tareas
  xTaskCreatePinnedToCore(tareaServidor, "TareaServidor", 5000, NULL, 1, NULL, 1);   // Core 1
  xTaskCreatePinnedToCore(tareaBroadcast, "TareaBroadcast", 4000, NULL, 1, NULL, 1); // Core 1
  xTaskCreatePinnedToCore(tareaMotores, "TareaMotores", 4000, NULL, 1, NULL, 0); // Core 0

}


void tareaServidor(void * pvParameters) {
  while(1) {
    server.poll();

    if (!cliente_conectado && server.poll()) {
      client = server.accept();
      cliente_conectado = true;
      Serial.println("Cliente conectado.");
    }

    if (cliente_conectado) {
      if (client.available()) {
        WebsocketsMessage msg = client.readBlocking();
        String comando = msg.data();
        Serial.println("Mensaje recibido: " + comando);
        comando.trim();
        procesarComando(comando);
      }

      if (!client.available()) {
        // Frenar el carro en caso de desconexion
        cliente_conectado = false;
        seguir_broadcast = true;
        Serial.println("Cliente desconectado.");
      }
    }

    delay(10);
  }
}

void loop()
{
  
}

void tareaMotores(void * pvParameters)
{
  //Estados
  // 0 - Detenido
  // 1 - Moviendo adelante
  // 2 - Moviendo atras
  // 3 - moviendo arriba
  // 4 - moviendo abajo
  // 5 - moviendo izquierda
  // 6 - moviendo derecha
 while(1)
 {
    switch (moviendo)
    {
      case 1:
        s2_pos -= movimiento;
        if (s2_pos < s2_min)
        {
          s2_pos = s2_min;
        }
        servos[1]->write(s2_pos);
        break;
      case 2:
        s2_pos += movimiento;
        if (s2_pos > s2_max)
        {
          s2_pos = s2_max;
        }
        servos[1]->write(s2_pos);
        break;
      case 3:
        s3_pos -= movimiento;
        if (s3_pos < s3_min)
        {
          s3_pos = s3_min;
        }
        servos[2]->write(s3_pos);
        break;
      case 4:
        s3_pos += movimiento;
        if (s3_pos > s3_max)
        {
          s3_pos = s3_max;
        }
        servos[2]->write(s3_pos);
        break;
      case 5:
        s4_pos -= movimiento;
        if (s4_pos < s4_min)
        {
          s4_pos = s4_min;
        }
        servos[3]->write(s4_pos);
        break;
      case 6:
        s4_pos += movimiento;
        if (s4_pos > s4_max)
        {
          s4_pos = s4_max;
        }
        servos[3]->write(s4_pos);
        break;
      
    }
    delay(30);
 }
  
}

void tareaBroadcast(void * pvParameters) {
  while(1) {
    if (seguir_broadcast) {
      String msg = "arm1 here";
      udp.beginPacket(IPAddress(255, 255, 255, 255), udpPort);
      udp.print(msg);
      udp.endPacket();
      Serial.println("Broadcast: " + msg);
    }
    delay(500);
  }
}

void procesarComando(String comando) {
  //Hay que modificar esta lista de comandos para que sea más escalable o prolija
  if (comando == "arm1 connected") {
    seguir_broadcast = false;
    Serial.println("Broadcast detenido por 'arm1 connected'");
  } else {
    Serial.println(comando);
    if (comando == "Quieto"){
        moviendo = 0;
    }
    else if (comando == "Adelante"){
      moviendo = 1;
    }
    else if (comando == "Atrás"){
      moviendo = 2;
    }
    else if  (comando == "Arriba"){
      moviendo = 4;
    }
    else if  (comando == "Abajo"){
      moviendo = 3;
    }
    else if  (comando == "Derecha"){
      moviendo = 6;
    }
    else if  (comando == "Izquierda"){
      moviendo = 5;
    }
    else {
      gripValue = comando.toFloat();
      if (gripValue != lastgrip)
      {
        int angulo = 115 - int(80 * gripValue);
        servos[4]->write(angulo); // Asi lei que era mas responsivo
        lastgrip = gripValue;
      }
      
    }
  
  }
}


