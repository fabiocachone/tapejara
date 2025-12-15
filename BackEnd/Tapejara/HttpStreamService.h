#pragma once
// HttpStreamService.h
// Cuida do servidor HTTP:
// - /status: retorna JSON com estado do controle
// - /stream: entrega MJPEG contínuo
//
// Observação: /stream é um loop contínuo enquanto o cliente estiver conectado.
// Isso “ocupa” aquela conexão, o que é esperado em MJPEG.

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include "AppConfig.h"
#include "CameraService.h"
#include "ControlState.h"

class HttpStreamService {
public:
  // Registra rotas e inicia o servidor
  void begin(WebServer& srv, CameraService& cam, ControlState& st) {
    server = &srv;
    camera = &cam;
    state  = &st;

    // /status: JSON com o estado atual
    server->on("/status", HTTP_GET, [this]() { this->handleStatus(); });

    // /stream: MJPEG contínuo
    server->on("/stream", HTTP_GET, [this]() { this->handleStream(); });

    server->begin();
  }

  // Deve ser chamado no loop para o WebServer processar requisições
  void handle() {
    if (server) server->handleClient();
  }

private:
  WebServer* server = nullptr;
  CameraService* camera = nullptr;
  ControlState* state = nullptr;

  // Responde um JSON simples para debug/monitoramento
  void handleStatus() {
    String json = "{";
    json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
    json += "\"thr\":" + String(state->thr) + ",";
    json += "\"yaw\":" + String(state->yaw) + ",";
    json += "\"pit\":" + String(state->pit) + ",";
    json += "\"rol\":" + String(state->rol);
    json += "}";
    server->send(200, "application/json", json);
  }

  // Stream MJPEG: entrega uma sequência de JPEGs com boundary
  void handleStream() {
    WiFiClient client = server->client();

    // Cabeçalhos HTTP
    server->sendHeader("Access-Control-Allow-Origin", "*");
    server->sendContent("HTTP/1.1 200 OK\r\n");
    server->sendContent("Content-Type: ");
    server->sendContent(STREAM_CONTENT_TYPE);
    server->sendContent("\r\n\r\n");

    // Loop do stream: enquanto o cliente estiver conectado, envia frames
    while (client.connected()) {
      camera_fb_t* fb = camera->capture();
      if (!fb) continue;

      // Boundary entre frames
      client.write(STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));

      // Header do frame (Content-Length)
      char part[64];
      int hlen = snprintf(part, sizeof(part), STREAM_PART, fb->len);
      client.write((const uint8_t*)part, hlen);

      // Dados JPEG
      client.write(fb->buf, fb->len);

      // Libera buffer do driver
      camera->release(fb);

      // Pequeno respiro: ajuda a não “engasgar”
      delay(5);
    }
  }
};
