#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <AccelStepper.h>

using namespace websockets;

// WiFi
const char* ssid = "Elu Niverso";
const char* password = "callmeelu123";

// Servidor WebSocket
WebsocketsServer server;
WebsocketsClient client;
bool cliente_conectado = false;

// Pines motores
#define STEP_PIN_M1 18
#define DIR_PIN_M1 19
#define STEP_PIN_M2 21
#define DIR_PIN_M2 22

// Motores
AccelStepper motor1(AccelStepper::DRIVER, STEP_PIN_M1, DIR_PIN_M1);
AccelStepper motor2(AccelStepper::DRIVER, STEP_PIN_M2, DIR_PIN_M2);

// Velocidades
volatile int velocidad_m1 = 0;
volatile int velocidad_m2 = 0;

// Tareas
void tareaMotores(void * pvParameters);
void tareaServidor(void * pvParameters);
void procesarComando(String comando);

void setup() {
  Serial.begin(115200);

  // Motores
  motor1.setMaxSpeed(1000);
  motor1.setAcceleration(400);
  motor2.setMaxSpeed(1000);
  motor2.setAcceleration(400);

  // WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado. IP: " + WiFi.localIP().toString());

  // WebSocket
  server.listen(81);
  Serial.println("Servidor WebSocket iniciado.");

  // Crear tareas
  xTaskCreatePinnedToCore(tareaServidor, "TareaServidor", 5000, NULL, 1, NULL, 1); // Core 1
  xTaskCreatePinnedToCore(tareaMotores, "TareaMotores", 5000, NULL, 1, NULL, 0);    // Core 0
}

void loop() {
  // No se usa loop principal
}

void tareaServidor(void * pvParameters) {
  for(;;) {
    server.poll();  // Siempre escuchar

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
        // Cliente desconectado
        cliente_conectado = false;
        Serial.println("Cliente desconectado.");
        velocidad_m1 = 0;
        velocidad_m2 = 0;
        motor1.setSpeed(0);
        motor2.setSpeed(0);
      }
    }

    delay(10); // Pequeño respiro
  }
}

void tareaMotores(void * pvParameters) {
  for(;;) {
    motor1.runSpeed();
    motor2.runSpeed();
    delay(1); // Ejecutar muy rápido
  }
}

void procesarComando(String comando) {
  if (comando == "F") {
    velocidad_m1 = 250;
    velocidad_m2 = -250;
  } else if (comando == "B") {
    velocidad_m1 = -250;
    velocidad_m2 = 250;
  } else if (comando == "A") {
    velocidad_m1 = 0;
    velocidad_m2 = 0;
  } else if (comando == "R") {
    velocidad_m1 = -250;
    velocidad_m2 = -250;
  } else if (comando == "L") {
    velocidad_m1 = 250;
    velocidad_m2 = 250;
  } else {
    Serial.println("Comando no reconocido.");
  }

  motor1.setSpeed(velocidad_m1);
  motor2.setSpeed(velocidad_m2);
}
