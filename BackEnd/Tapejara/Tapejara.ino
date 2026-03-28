// Tapejara.ino - Versão Simplificada com FreeRTOS
// Conexão WiFi à rede CACHONE + Stream MJPEG

#include <WiFi.h>
#include <WebServer.h>
#include <WiFiUdp.h>
#include "esp_camera.h"
#include "driver/ledc.h"  // Required for ledcSetup and ledcAttachPin

// ===== CONFIGURAÇÕES =====
const char* WIFI_SSID = "CACHONE";
const char* WIFI_PASS = "f22485910";

const char* STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=frame";
const char* STREAM_BOUNDARY = "\r\n--frame\r\n";
const char* STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

const uint8_t STATUS_LED_PIN = 4;

// Pinos do modelo AI Thinker (ESP32-CAM)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ===== PINOS DOS MOTORES (PWM) =====
// NOTA: ESP32-CAM tem pinos limitados. Usando GPIO12, 13, 14, 15 (se disponível)
// Se seu módulo usar diferentes pinos, ajuste aqui
#define MOTOR1_PIN        12  // Motor frente-esquerdo (FL)
#define MOTOR2_PIN        13  // Motor frente-direito (FR)
#define MOTOR3_PIN        14  // Motor trás-esquerdo (BL)
#define MOTOR4_PIN        15  // Motor trás-direito (BR)

// ===== VARIÁVEIS GLOBAIS =====
WebServer server(80);
WiFiUDP udpServer;
bool cameraOk = false;
bool wifiOk = false;

// ===== UDP CONFIG =====
const int UDP_PORT = 9000;
const int UDP_BUFFER_SIZE = 64;

// ===== ESTRUTURA CONTROLE =====
struct ControlState {
  float roll;
  float pitch;
  float yaw;
  float throttle;
  int buttons;
  unsigned long lastUpdate;
};

ControlState currentControl = {0.0f, 0.0f, 0.0f, 0.0f, 0, 0};

// ===== PWM / ESC CONFIG =====
const int PWM_FREQ = 50;          // 50 Hz (standard para ESCs)
const int PWM_RESOLUTION = 16;    // 16-bit resolution (0-65535)
const int MIN_PWM_VALUE = 1000;   // 1ms pulse (motors off)
const int MAX_PWM_VALUE = 2000;   // 2ms pulse (motors full speed)
const int PWM_RANGE = MAX_PWM_VALUE - MIN_PWM_VALUE;

// ===== ESTRUTURA PWM MOTORS =====
struct MotorPWM {
  uint16_t motor1;  // Front-Left (FL)
  uint16_t motor2;  // Front-Right (FR)
  uint16_t motor3;  // Back-Left (BL)
  uint16_t motor4;  // Back-Right (BR)
};

MotorPWM motorPWM = {MIN_PWM_VALUE, MIN_PWM_VALUE, MIN_PWM_VALUE, MIN_PWM_VALUE};

// ===== INICIALIZAÇÃO DA CÂMERA =====
bool initCamera() {
  camera_config_t config;
  
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  
  config.xclk_freq_hz = 24000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_UXGA;
  config.jpeg_quality = 12;
  config.fb_count     = 2;
  
  if (esp_camera_init(&config) != ESP_OK) {
    return false;
  }
  
  // Configura rotação 90 graus no sentido horário
  sensor_t * s = esp_camera_sensor_get();
  if (s != NULL) {
    s->set_vflip(s, 1);      // Flip vertical
    s->set_hmirror(s, 0);    // Mirror horizontal
    s->set_brightness(s, 0); // Brilho neutro
    s->set_contrast(s, 2);   // Contraste máximo
    s->set_saturation(s, 2); // Saturação máxima
    s->set_sharpness(s, 2);  // Nitidez máxima
    s->set_gainceiling(s, GAINCEILING_16X); // Ganho máximo para baixa luz
  }
  
  return true;
}

