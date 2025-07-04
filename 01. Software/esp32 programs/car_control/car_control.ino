#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <AccelStepper.h>
#include <WiFiUdp.h>
#include <ServoEasing.hpp>

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

// Pines motores
#define STEP_PIN_M1 18
#define DIR_PIN_M1 19
#define STEP_PIN_M2 21
#define DIR_PIN_M2 22
#define SERVO_PIN  32

// Motores
AccelStepper motor1(AccelStepper::DRIVER, STEP_PIN_M1, DIR_PIN_M1);
AccelStepper motor2(AccelStepper::DRIVER, STEP_PIN_M2, DIR_PIN_M2);
ServoEasing miServo;

// Velocidades
volatile int velocidad_m1 = 0;
volatile int velocidad_m2 = 0;
int vel_target_m1 = 0;
int vel_target_m2 = 0;
const int vel_max = 1000;
const int vel_step = 50;  // Paso de aceleración/desaceleración

// Tareas
void tareaMotores(void * pvParameters);
void tareaServidor(void * pvParameters);
void tareaBroadcast(void * pvParameters);
void procesarComando(String comando);

void setup() {
  Serial.begin(115200);

  miServo.attach(SERVO_PIN);
  miServo.setEasingType(EASE_LINEAR);
  miServo.setSpeed(30);
  miServo.write(60);
  delay(1000);

  // Configuración motores
  motor1.setMaxSpeed(2000);  // Max speed solo limita internamente
  motor2.setMaxSpeed(2000);

  // Conexión WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado. IP: " + WiFi.localIP().toString());

  // WebSocket
  server.listen(81);
  Serial.println("Servidor WebSocket iniciado.");

  // Tareas en distintos núcleos
  xTaskCreatePinnedToCore(tareaServidor, "TareaServidor", 5000, NULL, 1, NULL, 1);   // Core 1
  xTaskCreatePinnedToCore(tareaMotores, "TareaMotores", 5000, NULL, 1, NULL, 0);     // Core 0
  xTaskCreatePinnedToCore(tareaBroadcast, "TareaBroadcast", 4000, NULL, 1, NULL, 1); // Core 1
}

void loop() {}  // No usado

// Tarea de WebSocket
void tareaServidor(void * pvParameters) {
  while (1) {
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
        comando.trim();
        Serial.println("Mensaje recibido: " + comando);
        procesarComando(comando);
      }

      if (!client.available()) {
        cliente_conectado = false;
        seguir_broadcast = true;
        Serial.println("Cliente desconectado.");
        vel_target_m1 = 0;
        vel_target_m2 = 0;
        motor1.setSpeed(0);
        motor2.setSpeed(0);
      }
    }

    delay(10);
  }
}

// Tarea de motores con rampa por software
void tareaMotores(void * pvParameters) {
  unsigned long t_anterior = 0;

  while (1) {
    //Servo update
    miServo.update();
    // Ramp-up/ramp-down cada 10 ms
    if (millis() - t_anterior > 10) {
      if (velocidad_m1 < vel_target_m1) velocidad_m1 += vel_step;
      else if (velocidad_m1 > vel_target_m1) velocidad_m1 -= vel_step;

      if (velocidad_m2 < vel_target_m2) velocidad_m2 += vel_step;
      else if (velocidad_m2 > vel_target_m2) velocidad_m2 -= vel_step;

      motor1.setSpeed(velocidad_m1);
      motor2.setSpeed(velocidad_m2);

      t_anterior = millis();
    }

    motor1.runSpeed();
    motor2.runSpeed();

    vTaskDelay(1);  // Evita reinicio por watchdog
  }
}

// Tarea de broadcast por UDP
void tareaBroadcast(void * pvParameters) {
  while (1) {
    if (seguir_broadcast) {
      String msg = "cart here";
      udp.beginPacket(IPAddress(255, 255, 255, 255), udpPort);
      udp.print(msg);
      udp.endPacket();
      Serial.println("Broadcast: " + msg);
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

// Comandos de WebSocket
void procesarComando(String comando) {
  comando.trim();

  if (comando == "F") {
    vel_target_m1 = vel_max;
    vel_target_m2 = -vel_max;

  } else if (comando == "B") {
    vel_target_m1 = -vel_max;
    vel_target_m2 = vel_max;

  } else if (comando == "R") {
    miServo.startEaseTo(100);

  } else if (comando == "L") { 
    miServo.startEaseTo(20);

  } else if (comando == "A") {
    vel_target_m1 = 0;
    vel_target_m2 = 0;
    miServo.startEaseTo(60);

  } else if (comando == "car connected") {
    seguir_broadcast = false;
    Serial.println("Broadcast detenido por 'car connected'");

  } else {
    Serial.println("Comando no reconocido.");
  }
}
