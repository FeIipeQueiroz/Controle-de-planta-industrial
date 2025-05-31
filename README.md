# Projeto de Controle de Planta Industrial (PBL 2 – Sistemas Digitais 2025.1)

Este repositório descreve a implementação de um sistema embarcado, composto por dois Arduinos Nano, para controlar automaticamente uma planta de corte de blocos de madeira. A seguir, apresenta-se o README organizado em cinco seções principais:

1. [Introdução](#introdução)  
2. [Supervisor](#supervisor)  
3. [Chão de Fábrica](#chão-de-fábrica)  
4. [Comunicação entre Supervisor e Chão de Fábrica](#comunicação-entre-supervisor-e-chão-de-fábrica)  
5. [Conclusão](#conclusão)  

---

## Introdução

A problemática central deste projeto é desenvolver um sistema embarcado que controle, em tempo real, o processo de corte de blocos de madeira, garantindo segurança e monitoramento contínuo. Em uma planta industrial, múltiplos sensores e atuadores precisam cooperar para que o processo funcione corretamente: verificar condições ambientais, detectar situações de risco, acionar serras elétricas e exibir informações de status ao operador.

Para resolver essa problemática, optou-se por uma arquitetura distribuída, usando dois microcontroladores Arduino Nano interconectados via barramento I²C:

- **Supervisor (Arduino Nano 1)**: responsável por fornecer as referências de velocidade para as serras (por meio de potenciômetros), gerenciar a parada/retomada do processo (botão de emergência) e exibir no monitor serial o estado geral da planta.  
- **Chão de Fábrica (Arduino Nano 2)**: encarregado de monitorar quatro sensores de segurança (temperatura, nível de óleo, presença humana e alinhamento de peça), acionar ou desligar dois motores de corte via sinal PWM conforme os valores recebidos pelo Supervisor, exibir localmente em um display TFT o status de cada sensor e interromper a produção sempre que alguma condição de segurança for violada.

A estratégia básica de funcionamento é:

1. O Supervisor fornece, a cada ciclo, dois valores analógicos (0..1023) vindos de potenciômetros, que determinam o duty cycle dos motores no Chão de Fábrica.  
2. O Chão de Fábrica lê esses valores, converte para 0..255 (8 bits) e, somente se todos os sensores indicarem condições normais, aplica esse PWM aos motores de corte. Se qualquer sensor indicar falha, os motores são desligados instantaneamente.  
3. Enquanto implementa o controle de segurança, o Chão de Fábrica envia ao Supervisor, a cada iteração, strings de texto que detalham o estado de cada sensor. O Supervisor, por sua vez, imprime essas informações no monitor serial, mantendo um registro do comportamento de toda a planta.  

Toda a lógica de leitura analógica, interrupção de emergência, geração de PWM e comunicação I²C foi implementada diretamente em registradores AVR, com o auxílio mínimo de bibliotecas externas apenas para I²C (Wire.h) e para controlar o display TFT (Adafruit_GFX.h, Adafruit_ST7735.h e SPI.h).  

---

## Supervisor

O módulo **Supervisor** (Arduino Nano 1) é responsável por:

- Definir a velocidade das serras elétricas.  
- Controlar remotamente a parada e retomada do processo.  
- Exibir, no monitor serial, o estado de todos os sensores e indicadores enviados pelo Chão de Fábrica.  

### Componentes Conectados ao Supervisor

1. **Botão de Emergência (INT0 – pino PD2 / D2)**  
   - Conectado a um botão tipo “push‑button” cujo outro terminal está ligado a GND junto com um capacitor e uma resistencia para fazer um debouncing.  
   - O Arduino ativa o *pull‑up* interno nesse pino e configura uma interrupção externa para disparar sempre que o botão é pressionado (nível lógico baixo).  
   - Ao receber o pulso de interrupção, o Supervisor alterna uma variável de controle (`State`), que indica se a planta está em operação ou em parada.  

2. **Potenciômetros para Velocidade das Serras (ADC6 – A6 / PC6 e ADC7 – A7 / PC7)**  
   - Dois potenciômetros de 10 kΩ cujas extremidades ficam em +5 V e GND, e o pino central (“wiper”) conectado aos canais analógicos A6 e A7 do Arduino.  
   - A cada leitura analógica, esses valores (0..1023) são transformados em duty cycle 8 bits (0..255) no Chão de Fábrica.  
   - Na prática, um potenciômetro define a velocidade da serra horizontal e o outro define a velocidade da serra vertical.  

3. **LED Indicador “Planta em Operação” (pino PD7 / D7)**  
   - Conectado a um LED em série com resistor.  
   - Permanece aceso enquanto a variável `State` estiver em “planta em operação” (valor lógico 1).  

4. **LED Indicador “Parada Solicitada” (pino PD6 / D6)**  
   - Conectado a um LED em série com resistor (≈ 470 Ω).  
   - Acende-se sempre que `State` estiver em “planta parada” (valor lógico 0).  
   - Dessa forma, o operador visualiza imediatamente se o Supervisão está bloqueando a produção.  

5. **Barramento I²C (pinos A4 =SDA / PC4 e A5 =SCL / PC5)**  
   - O Supervisor usa a biblioteca **Wire.h** para configurar o Arduino como Slave I²C no endereço 8.  
   - Define callbacks para receber (onReceive) e enviar (onRequest) dados ao Master (Chão de Fábrica).  
   - Quando o Chão solicita, o Supervisor retorna uma única string contendo:  
     - O valor bruto lido em A6 (potenciômetro 1).  
     - O valor bruto lido em A7 (potenciômetro 2).  
     - O estado atual de `State` (0 = parada, 1 = operação).  

6. **Monitor Serial (pinos PD0 = RX, PD1 = TX)**  
   - Utilizado para debugar e imprimir todas as mensagens de status enviadas pelo Chão de Fábrica.  
   - Ao receber dados via I²C (na callback de recepção), o Supervisor apenas imprime diretamente cada pacote recebido, que contém descrições de cada sensor (temperatura, nível de óleo, presença, alinhamento) e, ao final, uma linha separadora.  

### Funcionamento Interno do Supervisor

- **Inicialização**  
  1. Configuração de interrupção externa no pino PD2, com *pull‑up* interno ativado.  
  2. Configuração do ADC para referência AVCC (5 V) e prescaler apropriado para obter clock de conversão de 125 kHz.  
  3. Inicialização dos LEDs (PD7 ligado, PD6 apagado) e da comunicação I²C como Slave no endereço 8.  
  4. Inicialização do monitor serial a 9600 bps.  

- **Lógica Principal**  
  - Enquanto não ocorre interrupção ou requisição I²C, o Supervisor mantém os LEDs em acordo com o valor de `State`:  
    - Se `State == 1` (planta em operação), acende o LED “Planta em Operação” (PD7) e apaga o LED “Parada Solicitada” (PD6).  
    - Se `State == 0` (planta parada), o LED “Parada Solicitada” (PD6) permanece aceso e o LED “Planta em Operação” (PD7) apaga.  
  - **Entrada de Interruptor → Saída de LEDs**  
    - Ao pressionar o botão, a interrupção `INT0` dispara, invertendo a variável `State`.  
    - Imediatamente no loop seguinte, observa-se a mudança dos LEDs conforme o novo valor de `State`.  

- **Comunicação I²C**  
  - Quando o Chão de Fábrica executa `Wire.requestFrom(8, 14)`, o callback `onRequest` formata e envia uma string concatenada:  
    ```
    "<ADC6_raw> - <ADC7_raw> / <State>"
    ```
    Exemplo: `“512 - 768 / 1”` (até 14 caracteres).  
  - Quando o Chão envia qualquer string via I²C (`Wire.write(...)` repetidamente), o callback `onReceive` do Supervisor apenas imprime cada byte recebido no monitor serial, gerando linhas como:  
    ```
    Inclinação: Alinhado
    Estado do Tanque: Pouco oleo
    Temperatura atual: 23.4C
    Presença: Nao detectada
    =======================================
    ```
    Esse fluxo de mensagens permite que o Supervisor registre todas as condições de cada sensor no Chão.  

---

## Chão de Fábrica

O módulo **Chão de Fábrica** (Arduino Nano 2) é responsável por:

- Monitorar as condições de segurança (temperatura, nível de óleo, presença humana e alinhamento de peça).  
- Controlar dois motores de corte via PWM conforme os valores de velocidade recebidos pelo Supervisor.  
- Exibir, em tempo real, o estado de cada sensor em um display TFT colorido.  
- Transmitir ao Supervisor, a cada ciclo, descrições de cada sensor.  
- Acionar imediatamente a parada dos motores sempre que alguma condição de segurança for violada (local ou remoto).  

### Componentes Conectados ao Chão de Fábrica

1. **Botão de Emergência Local (INT0 – pino PD2 / D2)**  
   - Conectado a um botão tipo “push‑button” cujo outro terminal vai a GND.  
   - O Arduino ativa *pull‑up* interno nesse pino e define interrupção externa na borda de descida.  
   - A cada pressão do botão, uma flag (`Bint1`) é invertida, indicando se foi solicitada parada manualmente no Chão de Fábrica.  

2. **Sensor de Inclinação (ADC0 – pino A0 / PC0)**    
   - O joystick alimentado com +5V e gnd, com a porta VRx dando o sinal do ADC
   - Em software, lê-se o valor bruto (`0..1023`).  
   - Se o valor ficar abaixo de 50 ou acima de 950, considera-se “desalinhado” (para esquerda ou para direita). Nesse caso, acende-se um LED indicativo e força-se a parada local (`Bint1 = false`).  
   - Se estiver entre 50 e 950, considera-se “alinhado” e o LED permanece apagado.  

3. **Sensor de Nível do Tanque de Óleo (ADC1 – pino A1 / PC1)**  
   - Um sensor que determina a região em contato e determina o nível baseado na variação da resistência do circuito  
   - Conforme o valor bruto lido:  
     - Se < 10, entende-se que o tanque está “cheio” → condição segura.  
     - Se entre 10 e 80, considera-se “nível aceitável” (sem críticas).  
     - Se > 80, interpreta-se como “nível crítico” (alto ou baixo demais) → força parada.  
   - Em cada leitura, o código define a flag `Btank` como **true** (condição segura) ou **false** (condição crítica).  

4. **Sensor de Temperatura (ADC2 – pino A2 / PC2) utilizando LM35**  
   - LM35 é um sensor cuja saída em V é diretamente proporcional à temperatura em °C (10 mV/°C).  
   - Conectado com VCC (+5 V) e GND, e o pino de saída ao pino A2 do Arduino.  
   - No software, converte-se o valor bruto ADC para mV (`ValorADC × (5000 mV / 1024)`), depois divide-se por 10 para obter a temperatura em °C.  
   - Considera-se a faixa segura entre 10 °C e 40 °C. Se estiver fora dessa faixa, define-se `Btemp = false` (parada), caso contrário `Btemp = true`.  

5. **Sensor de Presença Humana (pino PD4 / D4 – leitura digital)**  
   - Consiste de um módulo digital LDR (Light Dependent Resistor) que integra um sensor LDR e um comparador ajustável por um trimpot, fazendo esse conjunto retornar um valor digital.  
   - O pino PD4 é configurado como entrada digital. Quando o pino estiver em nível alto (1), interpreta-se que há presença humana no ambiente (o LDR está escuro), definindo `Bpres = false`.  
   - Quando estiver em nível baixo (0), considera-se que não há presença, definindo `Bpres = true`.  

6. **LED Indicador de Desalinhamento (pino PB1 / D10)**  
   - Conectado a um LED em série com resistor (≈ 1 KΩ).  
   - Se o valor de inclinação lido em A0 estiver fora dos limites (valor < 50 ou > 950), o Arduino acende esse LED para indicar que a peça está desalinhada.  
   - Enquanto esse LED estiver aceso, a flag de parada local (`Bint1`) permanece **false**, garantindo que os motores não ligarão mesmo que outras condições sejam seguras.  

7. **Motores de Corte (pinos PD6 / D6 e PD5 / D5 – OC0A e OC0B)**  
   - Dois motores DC  são conectados aos pinos PD6 e PD5, respectivamente, por meio de uma ponte H.  
   - O Timer0 é configurado em modo Fast PWM 8 bits, gerando um sinal PWM com frequência de aproximadamente 244 Hz, ideal para controle de motores DC convencionais.  
   - O valor de duty cycle aplicado (0..255) é determinado pelos valores recebidos do Supervisor, convertidos de 10 bits (0..1023) para 8 bits (0..255).  
   - Se qualquer condição de parada (sensor ou botão) indicar **false**, define-se ambos os comparadores (`OCR0A` e `OCR0B`) para zero, desligando imediatamente os motores.  

8. **Display TFT Colorido (ST7735 via SPI)**  
   - O display é conectado aos pinos SPI do Arduino Nano:  
     - **PB5 (D13)** → SCK (clock SPI)  
     - **PB3 (D11)** → MOSI (dados SPI)  
     - **PB4 (D12)** → CS (chip select)  
     - **PB0 (D13)** → DC (data/command)  
     - **PD7 (D7)** → RST (reset; deve seguir o esquema físico do módulo)  
   - Usa-se a biblioteca **Adafruit_ST7735** que depende de **Adafruit_GFX** e **SPI.h** para inicializar e desenhar texto.  
   - A cada ciclo, após verificar todas as condições de segurança e ajustar os motores, o código exibe quatro linhas de texto:  
     1. Status de Inclinação (“Alinhado” ou “Desalinhado para esquerda/direita”).  
     2. Estado do Tanque (“Tanque cheio”, “Pouco oleo” ou “Nivel critico!”).  
     3. Temperatura atual (por exemplo, “23.4C”).  
     4. Presença humana (“Nao detectada” ou “Presenca detectada”).  
   - Cada linha é exibida por aproximadamente 1 segundo, depois a tela é limpa para a próxima atualização.  

9. **Barramento I²C (pinos A4 =SDA / PC4 e A5 =SCL / PC5)**  
   - Configurado pela biblioteca **Wire.h** em modo Master (sem endereço).  
   - Em cada iteração do loop, o Chão solicita ao Supervisor os valores de potenciômetros e o estado de parada remoto.  
   - Após processar sensores e motores, o Chão envia **cinco pacotes** I²C de texto ao Supervisor, um para cada informação de sensor e um pacote separador:  
     1. `“Inclinação: <resultado_Inclinacao>”`  
     2. `“Estado do Tanque: <resultado_Tanque>”`  
     3. `“Temperatura atual: <resultado_Temperatura>”`  
     4. `“Presença: <resultado_Presenca>”`  
     5. `“=======================================”`  

10. **Monitor Serial (pinos PD0 = RX, PD1 = TX)**  
    - Usado para debug local: imprime o valor bruto de cada potenciômetro recebido do Supervisor, os valores convertidos de PWM e quaisquer mensagens desejadas pelo desenvolvedor.  

### Funcionamento Interno do Chão de Fábrica

- **Inicialização**  
  1. Configuração do ADC para referência AVCC (5 V) e prescaler adequado.  
  2. Configuração de interrupção externa no pino PD2 (botão local), com *pull‑up* ativado.  
  3. Inicialização do barramento I²C como Master.  
  4. Inicialização do display TFT em modo paisagem, preenchendo a tela de branco.  
  5. Configuração do Timer0 em modo Fast PWM 8 bits, habilitando saídas em PD6 e PD5.  
  6. Definição de pinos de sensores (PD4 como entrada digital, A0/A1/A2 como entradas analógicas) e do LED de desalinhamento (PB1 como saída, inicialmente apagado).  
  7. Inicialização do monitor serial a 9600 bps para debug.  

- **Lógica Principal (loop)**  
  1. **Receber Referências do Supervisor**  
     - Executa `Wire.requestFrom(8, 14)` para ler até 14 bytes da string enviada pelo Supervisor.  
     - A string segue o formato:  
       ```
       "<ADC6_raw> - <ADC7_raw> / <State>"
       ```  
     - Realiza parsing para extrair:  
       - `valorA_raw` (0..1023) → primeiro número (antes de “-”).  
       - `valorB_raw` (0..1023) → segundo número (entre “-” e “/”).  
       - `state_remoto` (0 ou 1) → caractere após “/”.  
     - Define a flag `Bint2 = true` se `state_remoto == 1`, ou `false` se `state_remoto == 0`.  

  2. **Conversão de Valores de PWM**  
     - Converte `valorA_raw` e `valorB_raw` (cada 0..1023) para 8 bits, realizando um deslocamento de 2 bits (equivale a dividir por 4).  
     - O resultado são dois valores `pwm1` e `pwm2` no intervalo [0..255].  

  3. **Leitura de Sensores e Avaliação de Segurança**  
     - **Sensor de Inclinação** (`ADC_Read(0)`):  
       - Se leitura < 50, considera “desalinhado para esquerda”: acende LED PB1 e define `Bint1 = false`.  
       - Se leitura > 950, “desalinhado para direita”: acende LED PB1 e `Bint1 = false`.  
       - Caso contrário, “alinhado”: apaga LED PB1 e `Bint1 = true`.  
     - **Sensor de Nível de Óleo** (`ADC_Read(1)`):  
       - Se leitura < 10, “Tanque cheio”: `Btank = true`.  
       - Se leitura > 80, “Nivel critico!”: `Btank = false`.  
       - Caso contrário, “Pouco oleo”: `Btank = true`.  
     - **Sensor de Temperatura (LM35)** (`ADC_Read(2)`):  
       - Converte para temperatura em °C e define `Btemp = true` se estiver em [10 °C..40 °C], ou `Btemp = false` caso contrário.  
     - **Sensor de Presença** (`digitalRead(PD4)`):  
       - Se PD4 == 1, “Nao detectada”: `Bpres = true`.  
       - Se PD4 == 0, “Presenca detectada”: `Bpres = false`.  

  4. **Controle de Motores via PWM**  
     - Verifica-se o resultado lógico de **todas** as flags:  
       ```
       Btemp  (temperatura segura)
       AND Bpres  (nenhuma presença humana)
       AND Btank  (nível de óleo aceitável)
       AND Bint1  (sem parada local)
       AND Bint2  (sem parada remota)
       ```  
     - Se todas **verdadeiras**, aplica-se `OCR0A = pwm1` (motor vertical) e `OCR0B = pwm2` (motor horizontal).  
     - Se qualquer for **false**, define-se `OCR0A = 0` e `OCR0B = 0`, desligando imediatamente os motores.  

  5. **Atualização do Display TFT**  
     - Limpa a tela e, em cada linha (0..3), escreve:  
       1. **Linha 0**: texto retornado pela função de inclinação (“Alinhado” ou “Desalinhado para esquerda/direita”).  
       2. **Linha 1**: texto sobre o estado do tanque (“Tanque cheio”, “Pouco oleo” ou “Nivel critico!”).  
       3. **Linha 2**: temperatura atual (por exemplo, “23.4C”).  
       4. **Linha 3**: presença humana (“Nao detectada” ou “Presenca detectada”).  
     - Mantém cada tela visível por 1 segundo antes de limpar e redesenhar na próxima iteração.  

  6. **Envio de Dados de Status ao Supervisor**  
     - Em cinco transmissões consecutivas I²C (cada uma em um *paket* isolado), envia:  
       1. `“Inclinação: <resultado_Inclinacao>”`  
       2. `“Estado do Tanque: <resultado_Tanque>”`  
       3. `“Temperatura atual: <resultado_Temperatura>”`  
       4. `“Presença: <resultado_Presenca>”`  
       5. `“=======================================”`  
     - O Supervisor recebe cada string em seu callback `onReceive` e imprime no monitor serial.  

---

## Comunicação entre Supervisor e Chão de Fábrica

A comunicação bidirecional entre Supervisor e Chão de Fábrica é feita exclusivamente pelo barramento **I²C**, configurado em modo Slave (Supervisor) e Master (Chão). A seguir, detalha-se o fluxo:

1. **Endereçamento e Inicialização**  
   - **Supervisor (Slave)**: ao iniciar, chama `Wire.begin(8)`, definindo o endereço 8 no barramento. Registra _callbacks_ para receber (`Wire.onReceive`) e para atender solicitações de dados (`Wire.onRequest`).  
   - **Chão de Fábrica (Master)**: chama `Wire.begin()` sem especificar endereço, atuando como Master.  

2. **Sequência de Vírgula Mestre → Escravo (Chão solicita valores de potenciômetro e estado)**  
   - Em cada iteração do loop do Chão, antes de ler sensores locais, o Master faz:  
     ```  
     Wire.requestFrom(8, 14);  
     ```  
     - Isso gera um evento I²C no Supervisor, que executa seu callback `onRequest`.  
   - **Callback `onRequest` (Supervisor)**:  
     - Lê o valor bruto dos dois potenciômetros conectados a A6 e A7 (cada 0..1023).  
     - Lê a variável `State` (0 ou 1).  
     - Concatena em uma única string no formato:  
       ```
       "<ADC6_raw> - <ADC7_raw> / <State>"
       ```  
     - Envia a string de até 14 bytes ao Master por `Wire.write`.  

   - **Master (Chão)**: lê cada byte recebido da wire e armazena em uma string (`pwmstr`). Após concluir a leitura, realiza parsing para separar:  
     - **`valorA_raw`** (antes de “-”).  
     - **`valorB_raw`** (entre “-” e “/”).  
     - **`state_remoto`** (caractere logo após a barra).  

3. **Processamento de Dados no Chão**  
   - Converte `valorA_raw` e `valorB_raw` para duty cycle de 8 bits.  
   - Define a flag `Bint2 = true` se `state_remoto == 1`, ou `Bint2 = false` se `state_remoto == 0`.  
   - Com isso, o Chão sabe que precisa desligar os motores caso o Supervisor tenha solicitado parada (via `State = 0`).  

4. **Sequência Slave → Master (Chão envia status de sensores)**  
   - Após processar sensores e definir os motores, o Chão transmite, em **cinco pacotes distintos** I²C, as seguintes strings:  
     1. **“Inclinação: <texto_inclinacao>”**  
     2. **“Estado do Tanque: <texto_tanque>”**  
     3. **“Temperatura atual: <texto_temperatura>”**  
     4. **“Presença: <texto_presenca>”**  
     5. **“=======================================”**  

   - O Supervisor, em seu callback `onReceive`, recebe cada pacote completo e, byte a byte, imprime no monitor serial. Isso garante que o operador visualiza, em sequência, todas as condições detectadas no Chão.  

5. **Sincronia e Fluxo Temporal**  
   - A cada iteração do loop do Chão (que ocorre aproximadamente a cada 1 segundo, devido ao delay para atualização do TFT), ocorre:  
     1. **Chão → Supervisor**: requisição I²C (Mestre solicita dados).  
     2. **Supervisor → Chão**: resposta I²C (string com dois valores ADC e estado).  
     3. **Chão processa sensores, ajusta motores e atualiza display**.  
     4. **Chão → Supervisor**: envio em cinco pacotes I²C com descrição dos sensores.  
     5. **Supervisor imprime no monitor serial**.  

   - Assim, estabelece-se um ciclo contínuo de comunicação, leitura de sensores, controle de atuadores e relatório de status, mantendo ambos os Arduinos sincronizados.  

---

## Conclusão

Um vídeo demonstrando o funcionamento pode ser visto abaixo:
https://youtu.be/Q-9VHuHNsxI

Este projeto demonstra a aplicação de conhecimentos avançados de sistemas embarcados em nível de registrador, integrando:

- **Leitura Analógica Direta (ADC)** de múltiplos sensores (LM35, potenciômetros, LDR).  
- **Geração de Sinais PWM** via Timer0 para controlar motos DC.  
- **Interrupções Externas** para parada de emergência em dois pontos distintos (Supervisor e Chão).  
- **Exibição Gráfica** em display TFT usando bibliotecas `Adafruit_GFX` e `Adafruit_ST7735`.  
- **Comunicação I²C Bidirecional** com Arduino configurado como Slave e Master.

Ao separar claramente as responsabilidades em dois módulos (Supervisor e Chão de Fábrica), obteve-se:

- **Modularidade**: cada Arduino trata de um conjunto específico de hardware—o Supervisor lida apenas com referência de velocidade e parada remota, enquanto o Chão gerencia sensores e atuadores.  
- **Segurança e Confiabilidade**: qualquer condição crítica em sensores locais ou botão de emergência (local ou remoto) resulta no corte imediato do sinal PWM, interrompendo a operação das serras.  
- **Monitoramento em Tempo Real**: o Chão exibe localmente o estado de cada sensor no TFT, e o Supervisor mantém registro de todas as leituras no monitor serial, facilitando diagnóstico e análise de falhas.

O uso de programação a nível de registrador (sem abstrações de alto nível, exceto para I²C e display) assegura controle preciso sobre temporizadores, conversores analógico‑digital e pinos de interrupção, refletindo um projeto compatível com requisitos acadêmicos de Sistemas Digitais e Embebidos. Além disso, a topologia de comunicação I²C garante baixo consumo de pinos e fácil escalabilidade para novos sensores ou módulos em futuras etapas do projeto.
