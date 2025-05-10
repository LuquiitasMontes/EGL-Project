#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <AccelStepper.h>
#include <WiFiUdp.h>

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

// Motores
AccelStepper motor1(AccelStepper::DRIVER, STEP_PIN_M1, DIR_PIN_M1);
AccelStepper motor2(AccelStepper::DRIVER, STEP_PIN_M2, DIR_PIN_M2);

// Velocidades
volatile int velocidad_m1 = 0;
volatile int velocidad_m2 = 0;
int vel_value = 400;


// Tareas
void tareaMotores(void * pvParameters);
void tareaServidor(void * pvParameters);
void tareaBroadcast(void * pvParameters);
void procesarComando(String comando);

void setup() {
  Serial.begin(115200);

  // Configs de motores
  motor1.setMaxSpeed(1500);
  motor2.setMaxSpeed(1500);
  motor1.setAcceleration(0);
  motor2.setAcceleration(0);

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

  // Tareas
  xTaskCreatePinnedToCore(tareaServidor, "TareaServidor", 5000, NULL, 1, NULL, 1);   // Core 1
  xTaskCreatePinnedToCore(tareaMotores, "TareaMotores", 5000, NULL, 1, NULL, 0);     // Core 0
  xTaskCreatePinnedToCore(tareaBroadcast, "TareaBroadcast", 4000, NULL, 1, NULL, 1); // Core 1
}

void loop() {}

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

void tareaBroadcast(void * pvParameters) {
  while(1) {
    if (seguir_broadcast) {
      IPAddress ip = WiFi.localIP(); // Obtener direccion del esp32
      String msg = ip.toString();
      udp.beginPacket(IPAddress(255, 255, 255, 255), udpPort);
      udp.print(msg);
      udp.endPacket();
      Serial.println("Broadcast IP: " + msg);
    }
    delay(500);
  }
}

void procesarComando(String comando) {
  //Hay que modificar esta lista de comandos para que sea más escalable o prolija
  if (comando == "F") {
    velocidad_m1 = vel_value;
    velocidad_m2 = -vel_value;

  } else if (comando == "B") {
    velocidad_m1 = -vel_value;
    velocidad_m2 = vel_value;

  } else if (comando == "A") {
    velocidad_m1 = 0;
    velocidad_m2 = 0;

  } else if (comando == "R") {
    velocidad_m1 = -vel_value;
    velocidad_m2 = -vel_value;

  } else if (comando == "L") {
    velocidad_m1 = vel_value;
    velocidad_m2 = vel_value;

  } else if (comando == "car connected") {
    seguir_broadcast = false;
    Serial.println("Broadcast detenido por 'car connected'");
  } else {
    Serial.println("Comando no reconocido.");
  }
  // Configurar velocidades
  motor1.setSpeed(velocidad_m1);
  motor2.setSpeed(velocidad_m2);
}

