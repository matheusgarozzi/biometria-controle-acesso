#include <Adafruit_Fingerprint.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // Endereço 0x27 para LCD 16x2

SoftwareSerial mySerial(2, 3); // RX, TX
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

const int relePin = 8;
const int buzzerPin = 9;
const int ledVerde = 10;
const int ledVermelho = 11;

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  
  pinMode(relePin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledVerde, OUTPUT);
  pinMode(ledVermelho, OUTPUT);

  finger.begin(57600);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  Sistema de");
  lcd.setCursor(0, 1);
  lcd.print("  Acesso");
  delay(2000);
  
  if (finger.verifyPassword()) {
    lcd.clear();
    lcd.print("Sensor OK");
    Serial.println("Sensor biométrico encontrado!");
  } else {
    lcd.clear();
    lcd.print("Sensor Falhou");
    Serial.println("Sensor biométrico não encontrado :(");
    while (1);
  }
}

void loop() {
  lcd.clear();
  lcd.print("Coloque o dedo");
  
  int resultado = getFingerprintIDez();
  
  if (resultado >= 0) {
    lcd.clear();
    lcd.print("Acesso Liberado!");
    digitalWrite(relePin, HIGH);
    digitalWrite(ledVerde, HIGH);
    tone(buzzerPin, 1000, 200);
    
    delay(3000); 
    
    digitalWrite(relePin, LOW);
    digitalWrite(ledVerde, LOW);
  } else if (resultado == -1) {

    lcd.clear();
    lcd.print("Acesso Negado!");
    digitalWrite(ledVermelho, HIGH);
    tone(buzzerPin, 300, 1000);
    delay(1000);
    digitalWrite(ledVermelho, LOW);
  } else if (resultado == -2) {

    lcd.clear();
    lcd.print("Erro no sensor");
    digitalWrite(ledVermelho, HIGH);
    delay(1000);
    digitalWrite(ledVermelho, LOW);
  }
  
  delay(50);
}


int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return -2;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -2;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) {
    Serial.println("Digital não encontrada");
    return -1;
  }
  
  Serial.print("ID encontrado #"); 
  Serial.print(finger.fingerID);
  Serial.print(" com confiança de "); 
  Serial.println(finger.confidence);
  return finger.fingerID;
}
