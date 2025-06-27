#include "Temporizador.h"

const int PINO_RX = 13;
const int PINO_RTS = 2;
const int PINO_CTS = 3;

#define BIT_INICIO -1
#define TAXA_BAUD 5
#define TAMANHO_BYTE 8
#define BIT_PARIDADE 8
#define BIT_FIM 9

char bitAtual = BIT_INICIO;
bool valido = false;
char caractere = 0;

bool paridadeImpar(char byte) {
  bool eimpar = 0;
  for (uint8_t i = 0; i < TAMANHO_BYTE; i++) if (byte & (1 << i)) eimpar = !eimpar;
  return eimpar;
}

ISR(TIMER1_COMPA_vect) {
  bool leitura = digitalRead(PINO_RX);

  switch (bitAtual) {
    case BIT_INICIO:
      if (!leitura) return;
      Serial.print("\nBit de Inicio: ");
      Serial.println(leitura);
      break;

    case BIT_PARIDADE:
      valido = paridadeImpar(caractere) == leitura;
      Serial.print("\nBit de paridade: ");
      Serial.println(leitura);
      break;

    case BIT_FIM:
      Serial.print(valido ? "Nao valido! " : "Valido ");
      Serial.println(caractere);
      valido = false;
      caractere = 0;
      bitAtual = -2;
      break;

    default:
      Serial.print(leitura);
      bitWrite(caractere, bitAtual, leitura);
      break;
  }

  bitAtual++;
}

void configurarPinos() {
  pinMode(PINO_RX, INPUT);
  pinMode(PINO_RTS, INPUT);
  pinMode(PINO_CTS, OUTPUT);
}

void setup() {
  noInterrupts();
  Serial.begin(9600);
  configurarPinos();
  configuraTemporizador(TAXA_BAUD);
  Serial.println("Pronto para receber dados: ");
  interrupts();
}

void reiniciarVariaveis() {
  bitAtual = BIT_INICIO;
  valido = false;
  caractere = 0;
}

void aguardarRTSLow() {
  while (digitalRead(PINO_RTS) != LOW) {
    delay(1000);
  }
}

void loop() {
  if (digitalRead(PINO_RTS) == LOW) return;

  Serial.println("Transmissao iniciada...");
  reiniciarVariaveis();
  digitalWrite(PINO_CTS, HIGH);
  iniciaTemporizador();
  aguardarRTSLow();
  paraTemporizador();
  digitalWrite(PINO_CTS, LOW);
  Serial.println("Transmissao finalizada...");
}
