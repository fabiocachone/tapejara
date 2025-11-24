# Tapejara - Telemetria dos Motores

autor: Nicolas Benitez

Descrição
--------
README inicial para a camada de telemetria responsável por coletar, processar e expor dados dos motores do drone Tapejara.

Responsabilidades principais
- Coletar métricas dos drivers/controladores dos motores (ESCs).
- Processar e normalizar dados (filtragem, média, detecção de anomalias).
- Disponibilizar telemetria em tempo real para o BackEnd/FrontEnd (API/WS/MQTT).
- Persistir dados históricos para diagnóstico (opcional).

Métricas sugeridas
- RPM (rotações por minuto) por motor
- Corrente (A) por motor
- Tensão (V) do supply
- Temperatura (°C) do motor/ESC
- Sinal PWM/ESC (ms ou duty)
- Estado (ativo, falha, limite térmico)

Formato de dados (exemplo JSON)
```json
{
  "timestamp": "2025-11-24T12:34:56.789Z",
  "motors": [
    { "id": 1, "rpm": 4200, "current": 12.3, "voltage": 11.7, "temp": 58.2, "pwm": 1500, "status": "ok" },
    { "id": 2, "rpm": 4185, "current": 12.1, "voltage": 11.7, "temp": 57.9, "pwm": 1500, "status": "ok" }
  ]
}
```

Interfaces recomendadas
- REST GET /telemetry/latest — último snapshot
- WS /telemetry/ws — stream em tempo real (preferível para baixa latência)
- MQTT topic tapejara/telemetry/motors — integração com brokers
- Endpoint para histórico: GET /telemetry?from=...&to=...

Estrutura sugerida
- src/
  - collectors/    — drivers e adaptadores (CAN, serial, I2C, ADC)
  - processors/    — agregação, filtros, detecção de falhas
  - api/           — REST/WS/MQTT exposição
  - storage/       — persistência (InfluxDB, Timescale, ou arquivos)
  - config/        — leitura de variáveis de ambiente
  - tests/         — testes unitários e de integração

Configuração (.env exemplo)
- TELEMETRY_PORT=4000
- TELEMETRY_WS_PORT=4001
- STORAGE_URL=http://localhost:8086
- COLLECTOR_INTERFACE=/dev/ttyUSB0
- SAMPLE_RATE_HZ=10

Como começar (exemplo genérico Node.js)
1. Instalar dependências:
   - npm install
2. Configurar variáveis (criar .env a partir de .env.example)
3. Rodar em desenvolvimento:
   - npm run dev
4. Testar:
   - GET http://localhost:4000/telemetry/latest
   - conectar WebSocket em ws://localhost:4001

Boas práticas
- Validar e limitar taxa de amostragem para não saturar canal.
- Implementar buffer e reconexão para perda temporária de coletor.
- Alarmes para limites críticos (corrente/temperatura).
- Sincronizar timestamps (NTP) para correlação com logs de voo.

Contribuição
- Use branches por feature e PRs revisados.
- Documente mudanças no protocolo de telemetria.
- Adicione testes para coleta e processamento de métricas.

Licença
- Defina a licença do projeto no arquivo raiz (LICENSE).