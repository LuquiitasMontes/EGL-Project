#include <WiFi.h>
#include <ArduinoWebsockets.h>
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


// Tareas
void tareaMotores(void * pvParameters);
void tareaServidor(void * pvParameters);
void tareaBroadcast(void * pvParameters);
void procesarComando(String comando);

void setup() {
  Serial.begin(115200);

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
        seguir_broadcast = true;
        Serial.println("Cliente desconectado.");
      }
    }

    delay(10);
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
  }

}

