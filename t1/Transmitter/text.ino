#include "Temporizador.h"

#define PINO_RX 13
#define PINO_CTS 5
#define PINO_RTS 6

#define BAUD_RATE 1
#define HALF_BAUD 1000/(2*BAUD_RATE)

#define FIRST_BIT_POS -1
#define PARITY_BIT_POS 8
#define END_BIT_POS 9

bool bitParidade(char data){
  bool isPair = true;
  for (uint8_t i = 0; i < 8; i++) if (data & (1 << i)) isPair = !isPair; // Inverte bit de paridade
  return isPair;
}

// Rotina de interrupcao do timer1
// O que fazer toda vez que 1s passou?
// Variaveis globais para serem acessadas dentro do ISR
uint8_t bitPos = -1;      
bool enviando = false;
uint8_t currLetter = 0;
bool isValid = false;

ISR(TIMER1_COMPA_vect){
  if(!enviando) return;

  uint8_t parityBit = 1;
  uint8_t endBit = 1;

  switch(bitPos) {
    case START_BIT:
      digitalWrite(PINO_TX, 1);
      Serial.println(F("Start Bit: 1"));
      Serial.print(F("Transmitindo "));
      Serial.println(currLetter);
      break;
  
    case PARITY_BIT:
      digitalWrite(PINO_TX, parity_bit);
      isValid = checkParity(current_letter);
      Serial.println("\nParidade é: " + String(isValid ? "válido" : "inválido"));
      break;    
    case END_BIT:
      Serial.print("Bit final ");
      Serial.println(end_bit);
      digitalWrite(PINO_RX, end_bit);
      nextLetter();
      break;
    
    default:
      bool data_bit = bitRead(current_letter, currBit);
      Serial.print(data_bit);
      digitalWrite(PINO_TX, data_bit);
      break;
  }
  currBit++; // Increment to next bit position 
}

void resetaVariaveisGlobais()
{
  enviando = false;    
  letter = 0;     
  bitPos = -1; 
}

void setupPinos()
{
  pinMode(PINO_RX, INPUT);
  pinMode(PINO_RTS, INPUT);
  pinMode(PINO_CTS, OUTPUT);
}

void recebeMensagem(){
  while(digitalRead(PINO_RTS) == LOW){
    delay(1000);
  }
}

void setup(){
  //desabilita interrupcoes
  noInterrupts();

  Serial.begin(9600);
  // Inicializa TX ou RX
  setupPinos();
  // Configura timer
  configuraTemporizador(BAUD_RATE);
  // habilita interrupcoes
  interrupts();
}

// O loop() eh executado continuamente (como um while(true))
void loop ( ) {
  if(digitalRead(PINO_RTS) == LOW) return;
  resetaVariaveisGlobais();

  digitalWrite(PINO_CTS, HIGH);

  iniciaTemporizador();

  recebeMensagem();

  paraTemporizador();

  digitalWrite(PINO_CTS, LOW);
}