/* ===== INICIALIZAÇÃO PWM (LEDC) ===== */
void initPWM() {
  Serial.println("\n--- Inicializando PWM (LEDC) ---");
  
  // Configura canais LEDC para cada motor
  // LEDC: LED Control PWM do ESP32
  // Timer 0, canais 0-3 para os 4 motores
  
  ledcSetup(0, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(1, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(2, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(3, PWM_FREQ, PWM_RESOLUTION);
  
  // Associa pinos aos canais
  ledcAttachPin(MOTOR1_PIN, 0);
  ledcAttachPin(MOTOR2_PIN, 1);
  ledcAttachPin(MOTOR3_PIN, 2);
  ledcAttachPin(MOTOR4_PIN, 3);
  
  // Inicializa todos os motores em posição "off" (1000 us = MIN_PWM)
  ledcWrite(0, MIN_PWM_VALUE);
  ledcWrite(1, MIN_PWM_VALUE);
  ledcWrite(2, MIN_PWM_VALUE);
  ledcWrite(3, MIN_PWM_VALUE);
  
  Serial.println("✓ PWM inicializado (50 Hz, 16-bit)");
  Serial.println("  Motor1 (FL) -> GPIO " + String(MOTOR1_PIN) + " (Canal 0)");
  Serial.println("  Motor2 (FR) -> GPIO " + String(MOTOR2_PIN) + " (Canal 1)");
  Serial.println("  Motor3 (BL) -> GPIO " + String(MOTOR3_PIN) + " (Canal 2)");
  Serial.println("  Motor4 (BR) -> GPIO " + String(MOTOR4_PIN) + " (Canal 3)");
  Serial.println("  Range PWM: " + String(MIN_PWM_VALUE) + " - " + String(MAX_PWM_VALUE) + " µs\n");
}

/* ===== MOTOR MIXING (Quadcopter X-Frame) ===== */
// Converte roll, pitch, yaw, throttle em 4 sinais PWM
// Configuração X-Frame (diagonal):
//    FL(1)     FR(2)      Motor layout:
//      \ /                 1: Front-Left (FL)
//       X       ->         2: Front-Right (FR)
//      / \                 3: Back-Left (BL)
//    BL(3)     BR(4)       4: Back-Right (BR) 
//
// Fórmula de mixing para Quad X:
// Motor1 (FL) = throttle + pitch + roll - yaw
// Motor2 (FR) = throttle + pitch - roll + yaw
// Motor3 (BL) = throttle - pitch + roll + yaw
// Motor4 (BR) = throttle - pitch - roll - yaw

void calculateMotorMix(const ControlState& control) {
  // Limpa valores de controle para range [-1, +1]
  float thr = constrain(control.throttle, 0.0f, 1.0f);  // 0 a 1
  float pit = constrain(control.pitch, -1.0f, 1.0f);    // -1 a +1
  float rol = constrain(control.roll, -1.0f, 1.0f);     // -1 a +1
  float yaw = constrain(control.yaw, -1.0f, 1.0f);      // -1 a +1
  
  // Escala: throttle (0-1) mapeia para PWM range completo
  // pitch, roll, yaw (-1 a +1) adicionam/subtraem do throttle
  float m1 = thr + pit + rol - yaw;
  float m2 = thr + pit - rol + yaw;
  float m3 = thr - pit + rol + yaw;
  float m4 = thr - pit - rol - yaw;
  
  // Normaliza para evitar saturação (se um motor > 1, redimensiona todos)
  float maxMotor = max({m1, m2, m3, m4});
  if (maxMotor > 1.0f) {
    m1 /= maxMotor;
    m2 /= maxMotor;
    m3 /= maxMotor;
    m4 /= maxMotor;
  }
  
  // Clipa para range [0, 1]
  m1 = constrain(m1, 0.0f, 1.0f);
  m2 = constrain(m2, 0.0f, 1.0f);
  m3 = constrain(m3, 0.0f, 1.0f);
  m4 = constrain(m4, 0.0f, 1.0f);
  
  // Mapeia [0, 1] para PWM range [MIN_PWM_VALUE, MAX_PWM_VALUE]
  motorPWM.motor1 = MIN_PWM_VALUE + (m1 * PWM_RANGE);
  motorPWM.motor2 = MIN_PWM_VALUE + (m2 * PWM_RANGE);
  motorPWM.motor3 = MIN_PWM_VALUE + (m3 * PWM_RANGE);
  motorPWM.motor4 = MIN_PWM_VALUE + (m4 * PWM_RANGE);
}

/* ===== ESCREVE PWM NOS MOTORES ===== */
void updateMotorPWM() {
  ledcWrite(0, motorPWM.motor1);
  ledcWrite(1, motorPWM.motor2);
  ledcWrite(2, motorPWM.motor3);
  ledcWrite(3, motorPWM.motor4);
}

// ===== INICIALIZAÇÃO UDP =====
void setupUDP() {
  Serial.println("Inicializando UDP no porto " + String(UDP_PORT) + "...");
  if (udpServer.begin(UDP_PORT)) {
    Serial.println("✓ UDP iniciado com sucesso na porta " + String(UDP_PORT));
  } else {
    Serial.println("✗ Falha ao inicializar UDP");
  }
}

/* ===== PARSER DE COMANDOS UDP ===== */
// Formato esperado do cliente (24 bytes POUCO-ENDIAN):
// [0-3]: magic (0xED, 0x01, 0x00, 0x00)
// [4-5]: seq (uint16)
// [6-9]: roll (float)
// [10-13]: pitch (float)
// [14-17]: yaw (float)
// [18-21]: throttle (float)
// [22-23]: buttons (uint16)
// (Checksum é validado opcionalmente)

void processUDPCommand(const uint8_t* buffer, int len) {
  if (len < 24) {
    Serial.println("⚠ Pacote UDP muito curto: " + String(len) + " bytes");
    return;
  }

  // Validar magic bytes
  if (buffer[0] != 0xED || buffer[1] != 0x01) {
    Serial.println("⚠ Magic bytes inválidos");
    return;
  }

  // Parser dos valores em little-endian
  uint16_t seq = buffer[4] | (buffer[5] << 8);
  
  // Copiar floats (4 bytes cada)
  float roll, pitch, yaw, throttle;
  memcpy(&roll, &buffer[6], 4);
  memcpy(&pitch, &buffer[10], 4);
  memcpy(&yaw, &buffer[14], 4);
  memcpy(&throttle, &buffer[18], 4);
  
  uint16_t buttons = buffer[22] | (buffer[23] << 8);

  // Atualizar estado global
  currentControl.roll = roll;
  currentControl.pitch = pitch;
  currentControl.yaw = yaw;
  currentControl.throttle = throttle;
  currentControl.buttons = buttons;
  currentControl.lastUpdate = millis();

  // Log detalhado (a cada 10ª mensagem para não sobrecarregar serial)
  static int logCounter = 0;
  if (++logCounter >= 10) {
    Serial.printf("📡 UDP #%u | roll=%.2f pitch=%.2f yaw=%.2f thr=%.2f btn=%d\n",
                  seq, roll, pitch, yaw, throttle, buttons);
    logCounter = 0;
  }
}

/* ===== TASK UDP (Núcleo 1 - Recebe comandos) ===== */
void TaskUDP(void *pvParameters) {
  uint8_t buffer[UDP_BUFFER_SIZE];
  
  Serial.println("Task UDP iniciada no Núcleo 1");
  vTaskDelay(2000 / portTICK_PERIOD_MS); // Aguarda WiFi estabilizar

  for (;;) {
    int packetSize = udpServer.parsePacket();
    
    if (packetSize > 0) {
      int len = udpServer.read(buffer, UDP_BUFFER_SIZE);
      if (len > 0) {
        processUDPCommand(buffer, len);
      }
    }

    vTaskDelay(1 / portTICK_PERIOD_MS); // Yield para outras tasks
  }
}

/* ===== TASK PWM (Núcleo 1 - Processa motor mixing) ===== */
// Esta task roda em paralelo com TaskUDP
// Lê o estado de controle global e atualiza PWM dos motores a cada 10ms (~100 Hz)
void TaskPWM(void *pvParameters) {
  Serial.println("Task PWM iniciada no Núcleo 1");
  vTaskDelay(2000 / portTICK_PERIOD_MS); // Aguarda boot completo

  unsigned long lastLogTime = 0;
  const unsigned long LOG_INTERVAL = 500; // Log a cada 500ms
  
  for (;;) {
    // Calcula mixing dos motores baseado no controle atual
    calculateMotorMix(currentControl);
    
    // Escreve valores PWM nos pinos (atualiza em tempo real)
    updateMotorPWM();
    
    // Log detalhado periodicamente para debug
    unsigned long now = millis();
    if (now - lastLogTime >= LOG_INTERVAL) {
      Serial.printf("⚡ PWM: M1=%d M2=%d M3=%d M4=%d | thr=%.2f pit=%.2f rol=%.2f yaw=%.2f\n",
                    motorPWM.motor1, motorPWM.motor2, motorPWM.motor3, motorPWM.motor4,
                    currentControl.throttle, currentControl.pitch, currentControl.roll, currentControl.yaw);
      lastLogTime = now;
    }

    // Atualiza a 100 Hz (10ms de intervalo)
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// ===== SETUP WIFI =====
void setupWiFi() {
  Serial.println();
  Serial.print("Conectando em ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✓ WiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    
    // Pisca LED uma vez
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(200);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(200);
    
    // Imprime link da stream
    Serial.print("Stream: http://");
    Serial.print(WiFi.localIP().toString());
    Serial.println("/stream");
    
    wifiOk = true;
  } else {
    Serial.println("✗ Falha ao conectar WiFi");
    wifiOk = false;
    digitalWrite(STATUS_LED_PIN, LOW);
  }
}

// ===== TASK WIFI (Núcleo 0 - Monitora conexão) =====
void TaskWiFi(void *pvParameters) {
  vTaskDelay(5000 / portTICK_PERIOD_MS);  // Aguarda inicialização
  
  for (;;) {
    // Verifica conexão WiFi a cada 10s
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi desconectado, reconectando...");
      setupWiFi();
    }
    
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}

// ===== HANDLERS HTTP =====
void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
  <style>
    body { margin: 0; padding: 0; display: flex; justify-content: center; align-items: center; height: 100vh; background: #000; }
    img { transform: rotate(270deg); width: 100%; height: 100%; object-fit: contain; }
  </style>
</head>
<body>
  <img src="/stream">
</body>
</html>
  )";
  server.send(200, "text/html", html);
}

void handleStream() {
  if (!cameraOk) {
    server.send(500, "text/plain", "Camera not initialized");
    return;
  }
  
  WiFiClient client = server.client();
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendContent("HTTP/1.1 200 OK\r\n");
  server.sendContent("Content-Type: ");
  server.sendContent(STREAM_CONTENT_TYPE);
  server.sendContent("\r\n\r\n");
  
  // Stream loop
  while (client.connected()) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) continue;
    
    client.write((const uint8_t*)STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
    
    char part[64];
    int hlen = snprintf(part, sizeof(part), STREAM_PART, fb->len);
    client.write((const uint8_t*)part, hlen);
    
    client.write(fb->buf, fb->len);
    esp_camera_fb_return(fb);
    
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

/* ===== HANDLER STATUS - Estado Completo (Controle + Motores) ===== */
void handleStatus() {
  // Retorna JSON com estado de controle e PWM dos motores
  String json = "{";
  json += "\"control\":{";
  json += "\"roll\":" + String(currentControl.roll, 3) + ",";
  json += "\"pitch\":" + String(currentControl.pitch, 3) + ",";
  json += "\"yaw\":" + String(currentControl.yaw, 3) + ",";
  json += "\"throttle\":" + String(currentControl.throttle, 3) + ",";
  json += "\"buttons\":" + String(currentControl.buttons);
  json += "},";
  
  json += "\"motors\":{";
  json += "\"m1\":" + String(motorPWM.motor1) + ",";
  json += "\"m2\":" + String(motorPWM.motor2) + ",";
  json += "\"m3\":" + String(motorPWM.motor3) + ",";
  json += "\"m4\":" + String(motorPWM.motor4);
  json += "},";
  
  json += "\"system\":{";
  json += "\"lastUpdate\":" + String(currentControl.lastUpdate) + ",";
  json += "\"uptime\":" + String(millis()) + ",";
  json += "\"freeHeap\":" + String(ESP.getFreeHeap());
  json += "}";
  json += "}";
  
  server.send(200, "application/json", json);
}

/* ===== HANDLER DEBUG - Estado de Controle ===== */
void handleDebug() {
  // Retorna estado atual em JSON para debug
  String json = "{";
  json += "\"roll\":" + String(currentControl.roll, 3) + ",";
  json += "\"pitch\":" + String(currentControl.pitch, 3) + ",";
  json += "\"yaw\":" + String(currentControl.yaw, 3) + ",";
  json += "\"throttle\":" + String(currentControl.throttle, 3) + ",";
  json += "\"buttons\":" + String(currentControl.buttons) + ",";
  json += "\"lastUpdate\":" + String(currentControl.lastUpdate) + ",";
  json += "\"uptime\":" + String(millis());
  json += "}";
  
  server.send(200, "application/json", json);
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=== TAPEJARA BOOT ===");
  
  // LED
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  
  // PWM / Motores (antes de câmera e WiFi para evitar atrasos)
  initPWM();
  
  // Câmera
  Serial.println("Inicializando câmera...");
  if (initCamera()) {
    Serial.println("✓ Câmera iniciada");
    cameraOk = true;
  } else {
    Serial.println("✗ Falha ao inicializar câmera");
    cameraOk = false;
  }
  
  // WiFi (no setup, antes de WebServer)
  setupWiFi();
  
  // UDP (após WiFi conectar)
  setupUDP();
  
  // HTTP
  server.on("/", handleRoot);
  server.on("/stream", handleStream);
  server.on("/status", handleStatus);
  server.on("/debug", handleDebug);
  server.onNotFound(handleNotFound);
  server.begin();
  
  Serial.println("✓ Servidor HTTP iniciado (porta 80)");
  
  // Criar task para monitorar WiFi (Núcleo 0)
  xTaskCreatePinnedToCore(TaskWiFi, "WiFi_Monitor", 4096, NULL, 1, NULL, 0);
  
  // Criar task para receber UDP (Núcleo 1)
  xTaskCreatePinnedToCore(TaskUDP, "UDP_Receiver", 4096, NULL, 1, NULL, 1);
  
  // Criar task para processar PWM (Núcleo 1 - share com TaskUDP)
  xTaskCreatePinnedToCore(TaskPWM, "PWM_Controller", 4096, NULL, 2, NULL, 1);
  
  Serial.println("=== BOOT COMPLETO ===");
  Serial.print("Free Heap: ");
  Serial.println(ESP.getFreeHeap());
  Serial.println();
}

// ===== LOOP PRINCIPAL =====
void loop() {
  server.handleClient();
  vTaskDelay(1 / portTICK_PERIOD_MS);
}
