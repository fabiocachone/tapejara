#include <WiFi.h>
#include <PubSubClient.h>

// ---- CONFIG Wi-Fi ----
const char* ssid = "Benitez Lopes";
const char* password = "Churito1";

// ---- CONFIG MQTT ----
const char* mqtt_server = "192.168.14.186";
const int mqtt_port = 1883;
const char* mqtt_topic = "sensor/potenciometro";

// ---- CONFIG PINOS ----
// Escolha um pino ADC válido: 32, 33, 34, 35, 36, 39
const int potPin = 34;  

// ---- Objetos WiFi e MQTT ----
WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando em ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("conectado!");
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5s");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  // Ajusta resolução do ADC do ESP32
  analogReadResolution(12);  // 0–4095
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int raw = analogRead(potPin);        // 0 a 4095
  float vout = (raw / 4095.0) * 3.3;   // Converte para volts

  Serial.print("Leitura: ");
  Serial.print(raw);
  Serial.print("  |  Tensão: ");
  Serial.print(vout);
  Serial.println(" V");

  // Envia via MQTT
  char msg[50];
  snprintf(msg, sizeof(msg), "%.2f", vout);
  client.publish(mqtt_topic, msg);

  delay(2000);
}

