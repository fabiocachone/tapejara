#pragma once
// AppConfig.h
// Centraliza configurações fixas: SSID, senha, IP do AP, porta UDP e constantes do MJPEG.

#include <Arduino.h>
#include <IPAddress.h>

// ---------- Wi-Fi AP ----------
// SSID e senha do hotspot criado pela ESP32-CAM
static const char* AP_SSID = "tapejara01";
static const char* AP_PASS = "tpj@ufabc25";

// IP fixo do Access Point: o app vai acessar http://192.168.4.1/...
static const IPAddress AP_IP(192,168,4,1);
static const IPAddress AP_MASK(255,255,255,0);

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
