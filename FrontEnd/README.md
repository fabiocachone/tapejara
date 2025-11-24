# Tapejara - Frontend Mobile

autor: Rennan Giaretta

Descrição
--------
README simples para a parte Frontend do aplicativo móvel responsável pelo controle remoto do drone "Tapejara". Este projeto provê a interface do piloto, envio de comandos e visualização básica de telemetria.

Principais responsabilidades
- Interface de controle (joysticks, botões de decolagem/aterrissagem, modos de voo).
- Exibição de telemetria em tempo real (altitude, velocidade, bateria, GPS).
- Gerenciamento de conexão com o BackEnd/Hardware (WebSocket/UDP/HTTP — especificar conforme implementação).
- Logs e notificações de estado.

Funcionalidades iniciais sugeridas
- Tela principal com controles de voo.
- Indicadores de status (bateria, sinal, GPS).
- Mapa/posicionamento do drone (opcional).
- Configuração de perfil e calibração dos controles.
- Registro básico de voos.

Estrutura sugerida
- src/
  - components/   — controles reutilizáveis (joystick, indicador).
  - screens/      — telas da aplicação (Home, Telemetria, Config).
  - services/     — comunicação com BackEnd/Hardware.
  - assets/       — imagens, ícones.
  - utils/        — helpers e constantes.

Como começar (exemplo genérico)
1. Instalar dependências:
   - npm install ou yarn
2. Rodar em modo desenvolvimento:
   - npm run start (ou comando equivalente da stack mobile)
3. Conectar ao backend/telemetria:
   - Verificar config em src/services/ (endereço e protocolo)

Observações
- Atualize este README com a stack escolhida (React Native, Flutter, Ionic, etc.), comandos exatos de execução e dependências.
- Documente o protocolo de comunicação com o BackEnd/Hardware em [BackEnd/](BackEnd/) e [Hardware/](Hardware/) para garantir compatibilidade.

Contribuição
- Use branches por feature e abra PRs claros.
- Adicione testes básicos para componentes críticos (controles e parsing de telemetria).

Licença
- Defina a licença do projeto no arquivo raiz (README.md ou LICENSE).