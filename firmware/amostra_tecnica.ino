#include "BluetoothSerial.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

#define PINO_RELE1 27
#define PINO_RELE2 12
const int PINO_TRANSISTOR = 23;
const int PINO_LDR = 34;
const int PINO_BUTTON = 26;

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(4, DHT11); 
BluetoothSerial SerialBT; 

// Limiares de luz ambiente (ajustável)
// [CORRIGIDO] O circuito é INVERTIDO: quanto mais escuro, MAIOR o valor.
// Lanterna = 0 | Luz ambiente = ~2850 | Dedo cobrindo = 4095
const int LIMIAR_ESCURO = 3400; // ACIMA disso = LDR sombreado (dedo perto)
const int LIMIAR_CLARO = 3000;  // ABAIXO disso = LDR com luz normal

bool cobertoPeloDedo = false;
String estadoLCD = "Modo: Automatico";

// Variáveis para controlar o tempo sem usar delay() muito alto
unsigned long ultimoDHT = 0;
const unsigned long INTERVALO_DHT = 2000; // 2 segundos de leitura do DHT
float temperatura = NAN;
float umidade = NAN;

// Variáveis do push button
int ultimoEstadoBotao = HIGH;
unsigned long ultimoDebounce = 0;
const unsigned long TEMPO_DEBOUNCE = 50;

void setup() {
  Serial.begin(115200); 
  
  pinMode(PINO_LDR, INPUT);
  pinMode(PINO_BUTTON, INPUT_PULLUP);

  // O transistor liga no setup e NUNCA mais desliga a alimentação do LDR
  pinMode(PINO_TRANSISTOR, OUTPUT);
  digitalWrite(PINO_TRANSISTOR, HIGH); 

  // [CORRIGIDO] Ordem original: primeiro vira OUTPUT, depois escreve HIGH.
  // (Relés acionados em LOW: HIGH = desligado)
  pinMode(PINO_RELE1, OUTPUT); 
  pinMode(PINO_RELE2, OUTPUT);
  digitalWrite(PINO_RELE1, HIGH); 
  digitalWrite(PINO_RELE2, HIGH);

  dht.begin(); 
  SerialBT.begin("Rodando na Base da Oração"); 
  SerialBT.setTimeout(50);
  
  lcd.init(); 
  lcd.backlight();
}

void loop() {
  // -------------------------------------------------------------
  // 1. LEITURA AUTOMÁTICA DO LDR
  // -------------------------------------------------------------
  // Média de 4 leituras para reduzir oscilação/ruído do LDR
  long soma = 0;
  for (int i = 0; i < 4; i++) {
    soma += analogRead(PINO_LDR);
    delay(2);
  }
  int leituraLDR = soma / 4;
  
  // Valor da luminosidade no monitor serial
  Serial.print("Leitura LDR: ");
  Serial.println(leituraLDR);

  // O botão só age UMA vez, no instante em que é apertado,
  // e alterna entre ligar e desligar as lâmpadas.
  int leituraButton = digitalRead(PINO_BUTTON);
  if (leituraButton != ultimoEstadoBotao && (millis() - ultimoDebounce) > TEMPO_DEBOUNCE) {
    ultimoDebounce = millis();
    ultimoEstadoBotao = leituraButton;

    // [TESTE] Mostra no Serial quando o botão muda (1 = solto, 0 = apertado).
    // Pode apagar essas duas linhas depois que o botão estiver funcionando.
    Serial.print("Botao mudou para: ");
    Serial.println(leituraButton);

    if (leituraButton == LOW) { // botão foi apertado (INPUT_PULLUP: apertado = LOW)
      bool algumaLigada = (digitalRead(PINO_RELE1) == LOW || digitalRead(PINO_RELE2) == LOW);
      if (algumaLigada) {
        digitalWrite(PINO_RELE1, HIGH);
        digitalWrite(PINO_RELE2, HIGH);
        estadoLCD = "Botao: OFF";
      } else {
        digitalWrite(PINO_RELE1, LOW);
        digitalWrite(PINO_RELE2, LOW);
        estadoLCD = "Botao: ON";
      }
    }
  }

  // [CORRIGIDO] SE o LDR ficar sombreado (valor SOBE, dedo perto): liga as lâmpadas
  if (leituraLDR > LIMIAR_ESCURO && !cobertoPeloDedo) {
    digitalWrite(PINO_RELE1, LOW);
    digitalWrite(PINO_RELE2, LOW);
    cobertoPeloDedo = true;
    estadoLCD = "LDR: Sombra ON";
  } 
  // [CORRIGIDO] SE a luz voltar (valor DESCE, tirou o dedo): desliga as lâmpadas
  else if (leituraLDR < LIMIAR_CLARO && cobertoPeloDedo) {
    digitalWrite(PINO_RELE1, HIGH);
    digitalWrite(PINO_RELE2, HIGH);
    cobertoPeloDedo = false;
    estadoLCD = "LDR: Livre";
  }

  // -------------------------------------------------------------
  // 2. COMANDOS VIA BLUETOOTH
  // -------------------------------------------------------------
  if (SerialBT.available()) {
    String receberDados = SerialBT.readString();
    receberDados.trim();
    
    if (receberDados == "Ligar L1") {
      digitalWrite(PINO_RELE1, LOW);
      estadoLCD = "BT: L1 ON";
    } 
    else if (receberDados == "Desligar L1") { 
      digitalWrite(PINO_RELE1, HIGH);
      estadoLCD = "BT: L1 OFF";
    }
    else if (receberDados == "Ligar L2") {
      digitalWrite(PINO_RELE2, LOW);
      estadoLCD = "BT: L2 ON";
    } 
    else if (receberDados == "Desligar L2") {
      digitalWrite(PINO_RELE2, HIGH);
      estadoLCD = "BT: L2 OFF";
    }
  }

  // -------------------------------------------------------------
  // 3. LEITURA DO DHT11 E EXIBIÇÃO NO LCD
  // -------------------------------------------------------------
  // Lê o DHT11 só a cada 2 segundos
  if (millis() - ultimoDHT >= INTERVALO_DHT) {
    ultimoDHT = millis();
    temperatura = dht.readTemperature();
    umidade = dht.readHumidity();
  }
  
  // Linha 0: Umidade e Temperatura
  lcd.setCursor(0, 0);
  lcd.print("U:");
  if (isnan(umidade)) {
    lcd.print("Err ");
  } else {
    lcd.print((int)umidade);
    lcd.print("% ");
  }

  lcd.print("T:");
  if (isnan(temperatura)) {
    lcd.print("Err ");
  } else {
    lcd.print((int)temperatura);
    lcd.print("C  ");
  }

  // Linha 1: Status no LCD
  String linha = estadoLCD;
  while (linha.length() < 16) linha += " ";
  lcd.setCursor(0, 1);
  lcd.print(linha.substring(0, 16));

  delay(100);
}
