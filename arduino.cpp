#include <Servo.h>

const int ledVerde = 12;
const int ledVermelho = 13;
const int servoPin = 9;

const int botaoCadastrar = 4;
const int botaoValidar = 10;
const int botaoNegar = 11;

Servo doorLock;

void setup() {
  doorLock.attach(servoPin);
  pinMode(ledVerde, OUTPUT);
  pinMode(ledVermelho, OUTPUT);
  pinMode(botaoCadastrar, INPUT_PULLUP);
  pinMode(botaoValidar, INPUT_PULLUP);
  pinMode(botaoNegar, INPUT_PULLUP);
  
  Serial.begin(9600);
  Serial.println("Sistema Biométrico - Inicializado");
  Serial.println("1 - Botão CADASTRAR");
  Serial.println("2 - Botão VALIDAR");
  Serial.println("3 - Botão NEGAR");

  doorLock.write(0);
  digitalWrite(ledVerde, LOW);
  digitalWrite(ledVermelho, LOW);

  testeLEDs();
}

void loop() {
  if (digitalRead(botaoCadastrar) == LOW) {
    delay(50);
    if (digitalRead(botaoCadastrar) == LOW) {
      cadastrarDigital();
      while(digitalRead(botaoCadastrar) == LOW);
    }
  }

  if (digitalRead(botaoValidar) == LOW) {
    delay(50);
    if (digitalRead(botaoValidar) == LOW) {
      validarDigital();
      while(digitalRead(botaoValidar) == LOW);
    }
  }

  if (digitalRead(botaoNegar) == LOW) {
    delay(50);
    if (digitalRead(botaoNegar) == LOW) {
      negarDigital();
      while(digitalRead(botaoNegar) == LOW);
    }
  }
}

void testeLEDs() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledVerde, HIGH);
    digitalWrite(ledVermelho, LOW);
    delay(200);
    digitalWrite(ledVerde, LOW);
    digitalWrite(ledVermelho, HIGH);
    delay(200);
  }
  digitalWrite(ledVermelho, LOW);
}

void cadastrarDigital() {
  Serial.println("\n--- MODO CADASTRO ---");
  Serial.println("Cadastrando digital...");

  for (int i = 0; i < 3; i++) {
    digitalWrite(ledVerde, HIGH);
    digitalWrite(ledVermelho, HIGH);
    delay(200);
    digitalWrite(ledVerde, LOW);
    digitalWrite(ledVermelho, LOW);
    delay(200);
  }
  
  Serial.println("Digital cadastrada!");
}

void validarDigital() {
  Serial.println("\n--- ACESSO AUTORIZADO ---");
  digitalWrite(ledVerde, HIGH);
  digitalWrite(ledVermelho, LOW);
  
  doorLock.write(180);
  delay(3000);
  doorLock.write(0);
  delay(500);
  
  digitalWrite(ledVerde, LOW);
}

void negarDigital() {
  Serial.println("\n--- ACESSO NEGADO ---");
  digitalWrite(ledVerde, LOW);
  digitalWrite(ledVermelho, HIGH);
  
  for (int i = 0; i < 3; i++) {
    delay(200);
    digitalWrite(ledVermelho, LOW);
    delay(200);
    digitalWrite(ledVermelho, HIGH);
  }
  
  delay(500);
  digitalWrite(ledVermelho, LOW);
}
