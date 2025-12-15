// Tapejara.ino
//
// DIAGRAMA DE FLUXO (ATUALIZE SE O FLUXO MUDAR)
//
//   [Android App] --(Wi-Fi conecta no AP "tapejara01")--> [ESP32-CAM AP: 192.168.4.1]
//        |                                                        |
//        |-- HTTP GET http://192.168.4.1/stream  (MJPEG vídeo) --> |
//        |-- HTTP GET http://192.168.4.1/status  (JSON debug) -->  |
//        |
//        |-- UDP 192.168.4.1:4210  "C,thr,yaw,pit,rol" ----------> |--> [ControlState: thr/yaw/pit/rol]
//                                                                 |
//                                                                 +--> (futuro) Mixer/ESC/PWM etc.
//
// RESPONSABILIDADES (módulos)
// - CameraService: inicialização + captura de frames da câmera
// - WifiApService: criação do hotspot (AP) com IP fixo
// - HttpStreamService: rotas HTTP (/status e /stream)
// - UdpControlService: recepção UDP e parsing dos comandos
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

// Servidor HTTP que vai servir /status e /stream
WebServer server(80);

// Estado compartilhado dos controles (throttle/yaw/pitch/roll)
ControlState control;

// Serviços separados por responsabilidade
CameraService camera;       // Inicializa e captura frames da câmera
WifiApService wifiAp;       // Cria hotspot (AP) e IP fixo
HttpStreamService http;     // Rotas HTTP e stream MJPEG
UdpControlService udpCtrl;  // Recebe comandos via UDP e atualiza control

void setup() {
  Serial.begin(115200);

  // 1) Inicializa câmera
  if (!camera.begin()) {
    Serial.println("Falha ao iniciar camera");
    while (true) delay(1000);
  }

  // 2) Sobe Wi-Fi em modo Access Point (hotspot)
  wifiAp.begin();

  // 3) Sobe servidor HTTP com rotas /status e /stream
  http.begin(server, camera, control);

  // 4) Sobe receptor UDP (em task separada) para ler comandos do app
  udpCtrl.begin(control);

  // Logs para debug no Serial Monitor
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Rotas: /status e /stream");
}

void loop() {
  // Mantém o servidor HTTP respondendo requisições
  http.handle();

  // Respiro do loop
  delay(1);
}
