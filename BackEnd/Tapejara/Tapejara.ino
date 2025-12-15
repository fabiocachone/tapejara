// Tapejara.ino
//
// DIAGRAMA DE FLUXO (ATUALIZE SE O FLUXO MUDAR)
//
//   [Android App] --(Wi-Fi conecta no AP "tapejara01")--> [ESP32-CAM AP: 192.168.4.1]
//        |                                                        |
//        |-- HTTP GET http://192.168.4.1/stream   (MJPEG vídeo) -->|
//        |-- HTTP GET http://192.168.4.1/status   (JSON debug) --> |--> inclui calibrated + valores de calibração + batteryPct
//        |-- HTTP GET http://192.168.4.1/calibrate?value=0|1&... ->|--> grava em SPIFFS (/config.txt) e controla LED
//        |
//        |-- UDP 192.168.4.1:4210  "C,thr,yaw,pit,rol" ----------> |--> [ControlState: thr/yaw/pit/rol]
//                                                                 |
//                                                                 +--> (futuro) Mixer/ESC/PWM etc.
//                                                                 +--> LED: se não calibrado fica ligado; se calibrado pisca 3x no boot/ao calibrar
//
// RESPONSABILIDADES (módulos)
// - CameraService: inicialização + captura de frames da câmera
// - WifiApService: criação do hotspot (AP) com IP fixo
// - HttpStreamService: rotas HTTP (/status, /stream, /calibrate)
// - UdpControlService: recepção UDP e parsing dos comandos
// - StorageService: SPIFFS + persistência (/config.txt) de calibrated + valores de calibração
// - LedService: LED de estado (não calibrado ligado; calibrado pisca 3x)
// - BatteryService: bateria (por enquanto hard-coded) exposta no /status e logada no Serial
// - ControlState: estado compartilhado thr/yaw/pit/rol
// - AppConfig: constantes (SSID, senha, portas, headers MJPEG, CONFIG_PATH, LED pin)

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
#include "LedService.h"
#include "BatteryService.h"

// Servidor HTTP que vai servir /status e /stream
WebServer server(80);

// Estado compartilhado dos controles (throttle/yaw/pitch/roll)
ControlState control;

// Serviços separados por responsabilidade
CameraService camera;       // Inicializa e captura frames da câmera
WifiApService wifiAp;       // Cria hotspot (AP) e IP fixo
StorageService storage;     // SPIFFS + config persistente
LedService led;             // LED de estado
BatteryService battery;     // Bateria (hard-coded por enquanto)
HttpStreamService http;     // Rotas HTTP e stream MJPEG
UdpControlService udpCtrl;  // Recebe comandos via UDP e atualiza control

static unsigned long lastBatteryLogMs = 0;

void setup() {
  Serial.begin(115200);

  led.begin();
  battery.begin();

  // Valor hard-coded por enquanto
  battery.setHardcoded(73);

  // 1) Inicializa câmera
  if (!camera.begin()) {
    Serial.println("Falha ao iniciar camera");
    while (true) delay(1000);
  }

  // 2) Monta SPIFFS e carrega/cria config
  if (!storage.begin(true)) {
    Serial.println("Falha ao montar SPIFFS");
    while (true) delay(1000);
  }

   // LOG: calibração carregada da flash (SPIFFS)
  const CalibrationData& c = storage.getCal();
  Serial.println("=== CALIBRATION (BOOT) ===");
  Serial.print("calibrated: ");
  Serial.println(c.calibrated ? "YES" : "NO");
  Serial.print("trimYaw: ");   Serial.println(c.trimYaw);
  Serial.print("trimPitch: "); Serial.println(c.trimPitch);
  Serial.print("trimRoll: ");  Serial.println(c.trimRoll);
  Serial.print("thrMin: ");    Serial.println(c.thrMin);
  Serial.print("thrMax: ");    Serial.println(c.thrMax);
  Serial.println("==========================");

  // 3) LED conforme estado persistido
  if (c.calibrated) {
    led.off();
    led.blink(3);
    led.off();
  } else {
    // led.on(); // fica ligado até calibrar
    led.off();
    led.blink(7);
    led.off();
  }

  // 4) Sobe Wi-Fi em modo Access Point (hotspot)
  wifiAp.begin();

  // 5) Sobe servidor HTTP com rotas /status /stream /calibrate
  http.begin(server, camera, control, storage, led, battery);

  // 6) Sobe receptor UDP (em task separada) para ler comandos do app
  udpCtrl.begin(control);

  // Logs para debug no Serial Monitor
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Rotas: /status, /stream, /calibrate");

  Serial.print("Battery (hard-coded): ");
  Serial.print(battery.getPercent());
  Serial.println("%");
}

void loop() {
  // Mantém o servidor HTTP respondendo requisições
  http.handle();

  // Log periódico da bateria no Serial (simula valor "dinâmico" por enquanto)
  const unsigned long now = millis();
  if (now - lastBatteryLogMs >= 5000) {
    lastBatteryLogMs = now;
    Serial.print("Battery: ");
    Serial.print(battery.getPercent());
    Serial.println("%");
  }

  // Respiro do loop
  delay(1);
}
