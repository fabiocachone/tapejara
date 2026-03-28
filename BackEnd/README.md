# Tapejara - BackEnd (ESP32-CAM Firmware)

**autor**: Fabio Cachone

## Descrição

Firmware para **ESP32-CAM (AI Thinker)** que implementa:
- 📹 **Stream MJPEG** em tempo real via HTTP (`/stream`)
- 🎮 **Recepção de comandos de controle** via UDP (porta 9000)
- 🔍 **Endpoint de debug** para monitorar estado (`/debug`)
- 📡 **Monitoramento automático de WiFi** com reconexão (FreeRTOS)
- 🖼️ **Tela web responsiva** com vídeo full-screen rotacionado

## Arquivos principais

- **[Tapejara.ino](Tapejara/Tapejara.ino)** — Sketch único que contém:
  - Inicialização da câmera
  - Setup WiFi e HTTP
  - Servidor UDP para comandos
  - Tasks FreeRTOS (WiFi Monitor, UDP Receiver)
  - Handlers HTTP (/, /stream, /debug)

## Fluxo de uso (passo a passo)

1. **Grave o firmware** na ESP32-CAM via Arduino IDE ou PlatformIO
2. **Ao ligar**, a placa se conecta à rede WiFi `CACHONE` (credenciais em `Tapejara.ino`)
3. **Abre um servidor HTTP** na porta 80 e UDP na porta 9000
4. **App/Navegador** se conecta à mesma rede WiFi
5. **HTTP GET `/`** — Exibe vídeo em tela cheia
6. **HTTP GET `/stream`** — MJPEG bruto (para WebView, VLC, etc)
7. **HTTP GET `/debug`** — JSON com estado actual dos controles
8. **UDP porta 9000** — Recebe comandos de controle do app Android

## Protocolos e formatos

### 📹 HTTP - Stream de Vídeo (`/stream`)

**Endpoint:** `GET http://<ESP_IP>/stream`

**Content-Type:** `multipart/x-mixed-replace;boundary=frame`

**Formato:** MJPEG (Motion JPEG) com boundary frames

**Exemplo com curl:**
```bash
curl http://192.168.4.1/stream -o stream.mjpeg
```

**Resolução e qualidade:**
- Resolução: UXGA (1600×1200) — máxima suportada
- JPEG Quality: 12 (alta compressão, boa performance)
- Frame Rate: ~20 FPS (depende da largura de banda)
- Rotação: 270° no cliente (CSS transform)

---

### 🌐 HTTP - Página HTML (`/`)

**Endpoint:** `GET http://<ESP_IP>/`

**Content-Type:** `text/html`

**Conteúdo:**
```html
<!DOCTYPE html>
<html>
<body style="margin:0; background:#000; display:flex; justify-content:center; align-items:center; height:100vh;">
  <img src="/stream" style="transform:rotate(270deg); width:100%; object-fit:contain;">
</body>
</html>
```

**Uso:** Abrir em navegador, WebView ou player MJPEG
```
http://192.168.4.1/
```

---

### 🐛 HTTP - Debug (Estado de Controle) (`/debug`)

**Endpoint:** `GET http://<ESP_IP>/debug`

**Content-Type:** `application/json`

**Response:**
```json
{
  "roll": 0.450,
  "pitch": -0.230,
  "yaw": 0.120,
  "throttle": 0.680,
  "buttons": 0,
  "lastUpdate": 125430,
  "uptime": 125435
}
```

**Campos:**
- `roll` — Rolagem (float, -1.0 a +1.0)
- `pitch` — Arfagem (float, -1.0 a +1.0)
- `yaw` — Guinada (float, -1.0 a +1.0)
- `throttle` — Altitude (float, 0.0 a 1.0)
- `buttons` — Bitmask de botões (int)
- `lastUpdate` — Timestamp (ms) do último comando UDP
- `uptime` — Uptime da placa (ms)

**Exemplo com curl:**
```bash
curl http://192.168.4.1/debug | jq .
```

**Uso:** Monitorar em tempo real se os comandos UDP estão sendo recebidos

---

### 🎮 UDP - Comandos de Controle (porta 9000)

**Porta:** 9000 (UDP)

**Tamanho:** 24 bytes (little-endian)

**Formato do pacote:**
```
Byte  Campo           Tipo        Descrição
─────────────────────────────────────────────────────
0-3   magic           uint8[4]    0xED, 0x01, 0x00, 0x00
4-5   sequence        uint16_le   Número de sequência
6-9   roll            float_le    Rolagem (-1.0 a +1.0)
10-13 pitch           float_le    Arfagem (-1.0 a +1.0)
14-17 yaw             float_le    Guinada (-1.0 a +1.0)
18-21 throttle        float_le    Altitude (0.0 a 1.0)
22-23 buttons         uint16_le   Bitmask de botões
```

