#include <WiFi.h>
#include <ArduinoWebsockets.h>

const char* ssid = "Elu Niverso";
const char* password = "callmeelu123";

using namespace websockets;
WebsocketsServer server;

// PARA CONTROL DEL CARRO MOVIL
#include <AccelStepper.h>

#define STEP_PIN_M1 18
#define DIR_PIN_M1 19
#define STEP_PIN_M2 21
#define DIR_PIN_M2 22

AccelStepper motor1(AccelStepper::DRIVER, STEP_PIN_M1, DIR_PIN_M1);
AccelStepper motor2(AccelStepper::DRIVER, STEP_PIN_M2, DIR_PIN_M2);

int pasos = 1;
int aceleracion = 400;
int velmax = 1400;

// PARA MANTENER UNA IP ESTATICA
// IP estática deseada
IPAddress local_IP(192, 168, 0, 175);
IPAddress gateway(192, 168, 16, 236);
IPAddress subnet(255, 255, 255, 0);

void setup() {
  Serial.begin(115200);

  // Configurar IP estática
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("⚠ Error al configurar IP estática");
  }

  Serial.println("CONTROL DE CARRO MOVIL INICIADO");
  // PARA CONTROL DEL CARRO MOVIL
  while (!Serial);  // Esperar a que el puerto esté listo (opcional, depende de la placa)

  Serial.println("Ingrese Velocidad Maxima (entre 1000 y 2000): ");
  int velmax = esperarNumeroDesdeSerial();

  Serial.println("Ingrese Aceleración (entre 300 y 800): ");
  int aceleracion = esperarNumeroDesdeSerial();

  motor1.setMaxSpeed(velmax);
  motor1.setAcceleration(aceleracion);
  motor2.setMaxSpeed(velmax);
  motor2.setAcceleration(aceleracion);

  // Conectar a Wi-Fi

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado. Dirección IP: " + WiFi.localIP().toString());

  // Iniciar servidor WebSocket
  server.listen(81);
  Serial.println("Servidor WebSocket iniciado en el puerto 81.");
}

void loop() {

  if (server.poll()) {
    WebsocketsClient client = server.accept();
    Serial.println("Cliente conectado.");

    while (client.available()) {
      // Si los motores aún están en movimiento, seguir ejecutando
      motor1.run();
      motor2.run();

      WebsocketsMessage msg = client.readBlocking();
      String comando = msg.data();

      Serial.print("Mensaje recibido: ");
      Serial.println(comando);
      comando.trim(); // Saca espacios o saltos de línea

      // Solo leemos nuevo comando si ambos motores están detenidos
      if (!motor1.isRunning() && !motor2.isRunning()) {
        procesarComando(comando);
      }
      else {
        client.send("Motores en movimiento. Espere a que terminen.");
      }
    }
  }
}

void procesarComando(String comando) {

  if (comando == "F") {
    Serial.println("Comando: ADELANTE reconocido");
    motor1.moveTo(motor1.currentPosition() + pasos);
    motor2.moveTo(motor2.currentPosition() + pasos);
  }
  else if (comando == "B") {
    Serial.println("Comando: ATRAS reconocido");
    motor1.moveTo(motor1.currentPosition() - pasos);
    motor2.moveTo(motor2.currentPosition() - pasos);
  }
  else if (comando == "R") {
    Serial.println("Comando: DERECHA reconocido");
    motor1.moveTo(motor1.currentPosition() + pasos);
    motor2.moveTo(motor2.currentPosition() - pasos);
  }
  else if (comando == "L") {
    Serial.println("Comando: IZQUIERDA reconocido");
    motor1.moveTo(motor1.currentPosition() - pasos);
    motor2.moveTo(motor2.currentPosition() + pasos);
  }
  else {
    Serial.println("Comando no reconocido");
  }
}

int esperarNumeroDesdeSerial() {
  while (Serial.available() == 0) {
    // Espera activa hasta que haya algo disponible
  }
  String input = Serial.readStringUntil('\n');
  input.trim(); // Elimina espacios y saltos de línea
  Serial.println(input);
  return input.toInt();
}
