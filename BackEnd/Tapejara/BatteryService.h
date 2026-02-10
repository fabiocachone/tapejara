#pragma once
// BatteryService.h
// Placeholder de bateria.
// Por enquanto devolve um valor fixo (hard-coded).
// Depois a gente troca para leitura real via ADC e mapeamento para 0..100%.

#include <Arduino.h>

class BatteryService {
public:
  void begin() {
    // Futuro: configurar ADC / pino / atenuação
  }

  // Retorna a bateria em % (0..100)
  int getPercent() const { return batteryPct; }

  // Define manualmente (hard-coded) com clamp
  void setHardcoded(int pct) {
    batteryPct = constrain(pct, 0, 100);
  }

private:
  int batteryPct = 73; // valor fixo inicial
};
