/*
  Sistema de Controle de Acesso Ciberfísico Avançado

  Funcionalidades Principais:
  - Controle da "porta" (servo) via autenticação biométrica ou botões manuais.
  - Temporizador de auto-fechamento de 5 segundos após um acesso válido.
  - Feedback visual no display OLED para status do sistema e mensagens de acesso.
  - Otimização de mensagens repetitivas no monitor serial e OLED.
  - Utilização da biblioteca `ezButton` para gerenciar o debounce dos botões.
  - Comunicação MQTT para monitoramento remoto de eventos.

  Bibliotecas Utilizadas:
  - ESP32Servo.h: Para controlar o servo motor.
  - ezButton.h: Para leitura de botões com debounce.
  - Adafruit_GFX.h e Adafruit_SSD1306.h: Para controle do display OLED.
  - Adafruit_Fingerprint.h: Para interface com o sensor de impressão digital (GT-511C1R ou similar).
  - WiFi.h: Para conexão Wi-Fi.
  - PubSubClient.h: Para comunicação MQTT.
*/

#include <ESP32Servo.h>
#include <ezButton.h>
#include <Wire.h> // Necessário para I2C do OLED
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Fingerprint.h> // Para o sensor de impressão digital
#include <WiFi.h>
#include <PubSubClient.h>

// --- Seção de Definição de Pinos (Hardware Connections) ---
#define BTN_ACESSO_MANUAL_PIN 19
#define BTN_ACESSO_INVALIDO_PIN 18
#define BTN_RESET_PIN 5

#define SERVO_PIN 26

// Sensor de Impressão Digital (UART2 no ESP32)
#define FINGERPRINT_TX_PIN 17
#define FINGERPRINT_RX_PIN 16

// Display OLED I2C
#define OLED_SDA_PIN 21
#define OLED_SCL_PIN 22
#define SCREEN_WIDTH 128 // Largura do display OLED
#define SCREEN_HEIGHT 64 // Altura do display OLED
#define OLED_RESET -1    // Reset pin # (or -1 if sharing Arduino reset pin)

// --- Credenciais de Wi-Fi e MQTT ---
const char* ssid = "Wokwi-GUEST";
const char* password = ""; // Sem senha no Wokwi Guest
const char* mqtt_server = "broker.hivemq.com"; // Exemplo de broker público
const int mqtt_port = 1883;
const char* mqtt_user = ""; // Seu usuário MQTT (se houver)
const char* mqtt_pass = ""; // Sua senha MQTT (se houver)
const char* mqtt_client_id = "ESP32_AcessoCiberfisico";
const char* mqtt_topic_events = "acesso/eventos";

// --- Seção de Criação de Objetos de Hardware e Bibliotecas ---
ezButton btnAcessoManual(BTN_ACESSO_MANUAL_PIN);
ezButton btnAcessoInvalido(BTN_ACESSO_INVALIDO_PIN);
ezButton btnReset(BTN_RESET_PIN);
Servo servo;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial2); // Usa Serial2 para o sensor

WiFiClient espClient;
PubSubClient client(espClient);

// --- Seção de Variáveis de Estado do Sistema ---
int servoAngle = 0;
bool portaAberta = false;
unsigned long doorOpenTime = 0;
const unsigned long AUTO_CLOSE_DELAY_MS = 5000; // 5 segundos

// Variáveis para mensagens no Serial e OLED (evitar repetição)
bool printedAlreadyOpenMessage = false;
String currentOLEDMessage = "";
unsigned long lastOLEDUpdate = 0;
const unsigned long OLED_UPDATE_INTERVAL = 100; // ms

// Estados para o display OLED
enum DisplayState {
  INITIALIZING,
  READY_TO_SCAN,
  ACCESS_GRANTED,
  ACCESS_DENIED,
  PORT_OPEN,
  PORT_CLOSED,
  PORT_CLOSING,
  PORT_ALREADY_OPEN,
  PORT_ALREADY_CLOSED,
  MANUAL_OVERRIDE_GRANTED,
  MANUAL_OVERRIDE_DENIED
};
DisplayState currentDisplayState = INITIALIZING;

// --- Protótipos de Funções ---
void updateOLED(DisplayState newState, String customMessage = "");
void setupWiFi();
void reconnectMQTT();
void publishMqttEvent(const char* eventType, const char* message);
void handleFingerprintSensor();
uint8_t getFingerprintID();