**Total:** 24 bytes

**Exemplo de envio (Linux/macOS com Python):**
```python
import socket
import struct

# Dados de controle
roll = 0.45
pitch = -0.23
yaw = 0.12
throttle = 0.68
buttons = 0
seq = 1

# Montar pacote
packet = struct.pack('<BBBBHHFFFF H',
    0xED, 0x01, 0x00, 0x00,  # Magic
    seq,                       # Sequence
    roll, pitch, yaw, throttle, # Controles
    buttons                    # Buttons
)

# Enviar
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.sendto(packet, ("192.168.4.1", 9000))
sock.close()
```

**Exemplo com netcat (ncat):**
```bash
# Criar arquivo binário com os dados
python3 -c "
import struct, sys
pkt = struct.pack('<BBBBHHFFFF H', 0xED, 0x01, 0, 0, 1, 0.45, -0.23, 0.12, 0.68, 0)
sys.stdout.buffer.write(pkt)
" | ncat -u 192.168.4.1 9000
```

**Validação no ESP32:**
- Magic bytes devem ser `0xED 0x01 0x00 0x00`
- Se inválido: log `⚠ Magic bytes inválidos` e descarta
- Se pacote < 24 bytes: log `⚠ Pacote UDP muito curto` e descarta

**Confirmação:**
- Log a cada 10ª mensagem: `📡 UDP #1 | roll=0.45 pitch=-0.23 yaw=0.12 thr=0.68 btn=0`

---

### ⚙️ HTTP - Status Completo (Controles + Motores) (`/status`)

**Endpoint:** `GET http://<ESP_IP>/status`

**Content-Type:** `application/json`

**Response:**
```json
{
  "control": {
    "roll": 0.450,
    "pitch": -0.230,
    "yaw": 0.120,
    "throttle": 0.680,
    "buttons": 0
  },
  "motors": {
    "m1": 1340,
    "m2": 1288,
    "m3": 1412,
    "m4": 1360
  },
  "system": {
    "lastUpdate": 125430,
    "uptime": 125435,
    "freeHeap": 145632
  }
}
```

**Campos:**
- **control** — Estado de entrada do joystick/app
  - `roll`, `pitch`, `yaw` (float, -1.0 a +1.0)
  - `throttle` (float, 0.0 a 1.0)
  - `buttons` (bitmask)
  
- **motors** — Saída PWM dos motores (microsegundos)
  - `m1` (FL - Front-Left): Motor frente-esquerdo
  - `m2` (FR - Front-Right): Motor frente-direito
  - `m3` (BL - Back-Left): Motor trás-esquerdo
  - `m4` (BR - Back-Right): Motor trás-direito
  - Range: 1000-2000 µs (MIN-MAX PWM)

- **system** — Info do sistema
  - `lastUpdate` — Timestamp (ms) do último comando UDP
  - `uptime` — Tempo de funcionamento (ms)
  - `freeHeap` — Memória RAM livre (bytes)

**Exemplo com curl:**
```bash
curl http://192.168.4.1/status | jq .

# Monitorar continuamente
watch -n 0.1 "curl -s http://192.168.4.1/status | jq '.motors'"
```

**Motor Mixing (Quadcopter X-Frame):**
```
Fórmula aplicada no firmware:
M1 (FL) = throttle + pitch + roll - yaw
M2 (FR) = throttle + pitch - roll + yaw
M3 (BL) = throttle - pitch + roll + yaw
M4 (BR) = throttle - pitch - roll - yaw

Layout físico:
    M1(FL)    M2(FR)
      \ /
       X
      / \
    M3(BL)  M4(BR)
```

---

## Arquitetura FreeRTOS

### Core 0 - Task WiFi Monitor (`TaskWiFi`)
```
Intervalo: 10 segundos
Ação: Verifica se WiFi está conectado
Se desconectado: chama setupWiFi() novamente
Prioridade: 1 (normal)
Stack: 4096 bytes
```

### Core 1 - Task UDP Receiver (`TaskUDP`)
```
Intervalo: 1ms (yield automático)
Ação: Monitora porta UDP 9000
Se pacote > 0: parseia e atualiza currentControl
Prioridade: 1 (normal)
Stack: 4096 bytes
```

