/*
  Funcionalidades Principais:
  - Controle da "porta" (servo) via botões.
  - Temporizador de auto-fechamento de 5 segundos após um acesso válido.
  - Diferenciação entre tentativas de acesso válido e inválido via monitor serial.
  - Otimização de mensagens repetitivas no monitor serial.
  - Utilização da biblioteca `ezButton` para gerenciar o debounce dos botões.
*/

#include <ESP32Servo.h> // Para controlar o servo motor
#include <ezButton.h>    // Para gerenciar os botões

// --- Definição de Pinos ---
#define BTN_ACESSO_VALIDO_PIN 21 // Botão de acesso válido
#define BTN_ACESSO_INVALIDO_PIN 22 // Botão de acesso inválido
#define BTN_RESET_PIN 23         // Botão de reset/fechamento

#define SERVO_PIN 26             
// --- Objetos ---
ezButton btnAcessoValido(BTN_ACESSO_VALIDO_PIN);
ezButton btnAcessoInvalido(BTN_ACESSO_INVALIDO_PIN);
ezButton btnReset(BTN_RESET_PIN);
Servo servo;

// --- Variáveis de Estado ---
int servoAngle = 0;            
bool portaAberta = false;       // Estado da porta
bool printedAlreadyOpenMessage = false; // Controla repetição da mensagem

// --- Variáveis de Temporização Não-Bloqueante ---
unsigned long doorOpenTime = 0;              // Tempo de abertura da porta
const unsigned long AUTO_CLOSE_DELAY_MS = 5000; // Atraso para auto-fechamento (5 segundos)


void setup() {
  // Inicialização Serial
  Serial.begin(115200);

  // Configura debounce para os botões
  btnAcessoValido.setDebounceTime(50);
  btnAcessoInvalido.setDebounceTime(50);
  btnReset.setDebounceTime(50);

  // Anexa e inicializa o servo
  servo.attach(SERVO_PIN);
  servo.write(servoAngle); // Servo em 0 graus (fechado)
}


void loop() {
  // Atualiza o estado dos botões
  btnAcessoValido.loop();
  btnAcessoInvalido.loop();
  btnReset.loop();

  // --- Lógica de Acesso ---

  // Botão de Acesso Válido
  if (btnAcessoValido.isPressed()) {
    Serial.println("Botão de Acesso Válido Pressionado!");
    if (!portaAberta) { // Se a porta estiver fechada, tenta abrir
      Serial.println("Acesso Válido: Porta Abrindo...");
      servoAngle = 90;
      servo.write(servoAngle);
      portaAberta = true;
      doorOpenTime = millis(); // Reseta o temporizador de auto-fechamento

      printedAlreadyOpenMessage = false;
    } else { // Se a porta já estiver aberta
      if (!printedAlreadyOpenMessage) {
        Serial.println("Acesso Válido: Porta já está aberta.");
        printedAlreadyOpenMessage = true;
      }
    }
  }

  // Botão de Acesso Inválido
  if (btnAcessoInvalido.isPressed()) {
    Serial.println("Botão de Acesso Inválido Pressionado!");
    Serial.println("Acesso Inválido: Negado!");
    // Nenhuma ação física, apenas feedback serial.
  }

  // Botão de Reset/Fechamento da Porta
  if (btnReset.isPressed()) {
    Serial.println("Botão de Reset/Fechamento Pressionado!");
    if (portaAberta) { // Se a porta estiver aberta, tenta fechar
      Serial.println("Porta Fechando...");
      servoAngle = 0;
      servo.write(servoAngle);
      portaAberta = false;

      printedAlreadyOpenMessage = false;
    } else { // Se a porta já estiver fechada
      Serial.println("Porta já está fechada.");
    }
  }

  // --- Temporizador de Auto-Fechamento da Porta ---
  // Fecha a porta automaticamente após o tempo definido, se estiver aberta.
  if (portaAberta && (millis() - doorOpenTime >= AUTO_CLOSE_DELAY_MS)) {
    Serial.println("Tempo esgotado: Porta fechada automaticamente.");
    servoAngle = 0;
    servo.write(servoAngle);
    portaAberta = false;

    printedAlreadyOpenMessage = false;
  }

  // Pequeno atraso para estabilidade do loop
  delay(10);
}
