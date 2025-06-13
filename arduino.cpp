#include <Servo.h>
#

const int ledAccess = 7;
const int ledDeny = 8;
const int servoPin = 9;

const int botaoCadastrar = 4;
const int botaoValidar = 10;
const int botaoNegar = 11;

Servo doorLock;

void setup() {
  doorLock.attach(servoPin);

  pinMode(ledAccess, OUTPUT);
  pinMode(ledDeny, OUTPUT);

  pinMode(botaoCadastrar, INPUT_PULLUP);
  pinMode(botaoValidar, INPUT_PULLUP);
  pinMode(botaoNegar, INPUT_PULLUP);

  Serial.begin(9600);
  Serial.println("Sistema Biométrico - Inicializado");
  Serial.println("1 - Botão CADASTRAR ");
  Serial.println("2 - Botão VALIDAR ");
  Serial.println("3 - Botão NEGAR ");

  doorLock.write(0);
  digitalWrite(ledAccess, LOW);
  digitalWrite(ledDeny, LOW);
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

void cadastrarDigital() {
  Serial.println("\n--- MODO CADASTRO ---");
  Serial.println("Cadastrando digital...");

  digitalWrite(ledAccess, HIGH);
  digitalWrite(ledDeny, HIGH);
  delay(1000);

  Serial.println("Digital cadastrada!");

  for (int i = 0; i < 3; i++) {
    digitalWrite(ledAccess, LOW);
    digitalWrite(ledDeny, LOW);
    delay(200);
    digitalWrite(ledAccess, HIGH);
    digitalWrite(ledDeny, HIGH);
    delay(200);
  }

  digitalWrite(ledAccess, LOW);
  digitalWrite(ledDeny, LOW);
}

void validarDigital() {
  Serial.println("\n--- ACESSO AUTORIZADO ---");
  digitalWrite(ledAccess, HIGH);
  doorLock.write(180);
  delay(3000);
  doorLock.write(0);
  delay(500);
  digitalWrite(ledAccess, LOW);
}

void negarDigital() {
  Serial.println("\n--- ACESSO NEGADO ---");
  digitalWrite(ledDeny, HIGH);
  delay(200);
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledDeny, LOW);
    delay(200);
    digitalWrite(ledDeny, HIGH);
    delay(200);
  }
  delay(500);
  digitalWrite(ledDeny, LOW);
}
