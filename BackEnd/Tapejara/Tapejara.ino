// Tapejara.ino
//
// DIAGRAMA DE FLUXO (ATUALIZE SE O FLUXO MUDAR)
//
//   [Android App] --(Wi-Fi conecta no AP "tapejara01")--> [ESP32-CAM AP: 192.168.4.1]
//        |                                                        |
//        |-- HTTP GET http://192.168.4.1/stream   (MJPEG vídeo) -->|
//        |-- HTTP GET http://192.168.4.1/status   (JSON debug) --> |--> inclui "calibrated"
//        |-- HTTP GET http://192.168.4.1/calibrate?value=0|1 ----->|--> grava flag em SPIFFS (/config.txt)
//        |
//        |-- UDP 192.168.4.1:4210  "C,thr,yaw,pit,rol" ----------> |--> [ControlState: thr/yaw/pit/rol]
//                                                                 |
//                                                                 +--> (futuro) Mixer/ESC/PWM etc.
//
// RESPONSABILIDADES (módulos)
// - CameraService: inicialização + captura de frames da câmera
// - WifiApService: criação do hotspot (AP) com IP fixo
// - HttpStreamService: rotas HTTP (/status, /stream, /calibrate)
// - UdpControlService: recepção UDP e parsing dos comandos
// - StorageService: SPIFFS + persistência (/config.txt)
// - ControlState: estado compartilhado thr/yaw/pit/rol
// - AppConfig: constantes (SSID, senha, portas, headers MJPEG)

#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include "esp_camera.h"

// Configurações e módulos do projeto
#include "AppConfig.h"
#include "ControlState.h"
#include "CameraService.h"
#include "WifiApService.h"
#include "HttpStreamService.h"
#include "UdpControlService.h"
#include "StorageService.h"

// Servidor HTTP que vai servir /status e /stream
WebServer server(80);

// Estado compartilhado dos controles (throttle/yaw/pitch/roll)
ControlState control;

// Serviços separados por responsabilidade
CameraService camera;       // Inicializa e captura frames da câmera
WifiApService wifiAp;       // Cria hotspot (AP) e IP fixo
StorageService storage;     // SPIFFS e persistência de configurações
HttpStreamService http;     // Rotas HTTP e stream MJPEG
UdpControlService udpCtrl;  // Recebe comandos via UDP e atualiza control

void setup() {
  Serial.begin(115200);

  // 1) Inicializa câmera
  if (!camera.begin()) {
    Serial.println("Falha ao iniciar camera");
    while (true) delay(1000);
  }

  // 2) Monta SPIFFS e carrega/cria config persistente
  if (!storage.begin(true)) {
    Serial.println("Falha ao montar SPIFFS");
    while (true) delay(1000);
  }

  // 3) Sobe Wi-Fi em modo Access Point (hotspot)
  wifiAp.begin();

  // 4) Sobe servidor HTTP com rotas /status, /stream e /calibrate
  http.begin(server, camera, control, storage);

  // 5) Sobe receptor UDP (em task separada) para ler comandos do app
  udpCtrl.begin(control);

  // Logs para debug no Serial Monitor
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Rotas: /status, /stream, /calibrate");
}

void loop() {
  // Mantém o servidor HTTP respondendo requisições
  http.handle();

  // Respiro do loop
  delay(1);
}
