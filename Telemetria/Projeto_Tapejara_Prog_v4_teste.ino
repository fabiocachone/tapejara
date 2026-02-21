#include <MPU6050_tockn.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>

//PARAMETROS CIRCUITO
const float ganho = 62;//ganho no amplificador diferencial
const float rshunt = 1;//resistencia do resistor shunt
const float divisao_tensao = 2;

/* Coeficientes do polinômio de 3º grau
const float a = 0;
const float b = 0;
const float c = 0;
const float d = 0;*/

// ---- CONFIG Wi-Fi ----
const char* ssid = "Benitez Lopes"; //hotspot
const char* password = "Churito1"; //senha

// ---- CONFIG MQTT ----
const char* mqtt_server = "192.168.98.186"; //IP maquina
const int mqtt_port = 1883;

//TOPICOS MQTT
//const char* mqtt_topic_pitch = "sensor/mpu/pitch";
//const char* mqtt_topic_roll  = "sensor/mpu/roll";
//const char* mqtt_topic_yaw   = "sensor/mpu/yaw";
//const char* mqtt_topic_corrente  = "bateria/corrente";
//const char* mqtt_topic_tensão   = "bateria/tensão";
const char* mqtt_telemetria = "drone/telemetria";

// ---- OBJETOS ----
WiFiClient espClient;
PubSubClient client(espClient);
MPU6050 mpu(Wire);
const int pinADCT = 34;  
const int pinADCI = 35;

// ---VARIAVEIS MEDIDAS---
float Vbateria = 3.3;
float Ibateria = 0.04;
volatile float pitch = 0;
volatile float roll = 0;
volatile float yaw = 0;

void setup_wifi() {
  vTaskDelay(10 / portTICK_PERIOD_MS);;
  Serial.println();
  Serial.print("Conectando em ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(500 / portTICK_PERIOD_MS);;
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  if (client.connected()) return;

  Serial.print("Tentando conexão MQTT...");

  if (client.connect("ESP32_MPU6050")) {
    Serial.println("conectado!");
  } else {
    Serial.print("falhou, rc=");
    Serial.println(client.state());
  }
}

void setup() {
  Serial.begin(115200);

//xTaskCreatePinnedToCore(função, nome, stack, parametros, prioridade, alteração manual, core);
   
  xTaskCreatePinnedToCore(TaskControle, "Controle", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(TaskTelemetria, "Telemetria", 8192, NULL, 1, NULL, 0);

}

void medida_VBateria(){

long somaV = 0; 
    for (int i = 0; i < 20; i++) { 
      int leituraV = analogReadMilliVolts(pinADCT); 
      somaV += leituraV; 
      vTaskDelay(1 / portTICK_PERIOD_MS);;  
    }
    float ADCbitsV = somaV / 20.0;
    Vbateria = ADCbitsV * divisao_tensao; //calcula tensão pelo polinomio de calibração na forma de Horner
    // Debug serial
    Serial.print("Tensão Bateria: "); Serial.print(Vbateria);
}

void medida_I(){

long somaI = 0; 
    for (int i = 0; i < 20; i++) { 
      int leituraI = analogReadMilliVolts(pinADCI); 
      somaI += leituraI; 
      vTaskDelay(1 / portTICK_PERIOD_MS);;  
    }
    float ADCbitsI = somaI / 20.0;
    Ibateria = ((ADCbitsI/ganho)*divisao_tensao)/rshunt; //estimar corrente
    // Debug serial
    Serial.print(" | Corrente: "); Serial.println(Ibateria);
}
  
void medidas_MPU6050(){

mpu.update();   

  pitch = mpu.getAngleX();
  roll  = mpu.getAngleY();
  yaw   = mpu.getAngleZ();   

  // Debug serial
  Serial.print("Pitch: "); Serial.print(pitch);
  Serial.print(" | Roll: "); Serial.print(roll);
  Serial.print(" | Yaw: ");  Serial.println(yaw);

}

void TaskControle(void *pvParameters) {
  
// MPU6050
  Wire.begin(21, 22);     
  mpu.begin();
  mpu.calcGyroOffsets(true); 
  Serial.println("MPU6050 inicializado!");

  //portas motores etc..
  
  for(;;) {
    medidas_MPU6050(); //usado na calibração

    // CONTROLE DE VOO (SERÁ IMPLEMENTADO DEPOIS)
    // PID, motores, etc

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}


void TaskTelemetria(void *pvParameters) {
  
// WiFi & MQTT
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

//BATERIA

  pinMode(pinADCT, INPUT); 
  pinMode(pinADCI, INPUT); 
  analogReadResolution(11); 
  
  for (;;){

    
    Serial.println("Telemetria rodando");
   if (!client.connected()) {
    reconnect();
  }
  client.loop();

  //AQUISIÇÃO DOS DADOS
  //medida_VBateria();
  //medida_I();

  // PUBLICA NO MQTT
  // PUBLICA NO MQTT
char payload[200];

snprintf(payload, sizeof(payload),
  "{\"pitch\":%.2f,\"roll\":%.2f,\"yaw\":%.2f,\"tensao\":%.2f,\"corrente\":%.2f}",
  pitch, roll, yaw, Vbateria, Ibateria
);

if (client.connected()) {
    client.publish(mqtt_telemetria, payload);
  }

  vTaskDelay(pdMS_TO_TICKS(1000)); // 1Hz pra teste
 
  }
}
 void loop() {
  vTaskDelay(portMAX_DELAY);
}