### Core 1 - Task PWM Controller (`TaskPWM`)
```
Intervalo: 10ms (100 Hz)
Ação: Calcula motor mixing baseado em currentControl
      Atualiza PWM dos 4 motores em tempo real
Prioridade: 2 (maior que UDP, menor que WiFi)
Stack: 4096 bytes
Log: A cada 500ms com valores de PWM e controle

O que faz:
1. Lê currentControl (roll, pitch, yaw, throttle)
2. Aplica fórmula de mixing para quadcóptero X-Frame
3. Mapeia [-1, +1] para range PWM [1000, 2000] µs
4. Normaliza se algum motor saturar
5. Escreve valores PWM nos pinos (GPIO 12, 13, 14, 15)
```

### Core 1 - Loop Principal (`loop()`)
```
Ação: Processa requisições HTTP
Intervalo: 1ms (yield)
Mantém servidor HTTP respondendo
```

---

## Como compilar e subir

### Arduino IDE (recomendado)
1. Abra `BackEnd/Tapejara/Tapejara.ino` no Arduino IDE
2. Instale a biblioteca **ESP32 by Espressif Systems** (Tools > Board Manager)
3. Selecione placa: **AI Thinker ESP32-CAM**
4. Configure porta serial e velocidade (115200 baud)
5. Pressione **Upload**

### PlatformIO (avançado)
```ini
[env:esp32-cam]
platform = espressif32
board = ai-thinker-esp32-cam
framework = arduino
upload_speed = 921600
```

```bash
platformio run --target upload
```

---

## Configurações principais

Edite em `Tapejara.ino`:

```cpp
const char* WIFI_SSID = "CACHONE";           // Rede WiFi
const char* WIFI_PASS = "f22485910";         // Senha
const int UDP_PORT = 9000;                   // Porta UDP
const uint8_t STATUS_LED_PIN = 4;            // LED de status
```

Configurações de câmera (em `initCamera()`):
```cpp
config.frame_size = FRAMESIZE_UXGA;          // 1600×1200
config.jpeg_quality = 12;                    // 0=máx, 63=mín
config.xclk_freq_hz = 24000000;              // Clock 24 MHz
```

Rotação de câmera (em `initCamera()`):
```cpp
s->set_vflip(s, 1);      // Flip vertical
s->set_hmirror(s, 0);    // Mirror horizontal
```

Pinos dos motores (PWM/ESC) — ajuste conforme seu hardware:
```cpp
#define MOTOR1_PIN        12  // Motor frente-esquerdo (FL)
#define MOTOR2_PIN        13  // Motor frente-direito (FR)
#define MOTOR3_PIN        14  // Motor trás-esquerdo (BL)
#define MOTOR4_PIN        15  // Motor trás-direito (BR)
```

Configurações PWM (em `Tapejara.ino`):
```cpp
const int PWM_FREQ = 50;          // 50 Hz (padrão ESC)
const int PWM_RESOLUTION = 16;    // 16-bit (0-65535)
const int MIN_PWM_VALUE = 1000;   // 1ms = Motors OFF
const int MAX_PWM_VALUE = 2000;   // 2ms = Motors FULL
```

---

## Testes práticos

### 1️⃣ Visualizar stream em navegador
```
Abra: http://<ESP_IP>/
```

### 2️⃣ Verificar debug em tempo real
```bash
# Monitorar continuamente
watch -n 0.1 "curl -s http://192.168.4.1/debug | jq ."

# Ou com curl simples
curl http://192.168.4.1/debug
```

### 3️⃣ Monitorar status dos motores (PWM em tempo real)
```bash
# Visualizar PWM e controle
curl http://192.168.4.1/status | jq '.motors'

# Saída esperada:
# {
#   "m1": 1340,
#   "m2": 1288,
#   "m3": 1412,
#   "m4": 1360
# }

# Monitorar continuamente (a cada 100ms)
watch -n 0.1 "curl -s http://192.168.4.1/status | jq '.motors'"
```

### 4️⃣ Enviar comando UDP de teste
```python
import socket, struct, time

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

for i in range(10):
    pkt = struct.pack('<BBBBHHFFFF H',
        0xED, 0x01, 0, 0,        # Magic
        i,                        # Sequence
        0.5, 0.2, -0.1, 0.7,     # roll, pitch, yaw, throttle
        0                         # buttons
    )
    sock.sendto(pkt, ("192.168.4.1", 9000))
    time.sleep(0.1)

sock.close()
```

### 5️⃣ Monitorar serial (115200 baud)
```bash
picocom /dev/ttyUSB0 -b 115200
# ou
minicom -D /dev/ttyUSB0 -b 115200
```

