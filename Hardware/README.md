# Tapejara - Hardware

autor: Elisa Cristo

Descrição
--------
README inicial e simples para a parte de Hardware do drone Tapejara. Contém visão geral dos componentes, pinagens principais e recomendações para montagem e testes.

Componentes principais sugeridos
- Flight controller (ex.: Pixhawk, Matek F7, or similar)
- ESCs (Electronic Speed Controllers) compatíveis com motores
- Motores brushless (especificar KV conforme carga/prop)
- Hélices adequadas aos motores
- Power Distribution Board (PDB) ou cabo de distribuição
- Bateria LiPo (ex.: 3S/4S conforme projeto) com conector apropriado
- GPS + Compass (ex.: u-blox)
- Telemetria (telemetry radio / LTE / Wi‑Fi)
- ESP32-CAM (câmera a bordo) — ver seção de pinagem
- IMU (integrada ao flight controller)
- Conectores, fusíveis, cabos, suportes e parafusos

Pinagens e conexões principais (exemplo genérico)
- Motores -> ESCs -> PDB/VCC:
  - ESC sinal -> PWM outputs do flight controller (1..N)
  - ESC V+ -> PDB / Bateria
  - ESC GND -> GND comum
- Flight controller:
  - Power in: VCC (do PDB) e GND
  - SBUS/PPM/DSM -> receptor RC
  - Telemetria UART -> telemetry radio (TX/RX cruzado)
  - GPS -> UART I2C (TX/RX, +5V, GND)
- ESP32-CAM (exemplo de pinos usados):
  - 5V (ou 3.3V conforme alimentação do módulo) -> VCC
  - GND -> GND comum
  - RX/TX (opcional, para controle/config) -> UART secundário do FC ou microcontrolador
  - IO0/EN só para programação
  - Alimentação: preferir regulador dedicado (câmera pode causar ruído)
- Bateria:
  - Balance connector para carregador
  - Main leads -> PDB/ESC power input
  - BEC (se necessário) -> alimentar flight controller/esc 5V

Notas elétricas e de aterramento
- GND deve ser comum para todos módulos (FC, ESCs, ESP32-CAM, telemetria).
- Use capacitores e filtros perto dos ESCs para reduzir ruído.
- Proteja linhas de alimentação com fusíveis apropriados.
- Dimensione fios para corrente máxima esperada.

Placas e módulos recomendados
- Flight controller com suporte a integração de telemetria e múltiplos UARTs.
- ESCs com telemetria (opcional) para coleta de corrente/temperatura.
- Regulador 5V/3A mínimo para alimentar ESP32-CAM e periféricos.
- PDB com pontos de solda claros e montagem mecânica compatível.

Segurança e testes
- Testes no solo com hélices removidas ao verificar sinais PWM e rotas de alimentação.
- Verificar temperaturas dos ESCs e motores em primeiros voos.
- Checar rede de aterramento e leituras de tensão antes do voo.
- Implementar interrupção de emergência (kill switch) no hardware/software.

BoM mínima sugerida
- 1x Flight controller (Pixhawk/Matek)
- 4x ESCs compatíveis
- 4x Motores brushless (especificar KV)
- 1x PDB / Power distribution
- 1x Bateria LiPo (especificar C-rating)
- 1x ESP32-CAM
- 1x GPS/Compass
- Cabos, conectores, parafusos, suportes

Como começar
1. Definir configurações elétricas (tensão, corrente, BEC).
2. Mapear saídas PWM do FC para ESCs e testar sinal sem hélices.
3. Alimentar e configurar ESP32-CAM separadamente; validar stream.
4. Integrar ESP32-CAM com BackEnd (endereço/porta) e checar comunicação.
5. Fazer testes incrementais com checklists de segurança.

Documentação adicional
- Adicionar esquemas de pinagem detalhados em Hardware/schematics/
- Incluir fotos ou diagramas de montagem em Hardware/docs/

Licença
- Defina a licença do hardware/documentação no arquivo raiz (LICENSE).