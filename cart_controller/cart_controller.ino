#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <AccelStepper.h>
#include <WiFiUdp.h>

using namespace websockets;

// WiFi
const char* ssid = "Elu Niverso";
const char* password = "callmeelu123";

// WebSocket
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
const int vel_motores = 250;
volatile int velocidad_m1 = 0;
volatile int velocidad_m2 = 0;

// UDP
WiFiUDP udp;
const int discoveryPort = 8888;

// Tareas
void tareaMotores(void * pvParameters);
void tareaServidor(void * pvParameters);
void tareaDiscovery(void * pvParameters);
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

  // Iniciar UDP
  udp.begin(discoveryPort);
  Serial.println("Esperando discovery en puerto UDP " + String(discoveryPort));

 //Tareas a realizar
  xTaskCreatePinnedToCore(tareaDiscovery, "TareaDiscovery", 4000, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(tareaServidor, "TareaServidor", 5000, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(tareaMotores, "TareaMotores", 5000, NULL, 1, NULL, 0);
}

void loop() {
  //Here lies Elunaro's hopes and dreams :c
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
        cliente_conectado = false;
        Serial.println("Cliente desconectado.");
        velocidad_m1 = 0;
        velocidad_m2 = 0;
        motor1.setSpeed(0);
        motor2.setSpeed(0);
      }
    }

    delay(10);
  }
}

void tareaMotores(void * pvParameters) {
  while(1) {
    motor1.runSpeed();
    motor2.runSpeed();
    delay(1);
  }
}

void tareaDiscovery(void * pvParameters) {
  char incoming[255];
  while(1) {
    int packetSize = udp.parsePacket();
    if (packetSize) {
      int len = udp.read(incoming, 255);
      if (len > 0) {
        incoming[len] = 0;
      }

      String mensaje = String(incoming);
      mensaje.trim();
      if (mensaje == "DISCOVER_WS") {
        IPAddress ip = WiFi.localIP();
        String respuesta = ip.toString(); // Puede incluir ":81" si querés ser explícito

        udp.beginPacket(udp.remoteIP(), udp.remotePort());
        udp.print(respuesta);
        udp.endPacket();

        Serial.println("Respondido a DISCOVER_WS con IP: " + respuesta);
      }
    }
    delay(10);
  }
}

void procesarComando(String comando) {
  if (comando == "F") { // Avanzar
    velocidad_m1 = vel_motores;
    velocidad_m2 = -vel_motores;
  } else if (comando == "B") { // Retroceder
    velocidad_m1 = -vel_motores;
    velocidad_m2 = vel_motores;
  } else if (comando == "A") {// Frenar
    velocidad_m1 = 0;
    velocidad_m2 = 0;
  } else if (comando == "R") {// Giro Derecha
    velocidad_m1 = -vel_motores;
    velocidad_m2 = -vel_motores;
  } else if (comando == "L") {// Giro Izquierda
    velocidad_m1 = vel_motores;
    velocidad_m2 = vel_motores;
  } else {
    Serial.println("Commandn't.");
  }

  motor1.setSpeed(velocidad_m1);
  motor2.setSpeed(velocidad_m2);
}
