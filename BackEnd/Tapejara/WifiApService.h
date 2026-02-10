#pragma once
// WifiApService.h
// Responsável por configurar a ESP32-CAM como Access Point (hotspot) com IP fixo.

#include <WiFi.h>
#include "AppConfig.h"

class WifiApService {
public:
  void begin() {
    // Modo Access Point: o celular conecta direto na ESP
    WiFi.mode(WIFI_AP);

    // Define IP fixo do AP (gateway e IP do próprio AP são o mesmo)
    WiFi.softAPConfig(AP_IP, AP_IP, AP_MASK);

    // Cria o hotspot no canal 6, sem ocultar SSID, máximo 1 cliente (ajuste se precisar)
    WiFi.softAP(AP_SSID, AP_PASS, 6, 0, 1);
  }
};
