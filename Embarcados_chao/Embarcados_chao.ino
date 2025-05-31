//------------------------------------------------------------------------
#define F_CPU 16000000UL                  //define a frequência do microcontrolador 16MHz (necessário para usar as rotinas de atraso)
#include <avr/io.h>                       //definições do componente especificado
#include <avr/interrupt.h>
#include <util/delay.h>                   //biblioteca para o uso das rotinas de _delay_ms() e _delay_us()

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
#include <Wire.h>

//Definições de macros - empregadas para o trabalho com os bits
#define set_bit(Y,bit_x) (Y|=(1<<bit_x))  //ativa o bit x da variável Y (coloca em 1)
#define clr_bit(Y,bit_x) (Y&=~(1<<bit_x)) //limpa o bit x da variável Y (coloca em 0)
#define tst_bit(Y,bit_x) (Y&(1<<bit_x))   //testa o bit x da variável Y (retorna 0 ou 1)
#define cpl_bit(Y,bit_x) (Y^=(1<<bit_x))  //troca o estado do bit x da variável Y (complementa)

#define TFT_CS   12
#define TFT_RST  7  // you can also connect this to the Arduino reset
                      // in which case, set this #define pin to 0!
#define TFT_DC   8
#define TFT_SCLK 13   // set these to be whatever pins you like!
#define TFT_MOSI 11   // set these to be whatev er pins you like!
#define BUTTON PD2
#define LDR PD4
#define LED PB1

#define PWM1 PD6  //OCOA
#define PWM2 PD5  //OCOB

const uint8_t PWM_PIN1 = 3;
const uint8_t PWM_PIN2 = 5;

char c;
int aux1, aux2, valorA, valorB, pwm1, pwm2;
String pwmstr, pwm1str, pwm2str, myString;
bool Btemp, Bpres, Btank;
bool Bint1 = true, Bint2 = true;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
//-----------------------------------------------------------------------

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
EICRA = (1 << ISC01) | (0 << ISC00);;
// Habilitar INT0
EIMSK = (1 << INT0);
sei();
}

// Rotina de serviço da interrupção INT0
ISR(INT0_vect) {
  Bint1 = !Bint1;
  //Serial.println("PAROU");
}

void Screen_Init(){
  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
  tft.fillScreen(ST7735_WHITE);
  tft.setRotation(1);
}

void Screen_Print(String txt, int line){
  tft.setRotation(0);
  tft.setTextWrap(false);
  tft.setCursor(2, (line*8)+2);
  tft.setTextColor(ST7735_RED);
  tft.setTextSize(1);
  tft.println(txt);
}

static void Timer0_Init_FastPWM(void)
{
    /* 1) Configura pinos PD6 (OC0A) e PD5 (OC0B) como saída */
    DDRD |= (1 << PD6) | (1 << PD5);

    /* 2) Configura modo Fast PWM 8-bit e não inversor em ambos canais */
    TCCR0A = (1 << WGM01) | (1 << WGM00)  /* WGM01=1,WGM00=1 → Modo 3: Fast PWM 8-bit */
            | (1 << COM0A1) | (0 << COM0A0) /* OC0A não inversor */
            | (1 << COM0B1) | (0 << COM0B0);/* OC0B não inversor */

    /* 3) Prescaler = 64 (CS02=0,CS01=1,CS00=1) */
    TCCR0B = (0 << WGM02)                  /* WGM02=0 → completa Fast PWM 8-bit */
            | (0 << CS02) | (1 << CS01) | (1 << CS00);

    /* 4) Zera comparadores inicialmente (PWMs desligados) */
    OCR0A = 0;
    OCR0B = 0;
}

//-----------------------------------------------------------------------
void setup(void) {

  ADC_Init();
  INT_Init();
  Wire.begin();
  Screen_Init();
  Timer0_Init_FastPWM();

  Serial.begin(9600);

  clr_bit(DDRD, LDR);
  clr_bit(DDRD, BUTTON);
  set_bit(PORTD, BUTTON);
  set_bit(PORTB, LED);

  Serial.println("I am I2C Master");
}

