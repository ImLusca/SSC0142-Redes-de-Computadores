#include "Temporizador.h"

const int TX_PIN = 13;     // TX pin for transmitting data
const int RTS_PIN = 2;    // RTS pin for flow control
const int CTS_PIN = 3;    // CTS pin for flow control
#define BAUD_RATE 2
#define START_BIT -1
#define PARITY_BIT 8
#define SIZE_BYTE 8
#define END_BIT 9
unsigned short byte_iterator;
int text_size;
char current_letter;

bool transmitting = false; 
char current_bit = -1; 
String text = "";
bool isValid;

bool checkParity(char data) {
  bool isPair = true;
  for (uint8_t i = 0; i < 8; i++) if (data & (1 << i)) isPair = !isPair; // Inverte bit de paridade
  return isPair;
}


void transmit() {
  iniciaTemporizador();
  while (digitalRead(CTS_PIN) == HIGH) {
    delay(1000);
  }
  paraTemporizador();
}

void finalizeTransmition() {
  digitalWrite(RTS_PIN, LOW);
  digitalWrite(TX_PIN, LOW);
  transmitting = false;
}

void nextLetter() {
  current_bit = -2;
  byte_iterator++;

  if (byte_iterator > text_size-1) {
    finalizeTransmition();
  	return;
  }
  current_letter = text.charAt(byte_iterator);
}

ISR(TIMER1_COMPA_vect) {
  if (!transmitting) return;
  
  int parity_bit = 1;
  int end_bit = 1;
  
  switch(current_bit) {
    case START_BIT:
      digitalWrite(TX_PIN,1);
      Serial.println("Start Bit: 1");
      Serial.print("\nTransmitindo ");
      Serial.println(current_letter);
      break;
  
    case PARITY_BIT:
      digitalWrite(TX_PIN,parity_bit);
      isValid = checkParity(current_letter);
      Serial.print("\nParidade ");
      Serial.println(isValid);
      break;
    
    case END_BIT:
      Serial.print("Bit final ");
      Serial.println(end_bit);
      digitalWrite(TX_PIN, end_bit);
      nextLetter();
      break;
    
    default:
      bool data_bit = bitRead(current_letter, current_bit);
      Serial.print(data_bit);
      digitalWrite(TX_PIN, data_bit);
      break;
  }
  current_bit++; // Increment to next bit position 
}

void setup() {
  noInterrupts();
  Serial.begin(9600);       // Serial monitor baud rate

  pinMode(TX_PIN, OUTPUT);  // Set TX_PIN as output
  pinMode(RTS_PIN, OUTPUT); // Set RTS_PIN as output
  pinMode(CTS_PIN, INPUT);  // Set CTS_PIN as input

  configuraTemporizador(BAUD_RATE);
  
  interrupts();
}

void loop() {
  Serial.println("Insert the text to be transmitted:");
  
  while(text == "") {
    text = Serial.readString();
  }
  
  text_size = text.length();
  byte_iterator = 0;
  current_letter = text[byte_iterator];
  
  // rts para high
  digitalWrite(RTS_PIN, HIGH);
    
  // ver se pode transmitir, esperando a resposta
  while (digitalRead(CTS_PIN) == LOW) {
  }
  transmitting = true;
  transmit();
  
  digitalWrite(RTS_PIN, LOW);
  
  Serial.println("Transmissao concluida");

  text = "";
}
