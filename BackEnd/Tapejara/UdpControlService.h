#pragma once
// UdpControlService.h
// Recebe comandos via UDP e atualiza ControlState.
// O recebimento roda numa task separada para não travar o loop do HTTP.
//
// Formato esperado:
//   C,thr,yaw,pit,rol
// Exemplo:
//   C,100,-20,0,15

#include <Arduino.h>
#include <WiFiUdp.h>
#include "AppConfig.h"
#include "ControlState.h"

class UdpControlService {
public:
  // Inicia o UDP e cria a task de leitura contínua
  void begin(ControlState& st) {
    state = &st;

    // Abre socket UDP na porta definida
    udp.begin(UDP_PORT);

    // Cria task no core 1 para ficar lendo pacotes
    xTaskCreatePinnedToCore(
      taskTrampoline, // função estática que chama o método de instância
      "udpTask",
      4096,
      this,           // passa "this" como parâmetro
      2,
      nullptr,
      1
    );
  }

private:
  WiFiUDP udp;
  ControlState* state = nullptr;
  char buf[128];

  // Ponte: FreeRTOS exige função C-style; aqui redirecionamos para o objeto
  static void taskTrampoline(void* pv) {
    auto* self = static_cast<UdpControlService*>(pv);
    self->taskLoop();
  }

  // Loop principal do UDP: lê pacotes e processa
  void taskLoop() {
    for (;;) {
      int psize = udp.parsePacket();
      if (psize > 0) {
        int n = udp.read(buf, sizeof(buf) - 1);
        if (n > 0) {
          buf[n] = 0;     // termina string
          parseCmd(buf);  // interpreta comando
        }
      }
      vTaskDelay(1); // cede CPU
    }
  }

  // Parser do comando do app
  void parseCmd(const char* s) {
    if (!s || s[0] != 'C') return;

    int t, y, p, r;
    if (sscanf(s, "C,%d,%d,%d,%d", &t, &y, &p, &r) == 4) {
      // Normaliza para limites esperados
      state->thr = constrain(t, -1000, 1000);
      state->yaw = constrain(y, -1000, 1000);
      state->pit = constrain(p, -1000, 1000);
      state->rol = constrain(r, -1000, 1000);

      // Aqui você encaixa o controle real (mixer/ESC/PWM) quando chegar a hora
    }
  }
};
