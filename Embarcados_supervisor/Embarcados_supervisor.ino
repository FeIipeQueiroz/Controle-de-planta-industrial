//------------------------------------------------------------------------
#define F_CPU 16000000UL                  //define a frequência do microcontrolador 16MHz (necessário para usar as rotinas de atraso)
#include <avr/io.h>                       //definições do componente especificado
#include <avr/interrupt.h>
#include <util/delay.h>     
#include <Wire.h>

//Definições de macros - empregadas para o trabalho com os bits
#define set_bit(Y,bit_x) (Y|=(1<<bit_x))  //ativa o bit x da variável Y (coloca em 1)
#define clr_bit(Y,bit_x) (Y&=~(1<<bit_x)) //limpa o bit x da variável Y (coloca em 0)
#define tst_bit(Y,bit_x) (Y&(1<<bit_x))   //testa o bit x da variável Y (retorna 0 ou 1)
#define cpl_bit(Y,bit_x) (Y^=(1<<bit_x))  //troca o estado do bit x da variável Y (complementa)

//-----------------------------------------------------------------------

#define BUTTON PD2
#define LED1 PD7
#define LED2 PD6

bool State;
//----------------------------------------------------------------------------

// Função para inicializar ADC
void ADC_Init(void) {
// Referência AVCC
ADMUX = (1 << REFS0);
// Habilitar ADC, prescaler 128
ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

// Função para ler ADC
uint16_t ADC_Read(uint8_t channel) {
// Selecionar canal
ADMUX = (ADMUX & 0xF8) | (channel & 0x07);
// Iniciar conversão
ADCSRA |= (1 << ADSC);
// Aguardar conversão
while (ADCSRA & (1 << ADSC));
return ADC;
}

// Função para configurar interrupção
void INT_Init(void) {
// PD2 (INT0) como entrada com pull-up
DDRD &= ~(1 << PD2);
PORTD |= (1 << PD2);
// Interrupção na borda de descida
EICRA = (1 << ISC01);
// Habilitar INT0
EIMSK = (1 << INT0);
}

// Rotina de serviço da interrupção INT0
ISR(INT0_vect) {
  State = !State;
}

//----------------------------------------------------------------------------

void setup() {

  INT_Init();
  ADC_Init();
  
  State = true;

  set_bit(PORTD, LED1);
  set_bit(PORTD, LED2);

  
  Wire.begin(8);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  clr_bit(DDRD, BUTTON);
  set_bit(PORTD, BUTTON);
  Serial.begin(9600); 
  Serial.println("I am I2C Slave");
}

void loop() {
    if (State) {
    clr_bit(PORTD, LED2);
    set_bit(PORTD, LED1);
  } else {
    clr_bit(PORTD, LED1);
    set_bit(PORTD, LED2);
    }
}

// function that executes whenever data is received from master
void receiveEvent(int howMany) {
  while (0 < Wire.available()) {
    char c = Wire.read();
    Serial.print(c);
  }
  Serial.println();
}

// function that executes whenever data is requested from master
void requestEvent() {
  String txt;
  txt = String(ADC_Read(6)) + " - " + String(ADC_Read(7)) + " / " + String(State);
  //Serial.println(txt);
  Wire.write(txt.c_str());
}