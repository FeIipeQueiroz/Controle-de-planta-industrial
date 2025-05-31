# Controle de planta industrial.
Repositório do PBL 2 da disciplina de Sistemas Digitais (2024.2). O objetivo deste projeto é simular o controle de uma planta industrial automatizada para o corte de madeiras, com foco em integração entre sensores, atuadores e comunicação entre microcontroladores.

O sistema é composto por dois Arduinos:
  - Arduino Supervisor: Responsável por controlar a velocidade dos motores de corte, enviar comandos de parada para o Chão de Fábrica, e exibir no monitor serial   informações sobre o funcionamento geral da planta. Ele conta com dois potenciômetros para regular dinamicamente a velocidade dos motores e um interruptor que, ao ser pressionado, envia um comando de parada para o Arduino do Chão de Fábrica. Também é responsável por receber periodicamente dados sobre o status dos sensores e a produção em andamento.
  - Arduino Chão de Fábrica: Atua diretamente sobre o processo de corte da madeira. Com base nos comandos recebidos do Supervisor, ajusta a velocidade dos motores e monitora sensores de temperatura, inclinação, presença humana e nível de óleo.

A comunicação entre os dois Arduinos é feita via protocolo I2C. Essa abordagem permite transmissão eficiente de dados e comandos entre os módulos, com endereçamento específico para evitar conflitos de comunicação.

# Softwares utilizados.
Arduino IDE: desenvolvimento e upload dos códigos para os microcontroladores.

AVR Toolchain: incluído na Arduino IDE, com suporte a bibliotecas como avr/io.h, util/delay.h e avr/interrupt.h.

Bibliotecas Externas:

Wire.h: comunicação I2C.

Adafruit_GFX.h e Adafruit_ST7735.h: para controle do display TFT no módulo Chão de Fábrica.

# Descição do circuito e instalação.

A implementação do circuito é dividida em dois sistemas distintos, representados pelos microcontroladores Arduino 1 (Supervisor) e Arduino 2 (Chão de Fábrica). Abaixo está descrita a conexão dos principais componentes baseando-se na pinagem presente no código:

**Arduino 1 (Supervisor)**

- Pino PD2: Interruptor de parada de emergência (interrupção externa).

- Pino A7: Potenciômetro para controle da velocidade do motor 1.

- Pino A6: Potenciômetro para controle da velocidade do motor 2.

- Pino A4 (SDA) e A5 (SCL): Comunicação I2C com o Arduino 2.

- Porta PD6: LED (indica funcionamento normal da planta).
  
- Porta PD7: LED (indica parada da planta).

**Arduino 2 (Chão de Fábrica)**

- Porta PD2: Interruptor de parada de emergência (interrupção externa).

- Porta A0: Sensor de inclinação (Substituido por um joystick).

- Porta A1: Sensor de nível do tanque de óleo.

- Porta A2: Sensor de temperatura.

- Porta PD4: Sensor de presença.

- Porta PB9: Servo motor para correção da inclinação (Substituido por led na montagem).

- Porta PD6: Motor de corte horizontal.

- Porta PD5: Motor de corte vertical.

- Porta PB0: Pino DC do monitor SPI utilizado.

- Porta PD7: Pino Reset do monitor SPI utilizado.

- Porta PB3: Pino MOSI do monitor SPI utilizado.

- Porta PB4: Pino CS do monitor SPI utilizado.

- Porta PB5: Pino SCLK do monitor SPI utilizado.

- Porta A4 (SDA) e Porta A5 (SCL): Comunicação I2C com o Arduino 1.

# Vídeo do funcionamento
Link para o vídeo no youtube: https://youtu.be/Q-9VHuHNsxI

