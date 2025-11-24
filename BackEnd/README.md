# Tapejara - BackEnd

autor: Fabio Cachone

Descrição
--------
BackEnd responsável por orquestrar controle e telemetria do drone Tapejara e por integrar o stream de vídeo da câmera ESP32-CAM ao aplicativo móvel.

Principais responsabilidades
- Encaminhar comandos de controle do app móvel para o controlador do drone.
- Receber e disponibilizar telemetria (altitude, velocidade, bateria, GPS).
- Integrar e proxy do feed de vídeo da ESP32-CAM para o FrontEnd.
- Autenticação básica e gerenciamento de sessão com o app móvel.

Integração com ESP32-CAM
- Streams suportados (dependendo do firmware da ESP32-CAM):
  - MJPEG via HTTP (ex.: http://esp32-cam-ip:81/stream)
  - JPEG snapshots via HTTP
  - RTSP (se o firmware suportar)
- O BackEnd deve prover um endpoint que o FrontEnd consome para vídeo em tempo real (proxy HTTP/MJPEG ou reencaminhamento RTSP/WebRTC).

Endpoints sugeridos
- POST /api/command          — enviar comando de voo (takeoff, land, move, etc.)
- GET  /api/telemetry       — telemetria atual (JSON)
- WS   /api/telemetry/ws    — telemetria em tempo real via WebSocket
- GET  /api/camera/stream   — proxy para o stream da ESP32-CAM
- GET  /api/camera/snapshot — snapshot JPEG

Requisitos iniciais
- Node.js (ex.: 16+), ou stack escolhida (Python/Go/etc.)
- Conectividade entre BackEnd e ESP32-CAM (mesma rede ou túnel)
- Configuração de IP/porta da ESP32-CAM em variáveis de ambiente

Exemplo de configuração (.env)
- ESP32_CAM_URL=http://192.168.4.1:81/stream
- TELEMETRY_WS_PORT=8081
- API_PORT=3000

Como começar (exemplo Node.js/Express genérico)
1. Instalar dependências:
   - npm install
2. Configurar variáveis de ambiente (ver .env.example)
3. Rodar em desenvolvimento:
   - npm run dev
4. Testar endpoints:
   - Abrir [http://localhost:3000/api/camera/stream](http://localhost:3000/api/camera/stream) (proxy para ESP32-CAM)
   - Conectar WebSocket em ws://localhost:8081

Estrutura sugerida
- src/
  - controllers/   — handlers de API (camera, commands, telemetry)
  - services/      — integração com ESP32-CAM, controlador do drone, telemetria
  - routes/        — definição de rotas REST/WS
  - utils/         — helpers, validação e configuração
  - config/        — leitura de .env e constantes

Boas práticas
- Validar e sanitizar comandos recebidos do FrontEnd.
- Implementar reconexão automática com a ESP32-CAM.
- Registrar erros e eventos críticos (logs).
- Isolar o proxy de vídeo para reduzir latência e uso de CPU.

Referências no workspace
- Frontend com a interface: [FrontEnd/README.md](FrontEnd/README.md)
- Telemetria e armazenamento: [Telemetria/](Telemetria/)

Contribuição
- Use branches por feature e abra PRs.
- Documente mudanças na API e atualize este README.

Licença
- Defina a licença do projeto no arquivo raiz (README.md ou LICENSE).