void loop() {

  pwmstr = "";
  pwm1str = "";
  pwm2str = "";
  myString = "";

  Wire.requestFrom(8, 14);
  while(Wire.available()){
    char c = Wire.read();
    pwmstr.concat(c);
  Serial.print(c);
  }
  Serial.println();

  for(int i = 0; i < pwmstr.length(); i++){
    if(pwmstr.charAt(i) == '-'){
      aux1 = i;
    }
    if(pwmstr.charAt(i) == '/'){
      aux2 = i;
    }
  }

  pwm1str = pwmstr.substring(0, (aux1-1));
  pwm2str = pwmstr.substring(aux1+1, aux2-1);
  myString = pwmstr.substring(aux2+2, aux2+3);

  Serial.println("string recebida:");
  Serial.println(myString);

  Serial.println("equals:");
  Serial.println(myString.equals("1"));

  if (myString.equals("1")) {
    Bint2 = true;
  } else {
    Bint2 = false;
  }

  Serial.println("Bint2:");
  Serial.println(Bint2);
  
  valorA = pwm1str.toInt();
  valorB = pwm2str.toInt();

  if (valorA < 0) valorA = 0;
  if (valorA > 1023) valorA = 1023;
  if (valorB < 0) valorB = 0;
  if (valorB > 1023) valorB = 1023;
  pwm1 = (uint8_t)(valorA >> 2);
  pwm2 = (uint8_t)(valorB >> 2);

  if(Btemp && Bpres && Btank && Bint1 && Bint2){
    OCR0A = pwm1;  // PD6
    OCR0B = pwm2;  // PD5
  }
  else{
    OCR0A = 0;  // PD6
    OCR0B = 0;  // PD5
  }

  Serial.println("pwm1:");
  Serial.println(pwm1);
  Serial.println("pwm2:");
  Serial.println(pwm1);

  Screen_Print(Inclinacao(), 0);
  Screen_Print(Tanque(), 1);
  Screen_Print(Temperatura(), 2);
  Screen_Print(Presenca(), 3);

  Send_Data(Inclinacao(), Tanque(), Temperatura(), Presenca());
  delay(1000);
  tft.fillScreen(ST7735_WHITE);
}

//-----------------------------------------------------------------------
String Inclinacao(void){
  int inc = ADC_Read(0);

  if(inc < 50){
    set_bit(PORTB, LED);
    return "Desalinhado para esquerda";
  }
  else if(inc > 950){
    set_bit(PORTB, LED);
    return "Desalinhado para direita";
  }
  clr_bit(PORTB, LED);
  return "Alinhado";
}

String Tanque(void){
  int nivel = ADC_Read(1);

  if(nivel < 10){
    Btank = true;
    return "Tanque cheio";
  }
  else if(nivel > 80){
    Btank = false;
    return "Nivel critico!";
  }
  Btank = true;
  return "Pouco oleo";
}

String Temperatura(void){
  int lm35 = ADC_Read(2);
  float mV_lm35 = lm35 * (5000.0 / 1024.0);
  float temp = mV_lm35/10;

  if(10 < temp < 40){
    Btemp = true;
  }
  else{
    Btemp = false;
  }

  return String(temp) + "C";
}

String Presenca(void){
  if(tst_bit(PIND, LDR) == 16){
    Bpres = false;
    return "Presenca detectada";
  }
  Bpres = true; 
  return "Nao detectada";
}

void Send_Data(String inc, String Tanque, String Temp, String Presenca){
  Wire.beginTransmission(8);
  Wire.write(("Dados do chao: "));
  Wire.endTransmission();

  Wire.beginTransmission(8);
  Wire.write(("Inclinação: " + inc).c_str());
  Wire.endTransmission();

  Wire.beginTransmission(8);
  Wire.write(("Estado do Tanque: " + Tanque).c_str());
  Wire.endTransmission();

  Wire.beginTransmission(8);
  Wire.write(("Temperatura atual: " + Temp).c_str());
  Wire.endTransmission();

  Wire.beginTransmission(8); 
  Wire.write(("Presença: " + Presenca).c_str());
  Wire.endTransmission();

  Wire.beginTransmission(8);
  Wire.write(("======================================="));
  Wire.endTransmission();
}
