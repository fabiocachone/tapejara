# Tapejara - BackEnd (ESP32-CAM Firmware)

autor: Fabio Cachone

Descrição
---------
Este diretório contém o firmware que roda diretamente na ESP32-CAM e implementa vídeo, telemetria, controle e persistência de calibração para o projeto Tapejara.

Responsabilidades principais do firmware
- Expor stream MJPEG via HTTP (`/stream`).
- Expor estado/telemetria via HTTP (`/status`).
- Receber comandos de controle via UDP (`porta 4210`).
- Persistir e recuperar configuração de calibração em SPIFFS (`/config.txt`).

Arquivos chave
- [Tapejara/Tapejara.ino](Tapejara/Tapejara.ino) — sketch principal (setup + loop).
- [Tapejara/AppConfig.h](Tapejara/AppConfig.h) — constantes (SSID, senha, IP do AP, portas, paths).
- [Tapejara/CameraService.h](Tapejara/CameraService.h) — inicialização e captura de frames JPEG.
- [Tapejara/WifiApService.h](Tapejara/WifiApService.h) — cria o Access Point com IP fixo.
- [Tapejara/HttpStreamService.h](Tapejara/HttpStreamService.h) — implementa `/stream`, `/status`, `/calibrate`.
- [Tapejara/UdpControlService.h](Tapejara/UdpControlService.h) — task UDP que atualiza `ControlState`.
- [Tapejara/StorageService.h](Tapejara/StorageService.h) — SPIFFS e persistência de calibração (`/config.txt`).
- [Tapejara/LedService.h](Tapejara/LedService.h) — gerencia LED de status (pisca / ligado).
- [Tapejara/BatteryService.h](Tapejara/BatteryService.h) — placeholder para % de bateria.

Fluxo de uso (passo a passo)
1. Gravar o firmware na ESP32-CAM (ver seção "Como compilar e subir").
2. Ao ligar, a ESP32-CAM cria um Access Point `tapejara01` (senha em `AppConfig.h`).
3. Conecte o celular ou PC no AP `tapejara01` e acesse o IP fixo `http://192.168.4.1`.
4. Para vídeo em tempo real, o app faz `HTTP GET http://192.168.4.1/stream` (MJPEG).
5. Para checar estado/telemetria: `HTTP GET http://192.168.4.1/status` (JSON).
6. Para ajustar/calibrar: `HTTP GET http://192.168.4.1/calibrate?value=1&...`.
7. Para enviar comandos de controle em tempo real, envie UDP para `192.168.4.1:4210` com o payload `C,thr,yaw,pit,rol`.

Protocolos e formatos
- Vídeo: HTTP MJPEG multipart (`multipart/x-mixed-replace;boundary=frame`).
   - Endpoint: `/stream`
   - Implementação: `HttpStreamService` captura JPEGs via `CameraService` e escreve partes com boundary `--frame`.
- Telemetria / Status: HTTP JSON.
   - Endpoint: `/status`.
   - Conteúdo: IP, `thr`, `yaw`, `pit`, `rol`, flags de calibração, trims, `thrMin`/`thrMax`, `batteryPct`.
- Calibração: HTTP query string para `/calibrate`.
   - Parâmetro obrigatório: `value=0|1`.
   - Opcionais: `trimYaw`, `trimPitch`, `trimRoll`, `thrMin`, `thrMax`.
   - Persiste em SPIFFS (`StorageService`).
- Controle: UDP datagram.
   - Porta: `4210` (ver `AppConfig.h`).
   - Formato: ASCII `C,thr,yaw,pit,rol` (ex.: `C,100,-20,0,15`).
   - Parse: `UdpControlService` faz `sscanf` e aplica `constrain` antes de atualizar `ControlState`.

Como compilar e subir o firmware
- Arduino IDE (recomendado para iniciantes):
   1. Abra `BackEnd/Tapejara/Tapejara.ino` no Arduino IDE.
   2. Selecione a placa `AI Thinker ESP32-CAM` (ou a placa correspondente).
   3. Configure a porta serial correta com o conversor FTDI/USB-to-Serial.
   4. Pressione `Upload`.

- PlatformIO (avançado):
   1. Configure um `platformio.ini` com ambiente ESP32 (use o board correspondente ao seu módulo).
   2. Execute `platformio run --target upload`.

Observações antes de gravar
- Confirme a pinagem em `CameraService.h` se sua placa for diferente do AI-Thinker.
- Ajuste `STATUS_LED_PIN` e `STATUS_LED_ACTIVE_HIGH` em `AppConfig.h` se necessário.

Exemplos práticos de teste
- Abrir stream em navegador / WebView:
   - http://192.168.4.1/stream

- Ver status via curl:
   - `curl http://192.168.4.1/status`

- Ativar calibragem e ajustar trims:
   - `http://192.168.4.1/calibrate?value=1&trimYaw=5&trimPitch=-2`

- Enviar comando UDP (Linux/macOS com `ncat`):
   - `echo -n "C,100,0,0,0" | ncat -u 192.168.4.1 4210`

- Enviar comando UDP (PowerShell no Windows):
   - `$udp = New-Object System.Net.Sockets.UdpClient; $b=[System.Text.Encoding]::ASCII.GetBytes("C,100,0,0,0"); $udp.Send($b,$b.Length,"192.168.4.1",4210)`

Fluxograma (visão geral)

   [App/Navegador]
            |
            | conecta Wi‑Fi AP `tapejara01` -> IP `192.168.4.1`
            v
   [ESP32-CAM]
      |- HTTP GET /stream  --> MJPEG (CameraService -> HttpStreamService)
      |- HTTP GET /status  --> JSON (HttpStreamService -> Storage/Battery)
      |- HTTP GET /calibrate?value=... --> grava em SPIFFS (StorageService)
      |- UDP recv on 4210  --> atualiza `ControlState` (UdpControlService)