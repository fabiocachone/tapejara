// Tapejara.ino - Versão Simplificada com FreeRTOS
// Conexão WiFi à rede CACHONE + Stream MJPEG

#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"

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

// ===== VARIÁVEIS GLOBAIS =====
WebServer server(80);
bool cameraOk = false;
bool wifiOk = false;

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
  
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count     = 2;
  
  if (esp_camera_init(&config) != ESP_OK) {
    return false;
  }
  
  // Configura rotação 90 graus no sentido horário
  sensor_t * s = esp_camera_sensor_get();
  if (s != NULL) {
    s->set_vflip(s, 1);      // Flip vertical
    s->set_hmirror(s, 1);    // Mirror horizontal
  }
  
  return true;
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
    wifiOk = true;
    digitalWrite(STATUS_LED_PIN, LOW);
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
  <title>Tapejara Stream</title>
  <meta charset="UTF-8">
</head>
<body>
  <h1>Tapejara - Video Stream</h1>
  <p>ESP32-CAM conectado à rede CACHONE</p>
  <p><strong>IP: )";
  html += WiFi.localIP().toString();
  html += R"(</strong></p>
  <hr>
  <h2>Stream MJPEG:</h2>
  <img src="/stream" style="width:100%; max-width:600px;">
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

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=== TAPEJARA BOOT ===");
  
  // LED
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  
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
  
  // HTTP
  server.on("/", handleRoot);
  server.on("/stream", handleStream);
  server.onNotFound(handleNotFound);
  server.begin();
  
  Serial.println("✓ Servidor HTTP iniciado (porta 80)");
  
  // Criar task para monitorar WiFi (Núcleo 0)
  xTaskCreatePinnedToCore(TaskWiFi, "WiFi_Monitor", 4096, NULL, 1, NULL, 0);
  
  Serial.println("=== BOOT COMPLETO ===\n");
}

// ===== LOOP PRINCIPAL =====
void loop() {
  server.handleClient();
  vTaskDelay(1 / portTICK_PERIOD_MS);
}
