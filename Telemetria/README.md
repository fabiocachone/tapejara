 Sistema de Telemetria para Drone com ESP32

 #Descrição:
Este projeto implementa um sistema de telemetria para drone utilizando um ESP32 com FreeRTOS, responsável por:

- Transmitir dados de orientação (Pitch, Roll, Yaw) via MPU6050

- Monitorar tensão da bateria

- Monitorar corrente da bateria

- Enviar dados em tempo real via Wi-Fi + MQTT

O sistema é dividido em tarefas independentes rodando em núcleos diferentes do ESP32, garantindo melhor desempenho e organização do firmware.

 #Arquitetura do Sistema

O firmware utiliza FreeRTOS com duas tarefas principais:

🔹 Core 1 – TaskControle (Prioridade 2)

Responsável por:

Inicialização do MPU6050

Leitura contínua dos ângulos

Futuramente: controle de voo (PID, motores, etc.)

🔹 Core 0 – TaskTelemetria (Prioridade 1)

Responsável por:

Conexão Wi-Fi

Conexão MQTT

Leitura de tensão e corrente da bateria

Publicação dos dados via MQTT

# Dados Transmitidos

Os seguintes tópicos MQTT são publicados:

Variável	Tópico MQTT
Pitch	sensor/mpu/pitch
Roll	sensor/mpu/roll
Yaw	sensor/mpu/yaw
Corrente da bateria	bateria/corrente
Tensão da bateria	bateria/tensão

# Hardware Utilizado

ESP32

MPU6050 (I2C – GPIO 21 SDA / GPIO 22 SCL)

Amplificador diferencial (ganho = 62)

Resistor shunt (1Ω)

Divisor de tensão (fator = 2)

ADC GPIO 34 → Tensão

ADC GPIO 35 → Corrente

# Parâmetros Importantes
ADC

Resolução configurada para 11 bits

Média de 20 amostras para redução de ruído

Cálculo da tensão

Utiliza polinômio de 3º grau (forma de Horner):

V = (((a*x + b)*x + c)*x + d) * divisao_tensao

Cálculo da corrente
I = (V_calibrado / ganho * divisao_tensao) / rshunt

# Configuração de Rede
Wi-Fi
const char* ssid = "SEU_WIFI";
const char* password = "SUA_SENHA";

MQTT
const char* mqtt_server = "IP_DO_SERVIDOR";
const int mqtt_port = 1883;

# Estrutura de Tarefas (FreeRTOS)
xTaskCreatePinnedToCore(TaskControle, "Controle", 4096, NULL, 2, NULL, 1);
xTaskCreatePinnedToCore(TaskTelemetria, "Telemetria", 8192, NULL, 1, NULL, 0);

Task	Core	Função
Controle	1	Leitura MPU e futuro controle
Telemetria	0	Wi-Fi, MQTT e bateria

# Fluxo de Funcionamento

ESP32 inicia

Tarefas são criadas

Core 1 inicializa MPU

Core 0 conecta ao Wi-Fi

Core 0 conecta ao MQTT

Sistema começa a transmitir dados continuamente

# Taxas de Atualização

MPU6050 → ~1ms delay

Telemetria → 30ms delay (~33Hz)

# Futuras Implementações

Implementação de PID para controle de voo

Filtro complementar/Kalman para melhor estimativa de atitude

Watchdog para segurança

Detecção de bateria crítica

Buffer circular para logs

Criptografia MQTT (TLS)

# Dependências

MPU6050_tockn

WiFi.h

PubSubClient

Wire.h

FreeRTOS (nativo do ESP32)

# Exemplo de Monitoramento

Você pode visualizar os dados usando:

Node-RED

MQTT Explorer

Mosquitto + Terminal

Dashboard Web personalizado

Exemplo via terminal:

mosquitto_sub -h 192.168.X.X -t sensor/mpu/pitch

# Objetivo do Projeto

Este sistema faz parte do desenvolvimento de um drone com telemetria embarcada, permitindo:

Monitoramento remoto em tempo real

Análise de estabilidade

Avaliação do consumo energético

Base para desenvolvimento de controle autônomo

# Autor: Nicolas benitez Lopes

Projeto desenvolvido para aplicação em sistemas embarcados e controle de voo com ESP32.
