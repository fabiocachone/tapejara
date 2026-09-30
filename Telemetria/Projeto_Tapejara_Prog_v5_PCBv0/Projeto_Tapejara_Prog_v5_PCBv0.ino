#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <sys/time.h>
#include <Adafruit_BMP280.h> 
#include <Adafruit_MPU6050.h>
#include <Adafruit_QMC5883P.h> 
#include <Adafruit_Sensor.h> 

//Portas

#define SDA 39
#define SCL 40
#define sinalTensao 1
#define led1 6
#define led2 41
#define led3 43
#define led4 5

// CONFIG Wi-Fi
const char* ssid = "nome da rede";
const char* password = "senha";

// CONFIG MQTT
const char* mqtt_server = "IP maquina";
const int mqtt_port = 1883;

const char* mqtt_telemetria = "drone/controle";

// OBJETOS
WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_BMP280 bmp;
Adafruit_MPU6050 mpu;
Adafruit_QMC5883P qmc;

// VARIÁVEIS MEDIDAS
float Vbateria = 0;

volatile float rpitch = 0;
volatile float rroll = 0;
volatile float ryaw = 0;

volatile float accpitch = 0;
volatile float accroll = 0;
volatile float accyaw = 0;

volatile float xmag = 0;
volatile float ymag = 0;
volatile float zmag = 0;

volatile float pressao = 0;
volatile float temperatura = 0;

// Timestamp será inteiro em milissegundos
uint64_t timestamp = 0;


// WIFI

void setup_wifi() {

  vTaskDelay(10 / portTICK_PERIOD_MS);

  Serial.println();
  Serial.print("Conectando em ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {

    vTaskDelay(500 / portTICK_PERIOD_MS);

    Serial.print(".");
  }

  Serial.println("\nWiFi conectado!");

  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}


// =====================================================
// NTP
// =====================================================

void setup_NTP() {

  Serial.println("Sincronizando horario via NTP...");

  // GMT-3 = Brasil
  // 0 = sem horario de verao
  configTime(-3 * 3600, 0,
             "pool.ntp.org",
             "time.nist.gov");

  struct tm timeinfo;

  while (!getLocalTime(&timeinfo)) {

    Serial.println("Aguardando NTP...");

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  Serial.println("Horario sincronizado!");

  Serial.printf(
    "Data: %02d/%02d/%04d %02d:%02d:%02d\n",
    timeinfo.tm_mday,
    timeinfo.tm_mon + 1,
    timeinfo.tm_year + 1900,
    timeinfo.tm_hour,
    timeinfo.tm_min,
    timeinfo.tm_sec
  );
}


// =====================================================
// TIMESTAMP
// =====================================================

uint64_t getTimestampMillis() {

  struct timeval tv;

  gettimeofday(&tv, NULL);

  uint64_t timestamp_ms =
      ((uint64_t)tv.tv_sec * 1000ULL) +
      ((uint64_t)tv.tv_usec / 1000ULL);

  return timestamp_ms;
}


// =====================================================
// MQTT
// =====================================================

void reconnect() {

  if (client.connected())
    return;

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
  Wire.begin(SDA, SCL);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT);

  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
  digitalWrite(led3, LOW);
  digitalWrite(led4, LOW);
  delay(500);
  
  //Setup BMP280
  if (!bmp.begin(0x76, 0x58)) {
    Serial.println("Could not find BMP280 sensor, check wiring.");
    while (1)
      delay(10);
    }
  digitalWrite(led1, HIGH);
  delay(500);

  //Setup MPU6050
  if (!mpu.begin(0x68, &Wire, 0)) {
    Serial.println("Failed to find MPU6050 chip");
    while (1)
      delay(10); 
    }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  digitalWrite(led2, HIGH);
  delay(500);

  //Setup QMC5883P 

  if (!qmc.begin(0x2C, &Wire)) {  // O endereço I2C encontrado foi esse
    Serial.println("Failed to find QMC5883P chip");
    while (1)
      delay(10);
  }
  qmc.setMode(QMC5883P_MODE_NORMAL);
  qmc.setODR(QMC5883P_ODR_50HZ);
  qmc.setOSR(QMC5883P_OSR_4);
  qmc.setDSR(QMC5883P_DSR_2);
  qmc.setRange(QMC5883P_RANGE_8G);
  qmc.setSetResetMode(QMC5883P_SETRESET_ON);
  digitalWrite(led3, HIGH);
  delay(500);

  xTaskCreatePinnedToCore(TaskControle,"Controle",4096,NULL,2,NULL,1);
  xTaskCreatePinnedToCore(TaskTelemetria,"Telemetria",8192,NULL,1,NULL,0);

}

void TaskControle(void *pvParameters) {

 

  for (;;) {

  //Medida BMP280

  temperatura = bmp.readTemperature();
  pressao = bmp.readPressure();

  //Medida MPU6050
  
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  accpitch = a.acceleration.x;
  accroll = a.acceleration.y;
  accyaw = a.acceleration.z;

  rpitch = g.gyro.x;
  rroll = g.gyro.y;
  ryaw = g.gyro.z;

  //Medida QMC5883P
  
  int16_t x, y, z;

  qmc.getRawMagnetic(&x, &y, &z);

  xmag = x;
  ymag = y;
  zmag = z;


   vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void TaskTelemetria(void *pvParameters) {

  setup_wifi();

  
    WiFiClient teste;

  Serial.println("Testando TCP...");

  if (teste.connect("10.48.20.186", 1883)) {
    Serial.println("TCP: CONECTOU!");
    teste.stop();
  } else {
    Serial.println("TCP: FALHOU!");
  }

  client.setServer(mqtt_server, mqtt_port);

  setup_NTP();

  digitalWrite(led4, HIGH);

  for (;;) {

    Serial.println("Telemetria rodando");


    if (!client.connected()) {
      reconnect();
    }
    client.loop();

    timestamp = getTimestampMillis();
    Vbateria = medeTensao();

    char payload[500];

    snprintf(
      payload,
      sizeof(payload),

      "{"
      "\"timestamp\":%llu,"
      "\"rpitch\":%.2f,"
      "\"rroll\":%.2f,"
      "\"ryaw\":%.2f,"
      "\"accpitch\":%.2f,"
      "\"accroll\":%.2f,"
      "\"accyaw\":%.2f,"
      "\"xmag\":%.2f,"
      "\"ymag\":%.2f,"
      "\"zmag\":%.2f,"
      "\"tensao\":%.2f,"
      "\"pressao\":%.2f,"
      "\"temperatura\":%.2f"
      "}",

      (unsigned long long)timestamp,

      rpitch,
      rroll,
      ryaw,

      accpitch,
      accroll,
      accyaw,

      xmag,
      ymag,
      zmag,

      Vbateria,

      pressao,
      temperatura
    );

    if (client.connected()) {

      client.publish(
        mqtt_telemetria,
        payload
      );
    }

    Serial.println(payload);

    vTaskDelay(
      pdMS_TO_TICKS(1000)
    );
  }
}

void loop() {

  vTaskDelay(portMAX_DELAY);
}

float divisor_tensao = 2;
float medeTensao()
{
  float tensao = (analogReadMilliVolts(sinalTensao) / 1000.0)*divisor_tensao; 
  return tensao;
}