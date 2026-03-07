#pragma once
// WifiApService.h (agora STA - Station mode)
// Responsável por conectar a ESP32-CAM a uma rede WiFi existente (STA mode).

#include <WiFi.h>
#include "AppConfig.h"

class WifiApService {
public:
  void begin() {
    // Modo Station: o ESP se conecta a uma rede WiFi existente
    WiFi.mode(WIFI_STA);

    // Define IP fixo para melhor estabilidade (opcional)
    WiFi.config(WIFI_IP, WIFI_GATEWAY, WIFI_SUBNET, WIFI_DNS);

    // Conecta à rede WiFi
    Serial.print("\nConectando à rede WiFi: ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Aguarda conexão (máximo 20 segundos)
    int tentativas = 0;
    while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
      delay(500);
      Serial.print(".");
      tentativas++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConectado!");
      Serial.print("IP do ESP: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("\nFalha ao conectar à WiFi!");
    }
  }
};
