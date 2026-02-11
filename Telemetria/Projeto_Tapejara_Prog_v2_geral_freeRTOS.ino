#include <MPU6050_tockn.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>

//PARAMETROS CIRCUITO
const float ganho = 62;//ganho no amplificador diferencial
const float rshunt = 1;//resistencia do resistor shunt
const float divisao_tensao = 2;

// Coeficientes do polinômio de 3º grau
const float a = 0;
const float b = 0;
const float c = 0;
const float d = 0;

// ---- CONFIG Wi-Fi ----
const char* ssid = "Benitez Lopes"; //hotspot
const char* password = "Churito1"; //senha

// ---- CONFIG MQTT ----
const char* mqtt_server = "192.168.154.186"; //IP maquina
const int mqtt_port = 1883;

//TOPICOS MQTT
const char* mqtt_topic_pitch = "sensor/mpu/pitch";
const char* mqtt_topic_roll  = "sensor/mpu/roll";
const char* mqtt_topic_yaw   = "sensor/mpu/yaw";
const char* mqtt_topic_corrente  = "bateria/corrente";
const char* mqtt_topic_tensão   = "bateria/tensão";

// ---- OBJETOS ----
WiFiClient espClient;
PubSubClient client(espClient);
MPU6050 mpu(Wire);
const int pinADCT = 34;  
const int pinADCI = 35;

//VARIAVEIS MEDIDAS
float Vbateria = 0;
float Ibateria = 0;
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
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    if (client.connect("ESP32_MPU6050")) {
      Serial.println("conectado!");
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando em 5s");
      vTaskDelay(5000 / portTICK_PERIOD_MS);;
    }
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
      int leituraV = analogRead(pinADCT); 
      somaV += leituraV; 
      vTaskDelay(1 / portTICK_PERIOD_MS);;  
    }
    float ADCbitsV = somaV / 20.0;
    Vbateria = (((a*ADCbitsV + b)*ADCbitsV + c)*ADCbitsV + d) * divisao_tensao; //calcula tensão pelo polinomio de calibração na forma de Horner
    // Debug serial
    Serial.print("Tensão Bateria: "); Serial.print(Vbateria);
}

void medida_I(){

long somaI = 0; 
    for (int i = 0; i < 20; i++) { 
      int leituraI = analogRead(pinADCI); 
      somaI += leituraI; 
      vTaskDelay(1 / portTICK_PERIOD_MS);;  
    }
    float ADCbitsI = somaI / 20.0;
    Ibateria = (((((a*ADCbitsI + b)*ADCbitsI + c)*ADCbitsI + d)/ganho)*divisao_tensao)/rshunt; //estimar corrente
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

    vTaskDelay(1 / portTICK_PERIOD_MS);
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
  if (!client.connected()) reconnect();
  client.loop();

  //AQUISIÇÃO DOS DADOS
  medida_VBateria();
  medida_I();

  // PUBLICA NO MQTT
  char msgV[16], msgI[16], msgPitch[16], msgRoll[16], msgYaw[16];
  dtostrf(Vbateria, 6, 2, msgV);
  dtostrf(Ibateria, 6, 2, msgI);
  dtostrf(pitch, 6, 2, msgPitch);
  dtostrf(roll,  6, 2, msgRoll);
  dtostrf(yaw,   6, 2, msgYaw);

  client.publish(mqtt_topic_tensão, msgV);
  client.publish(mqtt_topic_corrente, msgI);
  client.publish(mqtt_topic_pitch, msgPitch);
  client.publish(mqtt_topic_roll,  msgRoll);
  client.publish(mqtt_topic_yaw,   msgYaw);

  vTaskDelay(30 / portTICK_PERIOD_MS);
; 
  }
}
 void loop() {
  vTaskDelay(portMAX_DELAY);
}