void setup() {
  Serial.begin(115200);

  // --- Inicialização do Display OLED ---
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN); // Inicializa I2C para o OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Endereço 0x3C para 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;) delay(100); // Não continua se o OLED falhar
  }
  display.display();
  delay(2000); // Pausa para ver o logo
  updateOLED(INITIALIZING, "Inicializando...");

  // --- Configuração dos Botões ---
  btnAcessoManual.setDebounceTime(50);
  btnAcessoInvalido.setDebounceTime(50);
  btnReset.setDebounceTime(50);

  // --- Configuração do Servo Motor ---
  servo.attach(SERVO_PIN);
  servo.write(servoAngle); // Garante que a porta começa fechada

  // --- Configuração do Sensor de Impressão Digital ---
  Serial2.begin(57600, SERIAL_8N1, FINGERPRINT_RX_PIN, FINGERPRINT_TX_PIN);
  if (!finger.begin()) {
    Serial.println("Erro! Sensor de impressão digital não encontrado.");
    updateOLED(INITIALIZING, "Erro: Sensor FP");
    while (1) delay(1); // Trava aqui se o sensor não iniciar
  }
  Serial.println("Sensor de impressão digital encontrado!");
  finger.verifyPassword(); // Verifica a senha do módulo

  // --- Configuração Wi-Fi e MQTT ---
  setupWiFi();
  client.setServer(mqtt_server, mqtt_port);
  // client.setCallback(callback); // Se houver necessidade de receber mensagens MQTT
  updateOLED(READY_TO_SCAN, "Pronto para ler dedo");
}

void loop() {
  // --- Atualização do Estado dos Botões ---
  btnAcessoManual.loop();
  btnAcessoInvalido.loop();
  btnReset.loop();

  // --- Lógica Principal de Controle de Acesso ---

  // Lógica para o Botão de Acesso Manual (Override)
  if (btnAcessoManual.isPressed()) {
    Serial.println("Botão de Acesso Manual Pressionado!");
    if (!portaAberta) {
      Serial.println("Acesso Manual: Porta Abrindo...");
      servoAngle = 90;
      servo.write(servoAngle);
      portaAberta = true;
      doorOpenTime = millis();
      printedAlreadyOpenMessage = false;
      updateOLED(MANUAL_OVERRIDE_GRANTED, "Acesso Manual");
      publishMqttEvent("ACESSO_MANUAL_LIBERADO", "Porta aberta manualmente.");
    } else {
      if (!printedAlreadyOpenMessage) {
        Serial.println("Acesso Manual: Porta já está aberta.");
        updateOLED(PORT_ALREADY_OPEN);
        printedAlreadyOpenMessage = true;
      }
    }
  }

  // Lógica para o Botão de Acesso Inválido
  if (btnAcessoInvalido.isPressed()) {
    Serial.println("Botão de Acesso Inválido Pressionado!");
    Serial.println("Acesso Inválido: Negado!");
    updateOLED(ACCESS_DENIED, "Acesso Negado!");
    publishMqttEvent("ACESSO_MANUAL_NEGADO", "Tentativa de acesso invalido.");
  }

  // Lógica para o Botão de Reset/Fechamento da Porta
  if (btnReset.isPressed()) {
    Serial.println("Botão de Reset/Fechamento Pressionado!");
    if (portaAberta) {
      Serial.println("Porta Fechando...");
      servoAngle = 0;
      servo.write(servoAngle);
      portaAberta = false;
      updateOLED(PORT_CLOSING, "Fechando Porta...");
      publishMqttEvent("PORTA_FECHADA_MANUALMENTE", "Porta fechada via botao reset.");
    } else {
      Serial.println("Porta já está fechada.");
      updateOLED(PORT_ALREADY_CLOSED);
    }
    printedAlreadyOpenMessage = false; // Reseta a flag para novas aberturas
  }

  // --- Temporizador de Auto-Fechamento da Porta (Lógica Não-Bloqueante) ---
  if (portaAberta && (millis() - doorOpenTime >= AUTO_CLOSE_DELAY_MS)) {
    Serial.println("Tempo esgotado: Porta fechada automaticamente.");
    servoAngle = 0;
    servo.write(servoAngle);
    portaAberta = false;
    updateOLED(PORT_CLOSED, "Porta Fechada");
    publishMqttEvent("PORTA_FECHADA_AUTOMATICAMENTE", "Porta fechada automaticamente.");
    printedAlreadyOpenMessage = false;
  }

  // --- Gerenciamento do Sensor de Impressão Digital ---
  handleFingerprintSensor();

  // --- Manutenção da Conexão MQTT ---
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop(); // Processa mensagens MQTT

  // --- Atualização do OLED (controle de frequência) ---
  if (millis() - lastOLEDUpdate >= OLED_UPDATE_INTERVAL) {
    updateOLED(currentDisplayState); // Atualiza o OLED com o estado atual, se necessário
    lastOLEDUpdate = millis();
  }

  delay(10); // Pequeno atraso para estabilidade do loop
}

