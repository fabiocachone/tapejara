#pragma once
// AppConfig.h
// Centraliza configurações fixas: SSID, senha, IP fixo, porta UDP e constantes do MJPEG.

#include <Arduino.h>
#include <IPAddress.h>

// ---------- Wi-Fi STA (Station) ----------
// SSID e senha da rede WiFi à qual o ESP32-CAM se conectará
static const char* WIFI_SSID = "CACHONE";
static const char* WIFI_PASS = "f22485910";

// IP fixo do ESP em STA mode (opcional, útil para acesso estável)
// Se comentar, o ESP pedirá IP via DHCP
static const IPAddress WIFI_IP(192,168,1,100);
static const IPAddress WIFI_GATEWAY(192,168,1,1);
static const IPAddress WIFI_SUBNET(255,255,255,0);
static const IPAddress WIFI_DNS(8,8,8,8);

// ---------- UDP ----------
// Porta onde a ESP32-CAM escuta comandos do app (Datagram/UDP)
static const uint16_t UDP_PORT = 4210;

// ---------- Stream MJPEG ----------
// Cabeçalho do stream multipart (um JPEG atrás do outro)
static const char* STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=frame";

// Separador entre frames (boundary)
static const char* STREAM_BOUNDARY = "\r\n--frame\r\n";

// Cabeçalho de cada frame informando o Content-Length
static const char* STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// Adiciona o caminho do arquivo de configuração (SPIFFS)
static const char* CONFIG_PATH = "/config.txt";

// LED de status (na ESP32-CAM normalmente é o flash em GPIO 4)
// Se a sua placa for diferente, mude aqui.
static const uint8_t STATUS_LED_PIN = 4;
static const bool STATUS_LED_ACTIVE_HIGH = true; // na maioria das ESP32-CAM é true