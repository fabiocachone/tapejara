#pragma once
// LedService.h
// Classe para controle do LED de estado (usa o flash LED da ESP32-CAM por padrão).
// Regras:
// - se não calibrado: LED ligado fixo
// - se calibrado: pisca 3 vezes e apaga

#include <Arduino.h>
#include "AppConfig.h"

// Classe LedService para gerenciar o estado do LED
class LedService {
public:
    // Inicializa o serviço de LED
    void begin() {
        pinMode(STATUS_LED_PIN, OUTPUT); // Configura o pino do LED como saída
        off(); // Garante que o LED comece desligado
    }

    // Liga o LED
    void on()  { write(true); }

    // Desliga o LED
    void off() { write(false); }

    // Faz o LED piscar um número específico de vezes
    void blink(uint8_t times, uint16_t onMs = 150, uint16_t offMs = 150) {
        for (uint8_t i = 0; i < times; i++) {
            on(); // Liga o LED
            delay(onMs); // Aguarda o tempo definido enquanto o LED está ligado
            off(); // Desliga o LED
            delay(offMs); // Aguarda o tempo definido enquanto o LED está desligado
        }
    }

private:
    // Função auxiliar para escrever o estado do LED
    void write(bool isOn) {
        // Define o estado do LED com base na configuração de ativo alto ou baixo
        digitalWrite(STATUS_LED_PIN, STATUS_LED_ACTIVE_HIGH ? (isOn ? HIGH : LOW)
                                                                                                             : (isOn ? LOW : HIGH));
    }
};