**Saída esperada do boot:**
```
=== TAPEJARA BOOT ===
LED
--- Inicializando PWM (LEDC) ---
✓ PWM inicializado (50 Hz, 16-bit)
  Motor1 (FL) -> GPIO 12 (Canal 0)
  Motor2 (FR) -> GPIO 13 (Canal 1)
  Motor3 (BL) -> GPIO 14 (Canal 2)
  Motor4 (BR) -> GPIO 15 (Canal 3)
  Range PWM: 1000 - 2000 µs

Inicializando câmera...
✓ Câmera iniciada
Conectando em CACHONE
✓ WiFi conectado!
IP: 192.168.4.144
Stream: http://192.168.4.144/stream
Inicializando UDP no porto 9000...
✓ UDP iniciado com sucesso na porta 9000
✓ Servidor HTTP iniciado (porta 80)
Task UDP iniciada no Núcleo 1
Task PWM iniciada no Núcleo 1
=== BOOT COMPLETO ===
Free Heap: 145632

--- Ao receber comandos UDP ---
📡 UDP #1 | roll=0.50 pitch=0.20 yaw=-0.10 thr=0.70 btn=0
⚡ PWM: M1=1585 M2=1515 M3=1655 M4=1545 | thr=0.70 pit=0.20 rol=0.50 yaw=-0.10
```

---

## Diagrama de fluxo

```
┌─────────────────────────────────────────────────────────┐
│                    APP ANDROID                          │
│  (envia UDP + recebe stream via HTTP)                   │
└────────┬──────────────────────────────┬─────────────────┘
         │                              │
         │ UDP 9000 (24 bytes)         │ HTTP GET /
         │ (roll, pitch, yaw, thr)     │ /stream, /status

         ↓                              ↓
┌─────────────────────────────────────────────────────────┐
│              ESP32-CAM (Núcleo 0 + 1)                   │
│                                                         │
│  Core 0:                          Core 1:              │
│  ├─ loop() [HTTP handler]        ├─ TaskWiFi          │
│  │  ├─ GET /     (HTML)          ├─ TaskUDP           │
│  │  ├─ GET /stream (MJPEG)       │  └─ recv UDP 9000   │
│  │  ├─ GET /status (JSON)        │     └─ atualiza     │
│  │  └─ GET /debug (JSON)         │        currentControl
│  │                                │                     │
│  └────────────────────────────────┤─ TaskPWM          │
│                                   │  ├─ calcula mixing  │
│                                   │  └─ escreve PWM     │
└─────────────┬─────────────────────┴─────────────────────┘
              │
              │ GPIO 12, 13, 14, 15 (PWM 1000-2000 µs)
              ↓
        ┌──────────────────┐
        │ ESCs / Motores   │
        │ M1(FL) M2(FR)   │
        │ M3(BL) M4(BR)   │
        └──────────────────┘
```

---

## Troubleshooting

| Problema | Causa | Solução |
|----------|-------|---------|
| WiFi não conecta | SSID/senha incorretos | Edite `WIFI_SSID` e `WIFI_PASS` no código |
| LED não pisca | Pino incorreto | Verifique `STATUS_LED_PIN` para seu módulo |
| Stream lento | Resolução muito alta | Reduza `FRAMESIZE` para `FRAMESIZE_VGA` |
| UDP não recebe pacotes | Firewall bloqueando | Verifique regras firewall na rede |
| Placa trava ao iniciar | Memória insuficiente | Reduza `fb_count` de câmera para 1 |
| Câmera de cabeça para baixo | Rotação incorreta | Ajuste `set_vflip` e `set_hmirror` |
| Motores não ligam | Pinos PWM incorretos | Verifique GPIO 12, 13, 14, 15 ou ajuste em `#define MOTOR*_PIN` |
| PWM não varia | UDP não está recebendo | Teste com `/debug` para ver se `throttle` está mudando |
| Motor roça no valor mínimo | ESC sensível | Aumente `MIN_PWM_VALUE` para 1100-1200 µs |
| Motor só liga com throttle máximo | ESC necessita arm | Implemente sequência de arm (futuro) |
| FreeRTOS warning de stack | Tasks com pouca memória | Aumentar `4096` para `8192` em `xTaskCreatePinnedToCore` |

---

## Notas finais

- **FreeRTOS:** Duas tasks rodam paralelamente (WiFi + UDP) deixando o `loop()` livre para HTTP
- **Sincronização:** Não há mutex; `currentControl` é lido/escrito simultaneamente (safe em single-point writes)
- **Performance:** UXGA + JPEG quality 12 ~= 20-30 FPS dependendo de iluminação e WiFi
- **Timeout:** Sem timeout TCP automático em `/stream`; cliente desconectado é detectado por `client.connected()`