// --- Funções Auxiliares ---

void updateOLED(DisplayState newState, String customMessage) {
  if (newState != currentDisplayState || customMessage != currentOLEDMessage) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    switch (newState) {
      case INITIALIZING:
        display.println("Inicializando...");
        display.println(customMessage);
        break;
      case READY_TO_SCAN:
        display.println("Aproxime o dedo...");
        display.println("Pronto para escanear");
        break;
      case ACCESS_GRANTED:
        display.println("Acesso Liberado!");
        display.println("Porta Aberta");
        break;
      case ACCESS_DENIED:
        display.println("Acesso Negado!");
        display.println(customMessage);
        break;
      case PORT_OPEN:
        display.println("Porta Aberta");
        display.println("Aguardando fechamento");
        break;
      case PORT_CLOSED:
        display.println("Porta Fechada");
        display.println("Sistema Pronto");
        break;
      case PORT_CLOSING:
        display.println("Fechando Porta...");
        break;
      case PORT_ALREADY_OPEN:
        display.println("Porta ja esta aberta.");
        break;
      case PORT_ALREADY_CLOSED:
        display.println("Porta ja esta fechada.");
        break;
      case MANUAL_OVERRIDE_GRANTED:
        display.println("Acesso Manual OK!");
        display.println("Porta Aberta");
        break;
      case MANUAL_OVERRIDE_DENIED:
        display.println("Acesso Manual");
        display.println("NEGADO!");
        break;
      default:
        display.println("Status Desconhecido");
        break;
    }
    display.display();
    currentDisplayState = newState;
    currentOLEDMessage = customMessage;
  }
}

void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    updateOLED(INITIALIZING, "Conectando WiFi...");
  }

  Serial.println("");
  Serial.println("WiFi conectado!");
  Serial.print("Endereco IP: ");
  Serial.println(WiFi.localIP());
  updateOLED(INITIALIZING, "WiFi Conectado!");
  delay(1000);
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Tentando conexao MQTT...");
    // Tenta conectar com nome de usuário e senha, se definidos
    if (client.connect(mqtt_client_id, mqtt_user, mqtt_pass)) {
      Serial.println("conectado");
      // client.subscribe("seu/topico/aqui"); // Se precisar subscrever
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      updateOLED(INITIALIZING, "MQTT Falhou...");
      delay(5000);
    }
  }
}

void publishMqttEvent(const char* eventType, const char* message) {
  String payload = "{\"event\": \"";
  payload += eventType;
  payload += "\", \"message\": \"";
  payload += message;
  payload += "\", \"timestamp\": ";
  payload += millis(); // ou use um timestamp real
  payload += "}";

  if (client.publish(mqtt_topic_events, payload.c_str())) {
    Serial.println("Evento MQTT publicado: " + payload);
  } else {
    Serial.println("Falha ao publicar evento MQTT.");
  }
}

void handleFingerprintSensor() {
  if (portaAberta) { // Não verifica impressão se a porta já estiver aberta
    return;
  }

  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return;

  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    Serial.print("Encontrei ID #");
    Serial.print(finger.fingerID);
    Serial.print(" com confianca de ");
    Serial.println(finger.confidence);

    if (!portaAberta) {
      Serial.println("Acesso Biometrico Valido: Porta Abrindo...");
      servoAngle = 90;
      servo.write(servoAngle);
      portaAberta = true;
      doorOpenTime = millis();
      printedAlreadyOpenMessage = false;
      updateOLED(ACCESS_GRANTED, "ID: " + String(finger.fingerID));
      publishMqttEvent("BIOMETRIA_LIBERADA", "Acesso liberado por biometria (ID: " + String(finger.fingerID) + ").");
    } else {
      // Caso improvável de um dedo ser lido enquanto a porta abre
      if (!printedAlreadyOpenMessage) {
        Serial.println("Biometria: Porta já está aberta.");
        updateOLED(PORT_ALREADY_OPEN);
        printedAlreadyOpenMessage = true;
      }
    }
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Digital nao encontrada.");
    updateOLED(ACCESS_DENIED, "Digital Invalida!");
    publishMqttEvent("BIOMETRIA_NEGADA", "Digital nao reconhecida.");
  } else {
    Serial.print("Erro de comunicacao com o sensor: ");
    Serial.println(p);
    updateOLED(INITIALIZING, "Erro Sensor FP");
  }
}
