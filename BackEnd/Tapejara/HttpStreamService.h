#pragma once
// HttpStreamService.h
// - /status: JSON (inclui calibrated e valores de calibração)
// - /stream: MJPEG
// - /calibrate: grava calibrated e (opcionalmente) valores de calibração em SPIFFS
//
// /calibrate aceita:
//   value=0|1   (obrigatório)
//   trimYaw=INT (opcional)
//   trimPitch=INT (opcional)
//   trimRoll=INT (opcional)
//   thrMin=INT (opcional)
//   thrMax=INT (opcional)

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include "AppConfig.h"
#include "CameraService.h"
#include "ControlState.h"
#include "StorageService.h"
#include "LedService.h"

class HttpStreamService {
public:
  void begin(WebServer& srv, CameraService& cam, ControlState& st, StorageService& store, LedService& ledSvc) {
    server = &srv;
    camera = &cam;
    state  = &st;
    storage = &store;
    led = &ledSvc;

    // /status: JSON com o estado atual
    server->on("/status", HTTP_GET, [this]() { this->handleStatus(); });

    // /stream: MJPEG contínuo
    server->on("/stream", HTTP_GET, [this]() { this->handleStream(); });

    // seta calibrado (persistente)
    server->on("/calibrate", HTTP_GET, [this]() { this->handleCalibrate(); });

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
  StorageService* storage = nullptr;
  LedService* led = nullptr;

  // Responde um JSON simples para debug/monitoramento
  void handleStatus() {
    const CalibrationData& c = storage->getCal();

    String json = "{";
    json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
    json += "\"thr\":" + String(state->thr) + ",";
    json += "\"yaw\":" + String(state->yaw) + ",";
    json += "\"pit\":" + String(state->pit) + ",";
    json += "\"rol\":" + String(state->rol) + ",";
    json += "\"calibrated\":" + String(c.calibrated ? 1 : 0) + ",";
    json += "\"trimYaw\":" + String(c.trimYaw) + ",";
    json += "\"trimPitch\":" + String(c.trimPitch) + ",";
    json += "\"trimRoll\":" + String(c.trimRoll) + ",";
    json += "\"thrMin\":" + String(c.thrMin) + ",";
    json += "\"thrMax\":" + String(c.thrMax);
    json += "}";
    server->send(200, "application/json", json);
  }

  void handleCalibrate() {
    server->sendHeader("Access-Control-Allow-Origin", "*");

    if (!server->hasArg("value")) {
      server->send(400, "application/json", "{\"ok\":0,\"err\":\"missing value=0|1\"}");
      return;
    }

    String v = server->arg("value");
    v.trim();
    bool flag = (v == "1" || v.equalsIgnoreCase("true"));

    // Atualiza apenas o que vier na query (o resto mantém)
    CalibrationData& c = storage->editCal();
    c.calibrated = flag;

    if (server->hasArg("trimYaw"))   c.trimYaw = server->arg("trimYaw").toInt();
    if (server->hasArg("trimPitch")) c.trimPitch = server->arg("trimPitch").toInt();
    if (server->hasArg("trimRoll"))  c.trimRoll = server->arg("trimRoll").toInt();
    if (server->hasArg("thrMin"))    c.thrMin = server->arg("thrMin").toInt();
    if (server->hasArg("thrMax"))    c.thrMax = server->arg("thrMax").toInt();

    if (!storage->save()) {
      server->send(500, "application/json", "{\"ok\":0,\"err\":\"save failed\"}");
      return;
    }

    // LED conforme regra
    if (led) {
      if (flag) {
        led->off();
        led->blink(3);
        led->off();
      } else {
        led->on(); // mantém ligado até calibrar
      }
    }

    String json = "{\"ok\":1,\"calibrated\":";
    json += (flag ? "1" : "0");
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
