#pragma once
// ControlState.h
// Estrutura simples para compartilhar o "estado de controle" entre serviços.
// Aqui ficam as variáveis que o UDP atualiza e o restante do projeto pode usar.

#include <Arduino.h>

struct ControlState {
  // volatile: evita otimizações indevidas, pois esses valores são atualizados por task (UDP)
  volatile int thr = 0; // throttle (aceleração / potência)
  volatile int yaw = 0; // yaw (giro no eixo vertical)
  volatile int pit = 0; // pitch (inclinação frente/trás)
  volatile int rol = 0; // roll (inclinação esquerda/direita)
};
