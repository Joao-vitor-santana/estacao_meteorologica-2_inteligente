//VERSÃO 4 modificada pelo claude

/*
  =====================================================================
  MONITOR DE TEMPERATURA E UMIDADE COM ALARME - ESP32
  =====================================================================
  -----------------------------------------------------------------------
  NOVIDADES DESTA VERSÃO (v4) EM RELAÇÃO À v3
  -----------------------------------------------------------------------
  1) CORES CORRIGIDAS (frio = AZUL, calor = VERMELHO):
     Na v3, por engano, o alarme de FRIO usava o LED verde e o de
     UMIDADE ALTA usava o LED azul. Agora: calor = vermelho, frio =
     azul, e a umidade ganhou cores próprias (verde-água / âmbar) que
     não competem visualmente com as de temperatura.

  2) LED RGB COM PWM E MISTURA DE CORES (NOVO):
     O LED deixou de ser só "liga uma cor pura por vez" e passou a usar
     PWM (LEDC do ESP32) nos 3 canais, permitindo misturar cores. Em
     todo alarme (calor, frio, umidade alta, umidade baixa), a cor
     COMEÇA misturada (ex.: laranja) quando mal passou do limite - e o
     buzzer beepa DEVAGAR - e vai se tornando cada vez mais PURA (ex.:
     vermelho 100%) conforme o valor se afasta mais do limite, no
     mesmo ritmo em que o beep/pisca acelera. Ou seja, cor e velocidade
     do alarme sobem juntas, usando a MESMA proporção (0-100%) que já
     controlava a velocidade na v3:
       - CALOR:         laranja  -> vermelho puro
       - FRIO:          ciano    -> azul puro
       - UMIDADE ALTA:  verde-água suave -> verde-água forte/saturado
       - UMIDADE BAIXA: amarelo-areia    -> âmbar forte
     (ver função ledSetColorMisturada() e as paletas COR_* logo acima
     dela, mais abaixo neste arquivo).

  3) ÍCONE DE WI-FI REDESENHADO (MELHORADO):
     O símbolo de "sem wifi" da v3 ficou com um "X" solto e pouco
     reconhecível. Nesta versão o ícone de sinal (fraco/médio/forte)
     ficou mais proporcional entre si (bolinha + arcos concêntricos,
     crescendo de fora para dentro) e o de "sem conexão" agora é o
     símbolo cheio de wifi cortado por uma barra diagonal, no estilo
     usado por Windows/Android - mais fácil de reconhecer de relance.

  Componentes:
    - Display LCD 16x4 (I2C, módulo PCF8574) - via barramento I2C #1 (I2C_LCD)
    - RTC DS3231 (I2C)                        - via barramento I2C #2 (I2C_RTC)
    - Sensor DHT (auto-detectado no boot entre DHT11 / DHT21(AM2301) / DHT22)
    - Buzzer
    - LED RGB (3 pinos: R, G, B) em portas separadas
    - Wi-Fi (interno do ESP32) - usado só para pegar hora/data via NTP

  -----------------------------------------------------------------------
  NOVIDADES DESTA VERSÃO (v3) EM RELAÇÃO À v2s
  -----------------------------------------------------------------------
  1) ALARME BIDIRECIONAL COM VELOCIDADE PROPORCIONAL (CORRIGIDO):
     Na v2 a lógica de alarme ficou quebrada (usava uma variável
     "TEMP_LIMITE" que não existia mais, e uma variável "buzer" que
     nunca foi declarada - o código nem compilava). Nesta versão:
       - Quanto mais QUENTE acima de TEMP_LIMITE_MAX, mais RÁPIDO o LED
         (vermelho) pisca e o buzzer beepa.
       - Quanto mais FRIO abaixo de TEMP_LIMITE_MIN, mais RÁPIDO o LED
         (azul) pisca e o buzzer beepa.
       - A "velocidade máxima" (pisca/beep mais rápido possível) é
         atingida conforme o quanto a temperatura passou do limite,
         controlado por uma ÚNICA variável de 0 a 100:
         ALARME_SENSIBILIDADE_PERCENT (ver explicação detalhada abaixo
         na seção de configuração do alarme).

  2) TELA DE AUTOTESTE "UM COMPONENTE DE CADA VEZ" (NOVO):
     Agora, enquanto testa cada componente, o display mostra CENTRALIZADO
     apenas o nome do componente sendo testado no momento (ex.: "Testando
     Buzzer" enquanto varia a frequência de teste, depois "Buzzer OK").
     Só ao final de todo o autoteste é que aparece a tela-resumo com
     todos os componentes e o status (OK / FALHA) de cada um.

  3) WI-FI + NTP + SINCRONIZAÇÃO CONDICIONAL DO RTC (NOVO):
     No boot, após os testes de hardware, o ESP32 tenta conectar numa
     rede Wi-Fi (SSID/senha configuráveis abaixo). O display mostra
     "Conectando WiFi..." e depois o resultado (OK ou "Sem WiFi").
     Se conectou, busca a hora/data via NTP. Se a diferença entre o
     horário do RTC e o horário da internet for MAIOR que
     RTC_SYNC_DIFERENCA_PERCENT (configurável, começa em 4%), o RTC é
     ajustado automaticamente; caso contrário, o RTC é mantido como
     está. Durante o ajuste, o display mostra centralizado a hora vinda
     da internet e "Configurando RTC", depois volta para a tela normal.
     Se não conseguir conectar ao Wi-Fi, o sistema segue normalmente
     sem sincronizar (o RTC continua sendo usado como fonte de hora).

  4) ÍCONE DE WI-FI NO CANTO SUPERIOR DIREITO (ATUALIZADO):
     Como o LCD é de caracteres (HD44780, sem pixels endereçáveis por
     comando gráfico), o ícone de Wi-Fi é feito com CUSTOM CHARS de
     5x8 pixels gravados na CGRAM do display - desenhados bit a bit,
     com um desenho mais limpo/proporcional (arco + bolinha central,
     no estilo Windows/Android). Como o HD44780 só permite 8 custom
     chars simultâneos, os ícones de intensidade de sinal usam 3 níveis
     (fraco / médio / forte) mais o ícone de "sem wifi" (símbolo com um
     "X" no lugar da bolinha), reaproveitando o mesmo slot de CGRAM
     conforme o RSSI muda.
       - Conectado: ícone de wifi (ondas), com 1 a 3 "barras" preenchidas
         conforme a força do sinal (RSSI), igual ao Windows/Android.
       - Sem conexão: ícone de wifi com um "X" no centro.
       - Fica na última coluna da linha 0 (canto superior direito).

  -----------------------------------------------------------------------
  MANTIDO DA v2
  -----------------------------------------------------------------------
  - Driver manual de LCD HD44780 via PCF8574 (LiquidCrystal_I2C ignora
    TwoWire customizado, então o LCD "ficava mudo" - resolvido com a
    classe Lcd_I2C_Manual que fala direto com o barramento I2C_LCD).
  - Auto-detecção DHT11 / DHT21(AM2301) / DHT22.
  - Autoteste com orçamento total de tempo enxuto.

  -----------------------------------------------------------------------
  BIBLIOTECAS NECESSÁRIAS (Library Manager)
  -----------------------------------------------------------------------
    - RTClib (Adafruit)
    - DHT sensor library (Adafruit)
    - Adafruit Unified Sensor            <-- dependência da DHT sensor library
    - WiFi.h e time.h já vêm no core do ESP32 (não precisa instalar nada)
  (LiquidCrystal_I2C NÃO é necessária - substituída pelo driver manual
   incluído neste próprio arquivo)
  =====================================================================
*/

#include <Wire.h>
#include <RTClib.h>
#include <DHT.h>
#include <WiFi.h>
#include <time.h>

// =====================================================================
// CONFIGURAÇÕES - AJUSTE AQUI
// =====================================================================

// ---- Limite de temperatura MÁXIMA para disparar o alarme (em °C) ----
float TEMP_LIMITE_MAX = 28.0;

// ---- Limite de temperatura MÍNIMA para disparar o alarme (em °C) ----
float TEMP_LIMITE_MIN = 22.0;

// ---- Limite de umidade MÁXIMA para disparar o alarme (em %) ----
float UMID_LIMITE_MAX = 60.0;

// ---- Limite de umidade MÍNIMA para disparar o alarme (em %) ----
float UMID_LIMITE_MIN = 40.0;

// ---- Pino do sensor DHT (11, 21/AM2301 ou 22, detectado automaticamente) ----
#define DHT_PIN   15

// ---- Pino do Buzzer ----
#define BUZZER_PIN 4

// ---- Pinos do LED RGB (R, G, B) ----
#define LED_R_PIN 18
#define LED_G_PIN 5
#define LED_B_PIN 19
// Se o seu LED RGB for de CÁTODO comum, PWM 255 = totalmente aceso (padrão abaixo).
// Se for de ÂNODO comum, inverta a lógica trocando "valor" por "255 - valor" em ledSetColor().
#define LED_CATODO_COMUM true

// ---- PWM do LED RGB via analogWrite (sem LEDC) ----
// O ESP32 com core >= 3.x suporta analogWrite() diretamente nos pinos,
// dispensando ledcSetup/ledcAttachPin/ledcWrite. Resolução padrão: 8 bits (0-255).

// ---- Paletas de cor de cada tipo de alarme (início "fraco" -> fim "no máximo") ----
// Cada alarme vai de uma cor MISTURADA (mal passou do limite, pisca/beep
// lento) até uma cor mais PURA/forte (bem longe do limite, pisca/beep no
// máximo) - controlado por PWM nos 3 canais do LED (0-255 por canal).

// Calor: começa laranja (vermelho + um pouco de verde) e vai virando
// vermelho puro conforme o calor aumenta.
#define COR_CALOR_INICIO_R   255
#define COR_CALOR_INICIO_G   110
#define COR_CALOR_INICIO_B   0
#define COR_CALOR_FIM_R      255
#define COR_CALOR_FIM_G      0
#define COR_CALOR_FIM_B      0

// Frio: começa ciano (verde + azul) e vai virando azul puro conforme
// esfria mais.
#define COR_FRIO_INICIO_R    0
#define COR_FRIO_INICIO_G    200
#define COR_FRIO_INICIO_B    255
#define COR_FRIO_FIM_R       0
#define COR_FRIO_FIM_G       0
#define COR_FRIO_FIM_B       255

// Umidade ALTA (ar muito úmido/"encharcado"): começa num verde-água
// (turquesa suave) e vai virando um verde-água mais forte/saturado -
// remete a "água em excesso" sem se confundir com o vermelho/laranja
// do calor nem com o azul puro do frio.
#define COR_UMID_ALTA_INICIO_R   0
#define COR_UMID_ALTA_INICIO_G   180
#define COR_UMID_ALTA_INICIO_B   130
#define COR_UMID_ALTA_FIM_R      0
#define COR_UMID_ALTA_FIM_G      255
#define COR_UMID_ALTA_FIM_B      90

// Umidade BAIXA (ar seco): começa num amarelo-areia (tom de terra/poeira
// seca) e vai virando um âmbar mais forte - remete a "ressecado", sem
// se confundir com o laranja/vermelho do alarme de calor.
#define COR_UMID_BAIXA_INICIO_R  200
#define COR_UMID_BAIXA_INICIO_G  180
#define COR_UMID_BAIXA_INICIO_B  40
#define COR_UMID_BAIXA_FIM_R     255
#define COR_UMID_BAIXA_FIM_G     140
#define COR_UMID_BAIXA_FIM_B     0

// ---- Barramento I2C #1 -> Display LCD 16x4 ----
#define LCD_SDA_PIN 25
#define LCD_SCL_PIN 26
#define LCD_I2C_ADDR 0x27   // endereço comum; se não funcionar, tente 0x3F
#define LCD_COLUNAS  16
#define LCD_LINHAS   4

// ---- Barramento I2C #2 -> RTC DS3231 ----
#define RTC_SDA_PIN 22
#define RTC_SCL_PIN 21
#define RTC_I2C_ADDR 0x68   // endereço padrão do DS3231

// =====================================================================
// ---- WI-FI + NTP (sincronização do RTC pela internet) ----
// =====================================================================
// Preencha com os dados da sua rede. Deixado como placeholder a pedido.
const char* WIFI_SSID  = "DTEL_LAURINHA";
const char* WIFI_SENHA = "Fisica2026";

// Tempo máximo esperando o Wi-Fi conectar antes de desistir (ms)
#define WIFI_TIMEOUT_MS 10000

// Servidor(es) NTP e fuso horário (Brasília = UTC-3, sem horário de verão)
const char* NTP_SERVIDOR_1 = "pool.ntp.org";
const char* NTP_SERVIDOR_2 = "a.st1.ntp.br";
#define NTP_GMT_OFFSET_SEC      (-3 * 3600)
#define NTP_DAYLIGHT_OFFSET_SEC 0
// Tempo máximo esperando o NTP responder com uma hora válida (ms)
#define NTP_TIMEOUT_MS 8000

// ---- % de diferença entre RTC e hora da internet para forçar ajuste ----
// Se o RTC estiver atrasado (ou adiantado) mais do que esta porcentagem
// em relação ao dia inteiro (24h = 86400s), o RTC é reajustado com a
// hora obtida pela internet. Se estiver dentro dessa margem, o RTC é
// mantido como está. Pode ajustar livremente (ex: 1.0 = mais rígido,
// 10.0 = mais tolerante).
float RTC_SYNC_DIFERENCA_PERCENT = 4.0;

// ---- Parâmetros do alarme de temperatura ----
// Período do PISCA do LED (varia entre esses dois conforme o quanto a
// temperatura passou do limite - quanto mais longe do limite, mais rápido).
#define BLINK_PERIODO_MAX_MS 800
#define BLINK_PERIODO_MIN_MS 120

// ---- Frequências do Buzzer (Hz) - tone()/noTone() ----
// Frequência usada quando o alarme é de temperatura MÍNIMA (frio)
unsigned int BUZZER_FREQUENCIA_TEMP_MIN = 1000;
// Frequência usada quando o alarme é de temperatura MÁXIMA (calor)
unsigned int BUZZER_FREQUENCIA_TEMP_MAX = 2000;
// Frequência usada quando o alarme é de umidade MÍNIMA (seco demais)
unsigned int BUZZER_FREQUENCIA_UMID_MIN = 1500;
// Frequência usada quando o alarme é de umidade MÁXIMA (úmido demais)
unsigned int BUZZER_FREQUENCIA_UMID_MAX = 2500;

// ---- Intervalo do BEEP do buzzer no alarme (em ms) ----
// Igual ao período de pisca do LED: o buzzer beepa no mesmo ritmo em
// que o LED pisca (ligado/desligado juntos), calculado pela mesma
// proporção de "quanto passou do limite".
// (Mantido aqui só como referência do valor "parado"/sem alarme.)
unsigned long BUZZER_BEEP_INTERVALO_MS = 300;

// =====================================================================
// ---- SENSIBILIDADE ÚNICA DO ALARME (0 a 100%) ----
// =====================================================================
// Esta é a variável única pedida: controla o quão rápido a temperatura
// precisa se afastar do limite (MAX ou MIN) para que o LED/buzzer já
// cheguem na velocidade máxima de pisca/beep.
//
//   0%   -> "dura de acelerar": a temperatura precisa passar MUITO do
//           limite (uma faixa larga) para chegar na velocidade máxima.
//   100% -> "fácil de acelerar": basta passar UM POUCO do limite para
//           já ir quase direto para a velocidade máxima.
//
// Internamente isso controla o tamanho da FAIXA DE ACELERAÇÃO (em °C):
// quanto maior a % de sensibilidade, MENOR a faixa necessária para
// atingir a velocidade máxima (e vice-versa).
//
//   FAIXA_ACELERACAO = FAIXA_ACELERACAO_BASE * (1 - sensibilidade/100)
//
// O valor abaixo (60%) foi calculado para reproduzir EXATAMENTE a
// mesma proporção da primeira versão do código, que usava uma faixa
// fixa de aceleração de 10.0 °C com FAIXA_ACELERACAO_BASE = 25.0:
//   25.0 * (1 - 60/100) = 25.0 * 0.40 = 10.0 °C  (igual à v1 original)
float ALARME_SENSIBILIDADE_PERCENT = 60.0;

// Faixa de aceleração usada quando a sensibilidade é 0% (°C que a
// temperatura precisa passar do limite, no cenário "mais difícil de
// acelerar", para chegar à velocidade máxima). Não precisa mexer aqui
// a não ser que quantidade queira recalibrar a escala do slider de 0-100%.
#define ALARME_FAIXA_ACELERACAO_BASE_C 25.0

// Faixa mínima de aceleração permitida, mesmo com sensibilidade = 100%
// (evita divisão por valores absurdamente pequenos / instabilidade).
#define ALARME_FAIXA_ACELERACAO_MINIMA_C 0.5

// ---- Intervalo de leitura do DHT em operação normal ----
#define INTERVALO_LEITURA_DHT_MS 1000

// ---- Parâmetros do autoteste ----
#define TESTE_LED_PISCADAS         2      // piscadas por cor
#define TESTE_LED_PISCA_MS         250    // duração de cada piscada em ms
#define TESTE_BUZZER_BEEPS         2      // quantos beeps o teste do buzzer dá (por frequência)
#define TESTE_BUZZER_BEEP_MS       150    // duração de cada beep do buzzer em ms
// Frequências de teste do buzzer: mostra "frequência x" depois "frequência y"
#define TESTE_BUZZER_FREQUENCIA_X  1200
#define TESTE_BUZZER_FREQUENCIA_Y  2500
#define TESTE_DHT_TENTATIVAS       3      // tentativas de leitura por modelo
#define TESTE_DHT_ESPERA_MS        180    // espera entre tentativas
#define TESTE_DHT_ESTABILIZA_MS    250    // espera após begin() do sensor
#define TESTE_MOSTRA_RESULT_MS     500    // tempo mostrando resultado de cada etapa
#define TESTE_DISPLAY_FLASH_MS     400    // tempo do "flash" de blocos sólidos no LCD

// =====================================================================
// DRIVER MANUAL DE LCD 16x4 VIA PCF8574 (HD44780 sobre I2C)
// =====================================================================
// Implementado à mão porque a LiquidCrystal_I2C padrão ignora o TwoWire
// customizado e sempre usa o barramento Wire global - o que fazia o LCD
// "não mostrar nada" mesmo respondendo ao I2C.
// Fala diretamente com o barramento I2C_LCD passado no construtor.
// Inclui suporte a CUSTOM CHARS (CGRAM) para os ícones de Wi-Fi.

class Lcd_I2C_Manual {
  public:
    Lcd_I2C_Manual(TwoWire &barramento, uint8_t endereco, uint8_t colunas, uint8_t linhas)
      : _wire(barramento), _addr(endereco), _cols(colunas), _rows(linhas), _backlight(0x08) {}

    void begin() {
      delay(50);
      // Sequência de inicialização padrão HD44780 em modo 4 bits
      write4bits(0x03 << 4);
      delayMicroseconds(4500);
      write4bits(0x03 << 4);
      delayMicroseconds(4500);
      write4bits(0x03 << 4);
      delayMicroseconds(150);
      write4bits(0x02 << 4); // modo 4 bits

      command(0x28); // function set: 4 bits, 2 linhas (controlador trata 4 linhas como 2x40 pareadas)
      command(0x08); // display off
      command(0x01); // clear display
      delay(2);
      command(0x06); // entry mode: incrementa cursor, sem shift
      command(0x0C); // display on, cursor off, blink off
    }

    void backlight() {
      _backlight = 0x08;
      expanderWrite(0x00);
    }

    void noBacklight() {
      _backlight = 0x00;
      expanderWrite(0x00);
    }

    void clear() {
      command(0x01);
      delay(2);
    }

    void setCursor(uint8_t col, uint8_t row) {
      // Endereços base para display 16x4 (controladores comuns tratam
      // como duas linhas de 40 caracteres, pareando 1&3 e 2&4)
      static const uint8_t offsets[4] = {0x00, 0x40, 0x14, 0x54};
      if (row >= _rows) row = _rows - 1;
      command(0x80 | (offsets[row] + col));
    }

    void print(const String &texto) {
      for (size_t i = 0; i < texto.length(); i++) {
        writeChar((uint8_t)texto[i]);
      }
    }

    void write(uint8_t valor) {
      writeChar(valor);
    }

    // Grava um padrão de 5x8 pixels num dos 8 slots de CGRAM (0 a 7).
    // 'padrao' precisa ter 8 bytes, cada um usando os 5 bits menos
    // significativos para representar uma linha de pixels (1 = aceso).
    void criarCustomChar(uint8_t slot, const uint8_t padrao[8]) {
      slot &= 0x07; // só existem 8 slots (0-7) no HD44780
      command(0x40 | (slot << 3)); // seleciona endereço da CGRAM
      for (uint8_t linha = 0; linha < 8; linha++) {
        writeChar(padrao[linha]);
      }
      // Depois de escrever na CGRAM, o endereço do DDRAM (cursor normal)
      // fica indefinido - quem chamar isso deve reposicionar com setCursor().
    }

    // Escreve um custom char já gravado (slot 0-7) na posição atual do cursor.
    void escreverCustomChar(uint8_t slot) {
      writeChar(slot & 0x07);
    }

    // Verifica se o dispositivo responde no endereço I2C (usado no autoteste)
    bool respondeI2C() {
      _wire.beginTransmission(_addr);
      return (_wire.endTransmission() == 0);
    }

  private:
    TwoWire &_wire;
    uint8_t _addr;
    uint8_t _cols, _rows;
    uint8_t _backlight;

    void expanderWrite(uint8_t dado) {
      _wire.beginTransmission(_addr);
      _wire.write((uint8_t)(dado | _backlight));
      _wire.endTransmission();
    }

    void pulseEnable(uint8_t dado) {
      expanderWrite(dado | 0x04); // Enable alto
      delayMicroseconds(1);
      expanderWrite(dado & ~0x04); // Enable baixo
      delayMicroseconds(50);
    }

    void write4bits(uint8_t valor) {
      expanderWrite(valor);
      pulseEnable(valor);
    }

    void sendByte(uint8_t valor, uint8_t modo) {
      // modo: 0x00 = comando, 0x01 = dado (RS)
      uint8_t nibbleAlto = valor & 0xF0;
      uint8_t nibbleBaixo = (valor << 4) & 0xF0;
      write4bits(nibbleAlto | modo);
      write4bits(nibbleBaixo | modo);
    }

    void command(uint8_t valor) {
      sendByte(valor, 0x00);
    }

    void writeChar(uint8_t valor) {
      sendByte(valor, 0x01);
    }
};

// =====================================================================
// ÍCONES DE WI-FI (CUSTOM CHARS 5x8, DESENHADOS PIXEL A PIXEL)
// =====================================================================
// Cada linha do array é uma linha de pixels do caractere (5 bits usados,
// os 3 bits mais altos são ignorados pelo HD44780). '1' = pixel aceso.
//
// O HD44780 só guarda 8 desenhos por vez (slots 0-7 da CGRAM). Como
// usamos poucos ícones (sem-wifi + 3 níveis de sinal), gravamos todos
// os 4 de uma vez em slots fixos no boot e trocamos qual é DESENHADO
// na tela (não qual está gravado) conforme o RSSI muda.
#define WIFI_SLOT_SEM_SINAL   0   // símbolo de wifi cortado (sem conexão)
#define WIFI_SLOT_FRACO       1   // 1 barra preenchida
#define WIFI_SLOT_MEDIO       2   // 2 barras preenchidas
#define WIFI_SLOT_FORTE       3   // 3 barras preenchidas (sinal cheio)

// Ícone "sem wifi": o símbolo completo de wifi (3 arcos + bolinha
// central) com uma barra diagonal cortando por cima, no estilo do
// ícone "wifi desligado/sem conexão" do Windows/Android - mais
// reconhecível do que um "X" solto no meio do desenho.
const uint8_t ICONE_WIFI_SEM_SINAL[8] = {
  0b00001,
  0b01110,
  0b00010,
  0b01101,
  0b10101,
  0b01011,
  0b00100,
  0b10000
};

// Ícone wifi com 1 barra (sinal fraco): só a bolinha central acesa,
// arcos apagados - proporcional aos outros dois níveis.
const uint8_t ICONE_WIFI_FRACO[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00100,
  0b01110,
  0b01110
};

// Ícone wifi com 2 barras (sinal médio): bolinha central + arco interno.
const uint8_t ICONE_WIFI_MEDIO[8] = {
  0b00000,
  0b00000,
  0b01110,
  0b10001,
  0b00000,
  0b00100,
  0b01110,
  0b01110
};

// Ícone wifi com 3 barras (sinal forte): os três arcos concêntricos +
// bolinha central, formando o símbolo clássico de wifi bem centralizado
// e proporcional no caractere 5x8.
const uint8_t ICONE_WIFI_FORTE[8] = {
  0b11111,
  0b00000,
  0b01110,
  0b10001,
  0b00000,
  0b00100,
  0b01110,
  0b01110
};

// =====================================================================
// OBJETOS GLOBAIS
// =====================================================================

TwoWire I2C_LCD = TwoWire(0);   // Barramento I2C #1 -> Display (pinos 25/26)
TwoWire I2C_RTC = TwoWire(1);   // Barramento I2C #2 -> RTC (pinos 22/21)

RTC_DS3231 rtc;
Lcd_I2C_Manual lcd(I2C_LCD, LCD_I2C_ADDR, LCD_COLUNAS, LCD_LINHAS);

// Objeto DHT criado dinamicamente após a detecção do modelo no autoteste
DHT *dht = nullptr;
uint8_t modeloDhtDetectado = 0; // 0 = nenhum, 11 = DHT11, 21 = DHT21/AM2301, 22 = DHT22

// =====================================================================
// VARIÁVEIS DE ESTADO
// =====================================================================

float temperaturaAtual = 0.0;
float umidadeAtual = 0.0;
bool  leituraDhtValida = false;

unsigned long ultimaLeituraDht = 0;
unsigned long ultimoToggleAlarme = 0;
bool alarmeLedLigado = false;
bool emAlarme = false;

// Tipo de alarme ativo no momento: 0 = nenhum, 1 = temp. máxima (calor), 2 = temp. mínima (frio)
uint8_t tipoAlarmeAtivo = 0;

// ---- Alarme de UMIDADE (LED azul + buzzer, independente do de temperatura) ----
unsigned long ultimoToggleAlarmeUmid = 0;
bool alarmeLedUmidLigado = false;
bool emAlarmeUmid = false;

// Tipo de alarme de umidade ativo: 0 = nenhum, 1 = umid. máxima (úmido), 2 = umid. mínima (seco)
uint8_t tipoAlarmeUmidAtivo = 0;

// =====================================================================
// ---- TELA DEDICADA DE ALARME (alterna com a tela normal a cada 3s) ----
// =====================================================================
// Enquanto algum alarme (temperatura OU umidade) estiver ativo, o
// display alterna, a cada ALARME_TELA_INTERVALO_MS, entre a tela normal
// (data/hora/temp/umid) e uma tela exclusiva do alarme: só o nome do
// alarme + valor, centralizado, com "?" antes e depois piscando no
// mesmo ritmo do LED/buzzer. O alarme em si (LED/buzzer) NÃO pausa
// nunca, mesmo quando a tela normal está sendo mostrada.
#define ALARME_TELA_INTERVALO_MS 3000
unsigned long ultimaTrocaTelaAlarme = 0;
bool mostrandoTelaAlarme = false; // false = tela normal, true = tela de alarme

// ---- Beep horário (1 beep de 1,5s a cada hora cheia, minuto=0 e segundo=0) ----
#define BEEP_HORARIO_DURACAO_MS 1500
#define BEEP_HORARIO_FREQUENCIA 2000
int8_t ultimaHoraBeepHorario = -1; // hora (0-23) em que o beep horário já foi disparado; -1 = nenhuma ainda

// Flags do resultado do autoteste (para relatório final em caso de falha ou tela-resumo)
bool okDisplay = false;
bool okRTC = false;
bool okDHT = false;
bool okLEDs = false;
bool okBuzzer = false;

// Estado do Wi-Fi (para exibir o ícone durante a operação normal)
bool wifiConectado = false;
int8_t wifiRssiDbm = 0;

// =====================================================================
// PROTÓTIPOS
// =====================================================================
void lerSensores();
void atualizarDisplay(const DateTime &agora);
void atualizarAlarme();
void atualizarAlarmeUmidade();
void ledOff();
void ledSetColor(bool r, bool g, bool b);
void ledSetColorPWM(uint8_t r, uint8_t g, uint8_t b);
void ledSetColorMisturada(uint8_t rInicio, uint8_t gInicio, uint8_t bInicio,
                           uint8_t rFim, uint8_t gFim, uint8_t bFim,
                           float proporcao);
unsigned long calcularPeriodoAlarme(float temperatura);
float calcularProporcaoAlarme(float excedente);

bool i2cDispositivoResponde(TwoWire &barramento, uint8_t endereco);
void scanI2C(TwoWire &barramento, const char* nomeBarramento);
bool testeDisplay();
bool testeRTC();
bool testeDHT();
bool testeLEDs();
bool testeBuzzer();
void telaFalhaGeral();
void telaResumoFinal();
void lcdMsg(uint8_t linha, const String &texto);
void lcdMsgCentralizada(uint8_t linha, const String &texto);
void lcdClearLinha(uint8_t linha);
void lcdClearTudo();

void telaAlarmeCheia();
void verificarBeepHorario(const DateTime &agora);

void conectarWifiESincronizarRTC();
void iniciarIconesWifi();
uint8_t slotWifiPelaForcaDoSinal(int32_t rssi);
void desenharIconeWifiCantoSuperior();

// =====================================================================
// SETUP
// =====================================================================
void setup() {
  Serial.begin(115200);
  delay(300);

  unsigned long inicioAutoteste = millis(); // usado só para log do tempo total no fim

  // Pino do Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);

  // LED RGB via PWM (agora usando analogWrite, sem LEDC) - permite
  // misturar cores (laranja, ciano, etc.), em vez de só ligado/desligado
  // por canal.
  pinMode(LED_R_PIN, OUTPUT);
  pinMode(LED_G_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);
  ledOff();

  // Inicializa os dois barramentos I2C com seus pinos dedicados
  I2C_LCD.begin(LCD_SDA_PIN, LCD_SCL_PIN, 100000);
  I2C_RTC.begin(RTC_SDA_PIN, RTC_SCL_PIN, 100000);

  Serial.println("=== INICIANDO AUTOTESTE ===");

  // Scan completo dos dois barramentos ANTES de qualquer teste,
  // para sabermos exatamente o que está fisicamente respondendo.
  scanI2C(I2C_LCD, "I2C_LCD (barramento do Display)");
  scanI2C(I2C_RTC, "I2C_RTC (barramento do RTC)");

  // ---------------------------------------------------------------
  // TESTE 1: DISPLAY (precisa passar para os demais testes aparecerem nele)
  // ---------------------------------------------------------------
  okDisplay = testeDisplay();
  if (!okDisplay) {
    // Sem display funcionando, só temos a Serial para reportar.
    Serial.println("FALHA: Display LCD nao respondeu no barramento I2C.");
    Serial.println("Sistema parado. Verifique fiacao/endereco do LCD.");
    while (true) {
      delay(1000); // trava aqui, não faz sentido seguir sem display
    }
  }

  // Grava os 4 ícones de wifi na CGRAM assim que o display está OK.
  iniciarIconesWifi();

  lcd.clear();
  lcdMsgCentralizada(1, "Display");
  lcdMsgCentralizada(2, "OK");
  delay(TESTE_MOSTRA_RESULT_MS);

  // ---------------------------------------------------------------
  // TESTE 2: RTC - mostra SÓ o nome do componente sendo testado,
  // depois OK/FALHA, antes de seguir para o próximo.
  // ---------------------------------------------------------------
  lcdClearTudo();
  lcdMsgCentralizada(1, "Testando RTC");
  okRTC = testeRTC();

  lcdClearTudo();
  if (okRTC) {
    lcdMsgCentralizada(1, "RTC");
    lcdMsgCentralizada(2, "OK");
  } else {
    lcdMsgCentralizada(1, "RTC");
    lcdMsgCentralizada(2, "FALHA");
  }
  delay(TESTE_MOSTRA_RESULT_MS);

  if (!okRTC) {
    telaFalhaGeral();
  }

  // ---------------------------------------------------------------
  // TESTE 3: DHT (auto-detecção DHT22 -> DHT21/AM2301 -> DHT11)
  // ---------------------------------------------------------------
  lcdClearTudo();
  lcdMsgCentralizada(1, "Testando DHT");
  okDHT = testeDHT();

  lcdClearTudo();
  if (okDHT) {
    lcdMsgCentralizada(1, "DHT");
    lcdMsgCentralizada(2, "OK");
  } else {
    lcdMsgCentralizada(1, "DHT");
    lcdMsgCentralizada(2, "FALHA");
  }
  delay(TESTE_MOSTRA_RESULT_MS);

  if (!okDHT) {
    telaFalhaGeral();
  }

  // ---------------------------------------------------------------
  // TESTE 4: LEDs (R, G, B piscando, um de cada vez - só o nome do
  // componente "LED" aparece, sem misturar com outro teste na tela)
  // ---------------------------------------------------------------
  lcdClearTudo();
  lcdMsgCentralizada(1, "Testando LED");
  okLEDs = testeLEDs();

  lcdClearTudo();
  lcdMsgCentralizada(1, "LED");
  lcdMsgCentralizada(2, okLEDs ? "OK" : "FALHA");
  delay(TESTE_MOSTRA_RESULT_MS);

  if (!okLEDs) {
    telaFalhaGeral();
  }

  // ---------------------------------------------------------------
  // TESTE 5: Buzzer (separado do teste de LEDs) - mostra só "Buzzer"
  // ---------------------------------------------------------------
  lcdClearTudo();
  lcdMsgCentralizada(1, "Testando Buzzer");
  okBuzzer = testeBuzzer();

  lcdClearTudo();
  lcdMsgCentralizada(1, "Buzzer");
  lcdMsgCentralizada(2, okBuzzer ? "OK" : "FALHA");
  delay(TESTE_MOSTRA_RESULT_MS);

  if (!okBuzzer) {
    telaFalhaGeral();
  }

  // ---------------------------------------------------------------
  // TODOS OS TESTES PASSARAM -> tela-resumo com todos os componentes
  // ---------------------------------------------------------------
  unsigned long duracaoAutotesteMs = millis() - inicioAutoteste;
  Serial.print("=== AUTOTESTE OK - duracao total: ");
  Serial.print(duracaoAutotesteMs);
  Serial.println(" ms ===");

  telaResumoFinal();

  // ---------------------------------------------------------------
  // WI-FI + NTP: conecta, mostra resultado, e sincroniza o RTC se a
  // diferença passar de RTC_SYNC_DIFERENCA_PERCENT.
  // ---------------------------------------------------------------
  conectarWifiESincronizarRTC();

  lcd.clear();
  lcdMsgCentralizada(1, "Iniciando...");
  delay(600);
  lcd.clear();
}

// =====================================================================
// LOOP PRINCIPAL (só é alcançado se todos os testes passaram)
// =====================================================================
void loop() {
  unsigned long agoraMs = millis();

  if (agoraMs - ultimaLeituraDht >= INTERVALO_LEITURA_DHT_MS) {
    ultimaLeituraDht = agoraMs;
    lerSensores();
  }

  DateTime agora = rtc.now();

  // O alarme (LED/buzzer) é sempre atualizado primeiro e continua
  // piscando/beepando normalmente, independente de qual tela está
  // sendo mostrada no momento.
  atualizarAlarme();
  atualizarAlarmeUmidade();

  bool algumAlarmeAtivo = emAlarme || emAlarmeUmid;

  if (!algumAlarmeAtivo) {
    // Sem alarme: sempre tela normal, e zera o controle de alternância
    // para que, na próxima vez que um alarme começar, ele sempre comece
    // mostrando a tela normal antes de ir para a tela de alarme.
    mostrandoTelaAlarme = false;
    ultimaTrocaTelaAlarme = agoraMs;
    atualizarDisplay(agora);
    desenharIconeWifiCantoSuperior();
  } else {
    // Com alarme ativo: alterna entre tela normal e tela de alarme a
    // cada ALARME_TELA_INTERVALO_MS (3s), mas o LED/buzzer do alarme
    // continua ligado o tempo todo, mesmo na tela normal.
    if (agoraMs - ultimaTrocaTelaAlarme >= ALARME_TELA_INTERVALO_MS) {
      ultimaTrocaTelaAlarme = agoraMs;
      mostrandoTelaAlarme = !mostrandoTelaAlarme;
    }

    if (mostrandoTelaAlarme) {
      telaAlarmeCheia();
    } else {
      atualizarDisplay(agora);
      desenharIconeWifiCantoSuperior();
    }
  }

  verificarBeepHorario(agora);

  delay(50);
}

// =====================================================================
// FUNÇÕES DE AUTOTESTE
// =====================================================================

// Verifica se algum dispositivo responde no endereço I2C informado
bool i2cDispositivoResponde(TwoWire &barramento, uint8_t endereco) {
  barramento.beginTransmission(endereco);
  uint8_t erro = barramento.endTransmission();
  return (erro == 0);
}

// Varre todos os endereços I2C (1 a 126) no barramento informado e imprime
// na Serial quais responderam. Ajuda a confirmar fiação/endereço real.
void scanI2C(TwoWire &barramento, const char* nomeBarramento) {
  Serial.print("--- Scan I2C: ");
  Serial.print(nomeBarramento);
  Serial.println(" ---");

  int encontrados = 0;
  for (uint8_t endereco = 1; endereco < 127; endereco++) {
    barramento.beginTransmission(endereco);
    uint8_t erro = barramento.endTransmission();
    if (erro == 0) {
      Serial.print("  Dispositivo encontrado em 0x");
      if (endereco < 16) Serial.print("0");
      Serial.println(endereco, HEX);
      encontrados++;
    }
  }

  if (encontrados == 0) {
    Serial.println("  Nenhum dispositivo encontrado neste barramento!");
  } else {
    Serial.print("  Total encontrado: ");
    Serial.println(encontrados);
  }
}

// Testa o display: primeiro confirma via I2C que o endereço responde,
// depois inicializa (driver manual, barramento dedicado) e escreve um
// padrão de blocos cheios bem visível em todas as 4 linhas.
bool testeDisplay() {
  Serial.print("Testando Display LCD (0x");
  Serial.print(LCD_I2C_ADDR, HEX);
  Serial.println(")...");

  if (!i2cDispositivoResponde(I2C_LCD, LCD_I2C_ADDR)) {
    Serial.println("  -> Nenhum dispositivo respondeu nesse endereco I2C.");
    Serial.println("  -> Confira o resultado do scan acima: se o LCD apareceu");
    Serial.println("     em outro endereco (ex: 0x3F), ajuste LCD_I2C_ADDR.");
    return false;
  }

  lcd.begin();
  delay(100); // folga para o PCF8574 estabilizar
  lcd.backlight();
  delay(50);
  lcd.clear();
  delay(30);

  // Escreve um padrão de blocos cheios (caractere 255/0xFF) em todas as
  // posições das 4 linhas - impossível de não ver se o display funciona.
  for (uint8_t linha = 0; linha < LCD_LINHAS; linha++) {
    lcd.setCursor(0, linha);
    for (uint8_t coluna = 0; coluna < LCD_COLUNAS; coluna++) {
      lcd.write(255);
    }
  }

  Serial.println("  -> Display respondeu ao I2C e comando de escrita enviado.");
  delay(TESTE_DISPLAY_FLASH_MS);

  lcd.clear();
  return true;
}

// Testa o RTC: inicializa no barramento dedicado e verifica se responde.
bool testeRTC() {
  Serial.print("Testando RTC DS3231 (0x");
  Serial.print(RTC_I2C_ADDR, HEX);
  Serial.println(")...");

  if (!i2cDispositivoResponde(I2C_RTC, RTC_I2C_ADDR)) {
    Serial.println("  -> Nenhum dispositivo respondeu nesse endereco I2C.");
    return false;
  }

  if (!rtc.begin(&I2C_RTC)) {
    Serial.println("  -> rtc.begin() falhou.");
    return false;
  }

  if (rtc.lostPower()) {
    Serial.println("  -> RTC sem energia - ajustando para data/hora de compilacao.");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  Serial.println("  -> RTC respondeu e foi inicializado.");
  return true;
}

// Testa o sensor DHT tentando os modelos possíveis, na ordem
// DHT22 -> DHT21 (AM2301) -> DHT11. O primeiro que conseguir uma leitura
// válida (não-NaN) é o modelo assumido pelo sistema daqui em diante.
bool tentarModeloDht(uint8_t tipoDht, const char* nome) {
  Serial.print("  -> Tentando como ");
  Serial.print(nome);
  Serial.println("...");

  if (dht != nullptr) {
    delete dht;
    dht = nullptr;
  }
  dht = new DHT(DHT_PIN, tipoDht);
  dht->begin();
  delay(TESTE_DHT_ESTABILIZA_MS);

  for (int tentativa = 1; tentativa <= TESTE_DHT_TENTATIVAS; tentativa++) {
    float umid = dht->readHumidity();
    float temp = dht->readTemperature();

    if (!isnan(umid) && !isnan(temp)) {
      temperaturaAtual = temp;
      umidadeAtual = umid;
      leituraDhtValida = true;
      Serial.print("     Leitura OK como ");
      Serial.print(nome);
      Serial.print(": Temp=");
      Serial.print(temp);
      Serial.print("C Umid=");
      Serial.print(umid);
      Serial.println("%");
      return true;
    }

    Serial.print("     Tentativa ");
    Serial.print(tentativa);
    Serial.println(" falhou (leitura invalida/NaN).");
    delay(TESTE_DHT_ESPERA_MS);
  }

  return false;
}

bool testeDHT() {
  Serial.print("Testando sensor DHT no GPIO ");
  Serial.print(DHT_PIN);
  Serial.println(" (auto-deteccao DHT22 -> DHT21/AM2301 -> DHT11)...");

  if (tentarModeloDht(DHT22, "DHT22")) {
    modeloDhtDetectado = 22;
    return true;
  }

  if (tentarModeloDht(DHT21, "DHT21/AM2301")) {
    modeloDhtDetectado = 21;
    return true;
  }

  if (tentarModeloDht(DHT11, "DHT11")) {
    modeloDhtDetectado = 11;
    return true;
  }

  Serial.println("  -> Nenhum dos modelos (DHT22/DHT21/DHT11) respondeu.");
  modeloDhtDetectado = 0;
  return false;
}

// Testa os LEDs R, G, B piscando cada um TESTE_LED_PISCADAS vezes.
// O buzzer NÃO participa deste teste. Enquanto testa, o display mostra
// só "LED" (linha 1) + a cor atual (linha 2) - nenhum outro componente
// aparece na tela ao mesmo tempo.
bool testeLEDs() {
  Serial.println("Testando LEDs RGB...");

  bool cores[3][3] = {
    {true, false, false},  // Vermelho
    {false, true, false},  // Verde
    {false, false, true}   // Azul
  };
  const char* nomes[3] = {"Vermelho", "Verde", "Azul"};

  for (int c = 0; c < 3; c++) {
    Serial.print("  -> Piscando ");
    Serial.println(nomes[c]);

    lcdClearLinha(2);
    lcdMsgCentralizada(2, nomes[c]);

    for (int p = 0; p < TESTE_LED_PISCADAS; p++) {
      ledSetColor(cores[c][0], cores[c][1], cores[c][2]);
      delay(TESTE_LED_PISCA_MS);
      ledOff();
      delay(TESTE_LED_PISCA_MS);
    }
  }

  // Este teste não tem uma forma de "falhar" eletricamente sem sensor de
  // corrente, então consideramos OK se conseguiu executar a sequência.
  return true;
}

// Testa o Buzzer separadamente dos LEDs, usando tone()/noTone(). Toca
// primeiro a frequência X, depois a frequência Y, mostrando qual
// frequência está tocando no momento - sem misturar com outro componente.
bool testeBuzzer() {
  Serial.println("Testando Buzzer...");

  // --- Frequência X ---
  lcdClearLinha(2);
  lcdMsgCentralizada(2, "Frequencia X");
  Serial.print("  -> Frequencia X (");
  Serial.print(TESTE_BUZZER_FREQUENCIA_X);
  Serial.println(" Hz)");
  for (int b = 0; b < TESTE_BUZZER_BEEPS; b++) {
    tone(BUZZER_PIN, TESTE_BUZZER_FREQUENCIA_X);
    delay(TESTE_BUZZER_BEEP_MS);
    noTone(BUZZER_PIN);
    delay(TESTE_BUZZER_BEEP_MS);
  }

  // --- Frequência Y ---
  lcdClearLinha(2);
  lcdMsgCentralizada(2, "Frequencia Y");
  Serial.print("  -> Frequencia Y (");
  Serial.print(TESTE_BUZZER_FREQUENCIA_Y);
  Serial.println(" Hz)");
  for (int b = 0; b < TESTE_BUZZER_BEEPS; b++) {
    tone(BUZZER_PIN, TESTE_BUZZER_FREQUENCIA_Y);
    delay(TESTE_BUZZER_BEEP_MS);
    noTone(BUZZER_PIN);
    delay(TESTE_BUZZER_BEEP_MS);
  }

  return true;
}

// Mostra no display quais componentes falharam e trava o sistema ali.
void telaFalhaGeral() {
  lcd.clear();
  lcdMsgCentralizada(0, "FALHA NO TESTE!");

  uint8_t linha = 1;
  if (!okRTC && linha <= 3)   { lcdMsg(linha++, "- RTC nao OK"); }
  if (!okDHT && linha <= 3)   { lcdMsg(linha++, "- DHT nao OK"); }
  if (!okLEDs && linha <= 3)  { lcdMsg(linha++, "- LEDs nao OK"); }
  if (!okBuzzer && linha <= 3){ lcdMsg(linha++, "- Buzzer nao OK"); }

  Serial.println("=== SISTEMA PARADO - FALHA NO AUTOTESTE ===");
  Serial.print("Display: "); Serial.println(okDisplay ? "OK" : "FALHA");
  Serial.print("RTC:     "); Serial.println(okRTC ? "OK" : "FALHA");
  Serial.print("DHT:     "); Serial.println(okDHT ? "OK" : "FALHA");
  Serial.print("LEDs:    "); Serial.println(okLEDs ? "OK" : "FALHA");
  Serial.print("Buzzer:  "); Serial.println(okBuzzer ? "OK" : "FALHA");

  while (true) {
    delay(1000); // trava aqui - não inicia operação normal
  }
}

// Mostra, ao final de TODOS os testes terem passado, um resumo com o
// nome de cada componente testado e seu status (OK ou FALHA).
void telaResumoFinal() {
  lcd.clear();
  lcdMsgCentralizada(0, "Resumo do teste");

  char buf[17];
  snprintf(buf, sizeof(buf), "Disp:%s RTC:%s", okDisplay ? "OK" : "X", okRTC ? "OK" : "X");
  lcdMsg(1, buf);

  snprintf(buf, sizeof(buf), "DHT:%s LED:%s", okDHT ? "OK" : "X", okLEDs ? "OK" : "X");
  lcdMsg(2, buf);

  snprintf(buf, sizeof(buf), "Buzzer:%s", okBuzzer ? "OK" : "FALHA");
  lcdMsg(3, buf);

  Serial.println("=== RESUMO FINAL DO AUTOTESTE ===");
  Serial.print("Display: "); Serial.println(okDisplay ? "OK" : "FALHA");
  Serial.print("RTC:     "); Serial.println(okRTC ? "OK" : "FALHA");
  Serial.print("DHT:     "); Serial.println(okDHT ? "OK" : "FALHA");
  Serial.print("LEDs:    "); Serial.println(okLEDs ? "OK" : "FALHA");
  Serial.print("Buzzer:  "); Serial.println(okBuzzer ? "OK" : "FALHA");

  delay(1500);
}

// =====================================================================
// HELPERS DE LCD
// =====================================================================
void lcdMsg(uint8_t linha, const String &texto) {
  if (linha >= LCD_LINHAS) return;
  lcd.setCursor(0, linha);
  lcd.print(texto);
}

// Mostra um texto CENTRALIZADO na linha (preenche o resto com espaços
// para apagar qualquer resíduo de texto anterior na mesma linha).
void lcdMsgCentralizada(uint8_t linha, const String &texto) {
  if (linha >= LCD_LINHAS) return;

  String textoCortado = texto;
  if (textoCortado.length() > LCD_COLUNAS) {
    textoCortado = textoCortado.substring(0, LCD_COLUNAS);
  }

  uint8_t espacoLivre = LCD_COLUNAS - textoCortado.length();
  uint8_t margemEsquerda = espacoLivre / 2;

  String linhaFinal = "";
  for (uint8_t i = 0; i < margemEsquerda; i++) linhaFinal += " ";
  linhaFinal += textoCortado;
  while (linhaFinal.length() < LCD_COLUNAS) linhaFinal += " ";

  lcd.setCursor(0, linha);
  lcd.print(linhaFinal);
}

void lcdClearLinha(uint8_t linha) {
  if (linha >= LCD_LINHAS) return;
  lcd.setCursor(0, linha);
  lcd.print("                "); // 16 espaços
}

void lcdClearTudo() {
  for (uint8_t linha = 0; linha < LCD_LINHAS; linha++) {
    lcdClearLinha(linha);
  }
}

// =====================================================================
// ÍCONES DE WI-FI (CGRAM) - GRAVAÇÃO E DESENHO
// =====================================================================

// Grava os 4 desenhos (sem sinal / fraco / médio / forte) na CGRAM do
// LCD, cada um no seu slot fixo. Chamado uma única vez, logo após o
// display passar no autoteste.
void iniciarIconesWifi() {
  lcd.criarCustomChar(WIFI_SLOT_SEM_SINAL, ICONE_WIFI_SEM_SINAL);
  lcd.criarCustomChar(WIFI_SLOT_FRACO, ICONE_WIFI_FRACO);
  lcd.criarCustomChar(WIFI_SLOT_MEDIO, ICONE_WIFI_MEDIO);
  lcd.criarCustomChar(WIFI_SLOT_FORTE, ICONE_WIFI_FORTE);
}

// Converte a força do sinal Wi-Fi (RSSI, em dBm - valores tipicamente
// entre -30 [ótimo] e -90 [péssimo]) no slot de ícone correspondente,
// do mesmo jeito que Windows/Android fazem com as "barrinhas":
//   RSSI >= -60 dBm  -> sinal forte  (3 barras)
//   RSSI >= -75 dBm  -> sinal médio  (2 barras)
//   RSSI <  -75 dBm  -> sinal fraco  (1 barra)
uint8_t slotWifiPelaForcaDoSinal(int32_t rssi) {
  if (rssi >= -60) return WIFI_SLOT_FORTE;
  if (rssi >= -75) return WIFI_SLOT_MEDIO;
  return WIFI_SLOT_FRACO;
}

// Desenha o ícone de Wi-Fi no canto superior DIREITO (última coluna,
// linha 0) durante a operação normal: ícone cheio (com barras conforme
// o RSSI) se conectado, ou o ícone cortado se não há conexão.
void desenharIconeWifiCantoSuperior() {
  uint8_t slot;
  if (wifiConectado) {
    slot = slotWifiPelaForcaDoSinal(wifiRssiDbm);
  } else {
    slot = WIFI_SLOT_SEM_SINAL;
  }

  lcd.setCursor(LCD_COLUNAS - 1, 0);
  lcd.escreverCustomChar(slot);
}

// =====================================================================
// WI-FI + NTP: CONEXÃO E SINCRONIZAÇÃO CONDICIONAL DO RTC
// =====================================================================
void conectarWifiESincronizarRTC() {
  lcd.clear();
  lcdMsgCentralizada(1, "Conectando WiFi");
  lcdMsgCentralizada(2, WIFI_SSID);

  Serial.print("Conectando ao WiFi \"");
  Serial.print(WIFI_SSID);
  Serial.println("\"...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_SENHA);

  unsigned long inicioTentativa = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - inicioTentativa) < WIFI_TIMEOUT_MS) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  wifiConectado = (WiFi.status() == WL_CONNECTED);

  lcdClearTudo();
  if (!wifiConectado) {
    Serial.println("  -> Nao foi possivel conectar ao WiFi. Seguindo sem sincronizar RTC.");
    lcdMsgCentralizada(1, "WiFi");
    lcdMsgCentralizada(2, "Sem conexao");
    delay(TESTE_MOSTRA_RESULT_MS + 500);
    return; // segue a inicialização normal usando só o RTC
  }

  wifiRssiDbm = WiFi.RSSI();
  Serial.print("  -> WiFi conectado. IP: ");
  Serial.print(WiFi.localIP());
  Serial.print(" | RSSI: ");
  Serial.print(wifiRssiDbm);
  Serial.println(" dBm");

  lcdMsgCentralizada(1, "WiFi");
  lcdMsgCentralizada(2, "OK");
  delay(TESTE_MOSTRA_RESULT_MS);

  // --- Busca hora/data via NTP ---
  lcdClearTudo();
  lcdMsgCentralizada(1, "Obtendo hora");
  lcdMsgCentralizada(2, "da internet...");

  configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_OFFSET_SEC, NTP_SERVIDOR_1, NTP_SERVIDOR_2);

  struct tm horaInternet;
  bool ntpOk = getLocalTime(&horaInternet, NTP_TIMEOUT_MS);

  lcdClearTudo();
  if (!ntpOk) {
    Serial.println("  -> Nao foi possivel obter hora via NTP. Mantendo o RTC como esta.");
    lcdMsgCentralizada(1, "Hora internet");
    lcdMsgCentralizada(2, "Falhou");
    delay(TESTE_MOSTRA_RESULT_MS + 500);
    return;
  }

  DateTime horaNet(
    horaInternet.tm_year + 1900,
    horaInternet.tm_mon + 1,
    horaInternet.tm_mday,
    horaInternet.tm_hour,
    horaInternet.tm_min,
    horaInternet.tm_sec
  );

  DateTime horaRtcAtual = rtc.now();

  // Diferença absoluta entre RTC e internet, em segundos.
  long diferencaSegundos = (long)(horaNet.unixtime()) - (long)(horaRtcAtual.unixtime());
  if (diferencaSegundos < 0) diferencaSegundos = -diferencaSegundos;

  // % da diferença em relação a um dia inteiro (24h = 86400s).
  const long SEGUNDOS_POR_DIA = 86400L;
  float diferencaPercent = (100.0f * (float)diferencaSegundos) / (float)SEGUNDOS_POR_DIA;

  Serial.print("  -> Hora RTC atual:     ");
  Serial.println(horaRtcAtual.timestamp());
  Serial.print("  -> Hora da internet:   ");
  Serial.println(horaNet.timestamp());
  Serial.print("  -> Diferenca: ");
  Serial.print(diferencaSegundos);
  Serial.print(" s (");
  Serial.print(diferencaPercent, 3);
  Serial.print("% do dia) | limite configurado: ");
  Serial.print(RTC_SYNC_DIFERENCA_PERCENT);
  Serial.println("%");

  if (diferencaPercent > RTC_SYNC_DIFERENCA_PERCENT) {
    // Diferença maior que o limite -> ajusta o RTC com a hora da internet.
    char bufHora[17];
    snprintf(bufHora, sizeof(bufHora), "%02d:%02d:%02d",
             horaNet.hour(), horaNet.minute(), horaNet.second());

    lcdMsgCentralizada(1, bufHora);
    lcdMsgCentralizada(2, "Configurando RTC");

    rtc.adjust(horaNet);
    Serial.println("  -> RTC estava fora da margem: ajustado com a hora da internet.");

    delay(TESTE_MOSTRA_RESULT_MS + 500);
  } else {
    Serial.println("  -> RTC dentro da margem configurada: mantido como esta.");
  }

  lcdClearTudo();
}

// =====================================================================
// LEITURA DOS SENSORES (biblioteca Adafruit DHT, modelo já detectado)
// =====================================================================
void lerSensores() {
  if (dht == nullptr) {
    leituraDhtValida = false;
    return;
  }

  float umid = dht->readHumidity();
  float temp = dht->readTemperature();

  if (!isnan(umid) && !isnan(temp)) {
    leituraDhtValida = true;
    temperaturaAtual = temp;
    umidadeAtual = umid;
  } else {
    leituraDhtValida = false;
    Serial.println("Falha ao ler o sensor DHT!");
  }
}

// =====================================================================
// ATUALIZAÇÃO DO DISPLAY (operação normal)
// =====================================================================
void atualizarDisplay(const DateTime &agora) {
  char linhaData[12];
  char linhaHora[17];
  char linhaTemp[17];
  char linhaUmid[17];

  // A última coluna (LCD_COLUNAS - 1) da linha 0 fica reservada para o
  // ícone de Wi-Fi no canto superior DIREITO (desenharIconeWifiCantoSuperior()
  // é chamado depois desta função a cada loop e reescreve essa coluna).
  // A data ocupa as colunas 0..14 e fica CENTRALIZADA nesse espaço.
  snprintf(linhaData, sizeof(linhaData), "%02d/%02d/%04d",
           agora.day(), agora.month(), agora.year());

  snprintf(linhaHora, sizeof(linhaHora), "Hora: %02d:%02d:%02d",
           agora.hour(), agora.minute(), agora.second());

  if (leituraDhtValida) {
    snprintf(linhaTemp, sizeof(linhaTemp), "Temp: %.1f C%s",
             temperaturaAtual, emAlarme ? " !" : "");
    snprintf(linhaUmid, sizeof(linhaUmid), "Umid: %.1f %%%s",
             umidadeAtual, emAlarmeUmid ? " !" : "");
  } else {
    snprintf(linhaTemp, sizeof(linhaTemp), "Temp: --.- C");
    snprintf(linhaUmid, sizeof(linhaUmid), "Umid: --.- %%");
  }

  // Linha 0: data centralizada dentro do espaço livre (colunas 0 a
  // LCD_COLUNAS-2), deixando a última coluna livre para o ícone de Wi-Fi.
  uint8_t larguraUtil = LCD_COLUNAS - 1;
  uint8_t tamData = strlen(linhaData);
  uint8_t margemData = (tamData < larguraUtil) ? (larguraUtil - tamData) / 2 : 0;

  lcd.setCursor(0, 0);
  for (uint8_t i = 0; i < margemData; i++) lcd.print(" ");
  lcd.print(linhaData);
  uint8_t colunaAtual = margemData + tamData;
  while (colunaAtual < larguraUtil) {
    lcd.print(" ");
    colunaAtual++;
  }

  // Demais linhas: centralizadas na largura total do display.
  lcdMsgCentralizada(1, linhaHora);
  lcdMsgCentralizada(2, linhaTemp);
  lcdMsgCentralizada(3, linhaUmid);
}

// =====================================================================
// TELA DEDICADA DE ALARME (mostrada alternadamente com a tela normal,
// a cada 3s, enquanto algum alarme - temperatura OU umidade - estiver
// ativo). Mostra APENAS o nome do alarme e o valor, tudo centralizado,
// com um "?" antes e depois do texto. Os "?" piscam junto com o
// LED/buzzer (usam a mesma flag "ligado" do alarme correspondente),
// então essa tela precisa ser redesenhada a cada ciclo do loop (a cada
// 50ms) para o pisca dela acompanhar o pisca do LED/buzzer.
//
// PRIORIDADE: se o alarme de temperatura estiver ativo, ele é quem
// aparece aqui (mesma prioridade usada para o LED/buzzer compartilhados
// em atualizarAlarme()/atualizarAlarmeUmidade()). Só mostra o alarme de
// umidade quando o de temperatura não estiver ativo.
void telaAlarmeCheia() {
  String nome;
  float valor;
  bool piscaLigado;

  if (emAlarme) {
    nome = "Temperatura";
    valor = temperaturaAtual;
    piscaLigado = alarmeLedLigado;
  } else {
    // tipoAlarmeUmidAtivo: 1 = umidade alta, 2 = umidade baixa
    nome = "Umidade";
    valor = umidadeAtual;
    piscaLigado = alarmeLedUmidLigado;
  }

  char linhaValor[17];
  if (nome == "Temperatura") {
    snprintf(linhaValor, sizeof(linhaValor), "%.1f C", valor);
  } else {
    snprintf(linhaValor, sizeof(linhaValor), "%.1f %%", valor);
  }

  String marca = piscaLigado ? "!" : " "; // "!" pisca junto com LED/buzzer

  lcdClearTudo();
  lcdMsgCentralizada(0, marca);
  lcdMsgCentralizada(1, nome);
  lcdMsgCentralizada(2, linhaValor);
  lcdMsgCentralizada(3, marca);
}

// =====================================================================
// BEEP HORÁRIO: 1 beep alto de 1,5s a cada hora cheia (minuto=0 e
// segundo=0), ex.: 2h, 3h, 4h... Não depende de estar ou não em alarme
// - toca independentemente, mas cede prioridade ao alarme de
// temperatura/umidade se algum já estiver soando naquele instante
// exato (evita cortar o tone() do alarme no meio de um beep dele).
// -----------------------------------------------------------------------
// Controle de disparo único: usa ultimaHoraBeepHorario para garantir
// que o beep dispare só UMA VEZ por hora (no instante em que minuto e
// segundo viram 0), mesmo que o loop passe várias vezes por esse
// segundo (a cada 50ms).
void verificarBeepHorario(const DateTime &agora) {
  if (agora.minute() == 0 && agora.second() == 0) {
    if (ultimaHoraBeepHorario != (int8_t)agora.hour()) {
      ultimaHoraBeepHorario = (int8_t)agora.hour();

      // Se algum alarme de temperatura/umidade estiver soando agora,
      // não interrompe o beep dele - o beep horário fica pra próxima
      // leitura do loop (perde-se no máximo ~50ms, imperceptível).
      if (!emAlarme && !emAlarmeUmid) {
        tone(BUZZER_PIN, BEEP_HORARIO_FREQUENCIA);
        delay(BEEP_HORARIO_DURACAO_MS);
        noTone(BUZZER_PIN);
      }
    }
  }
}

// =====================================================================
// LÓGICA DO ALARME DE TEMPERATURA (BIDIRECIONAL: CALOR acima de
// TEMP_LIMITE_MAX, FRIO abaixo de TEMP_LIMITE_MIN)
// =====================================================================
// Regra pedida:
//   - Quanto mais QUENTE (acima de TEMP_LIMITE_MAX), mais RÁPIDO o LED
//     pisca (cor calor) e o buzzer beepa.
//   - Quanto mais FRIO (abaixo de TEMP_LIMITE_MIN), mais RÁPIDO o LED
//     pisca (cor frio) e o buzzer beepa.
//   - O LED e o buzzer piscam/beepam JUNTOS, no mesmo ritmo (o período
//     calculado abaixo vale tanto para o LED quanto para o buzzer).
//
// PRIORIDADE COM O ALARME DE UMIDADE: o LED e o buzzer são compartilhados
// por hardware entre os dois alarmes. Quando os dois estão ativos ao
// mesmo tempo, o alarme de TEMPERATURA tem prioridade (é tratado aqui);
// o alarme de umidade só assume o LED/buzzer quando a temperatura está
// dentro da faixa normal (ver atualizarAlarmeUmidade() logo abaixo).
void atualizarAlarme() {
  if (!leituraDhtValida) {
    emAlarme = false;
    tipoAlarmeAtivo = 0;
    return;
  }

  if (temperaturaAtual >= TEMP_LIMITE_MAX) {
    // ---- ALARME DE CALOR ----
    // Cor vai de LARANJA (mal passou do limite, pisca/beep lento) até
    // VERMELHO PURO (bem acima do limite, pisca/beep no máximo).
    emAlarme = true;
    tipoAlarmeAtivo = 1;

    float excedente = temperaturaAtual - TEMP_LIMITE_MAX;
    float proporcao = calcularProporcaoAlarme(excedente);
    unsigned long periodo = calcularPeriodoAlarme(excedente);
    unsigned long agoraMs = millis();

    if (agoraMs - ultimoToggleAlarme >= (periodo / 2)) {
      ultimoToggleAlarme = agoraMs;
      alarmeLedLigado = !alarmeLedLigado;

      if (alarmeLedLigado) {
        ledSetColorMisturada(COR_CALOR_INICIO_R, COR_CALOR_INICIO_G, COR_CALOR_INICIO_B,
                              COR_CALOR_FIM_R, COR_CALOR_FIM_G, COR_CALOR_FIM_B,
                              proporcao);
        tone(BUZZER_PIN, BUZZER_FREQUENCIA_TEMP_MAX);
      } else {
        ledOff();
        noTone(BUZZER_PIN);
      }
    }

  } else if (temperaturaAtual <= TEMP_LIMITE_MIN) {
    // ---- ALARME DE FRIO ----
    // Cor vai de CIANO (mal passou do limite, pisca/beep lento) até
    // AZUL PURO (bem abaixo do limite, pisca/beep no máximo).
    emAlarme = true;
    tipoAlarmeAtivo = 2;

    float excedente = TEMP_LIMITE_MIN - temperaturaAtual;
    float proporcao = calcularProporcaoAlarme(excedente);
    unsigned long periodo = calcularPeriodoAlarme(excedente);
    unsigned long agoraMs = millis();

    if (agoraMs - ultimoToggleAlarme >= (periodo / 2)) {
      ultimoToggleAlarme = agoraMs;
      alarmeLedLigado = !alarmeLedLigado;

      if (alarmeLedLigado) {
        ledSetColorMisturada(COR_FRIO_INICIO_R, COR_FRIO_INICIO_G, COR_FRIO_INICIO_B,
                              COR_FRIO_FIM_R, COR_FRIO_FIM_G, COR_FRIO_FIM_B,
                              proporcao);
        tone(BUZZER_PIN, BUZZER_FREQUENCIA_TEMP_MIN);
      } else {
        ledOff();
        noTone(BUZZER_PIN);
      }
    }

  } else {
    // ---- DENTRO DA FAIXA NORMAL: sem alarme de temperatura ----
    if (emAlarme) {
      emAlarme = false;
      tipoAlarmeAtivo = 0;
      alarmeLedLigado = false;
      ledOff();
      noTone(BUZZER_PIN);
    }
  }
}

// =====================================================================
// LÓGICA DO ALARME DE UMIDADE (BIDIRECIONAL: UMIDADE ALTA acima de
// UMID_LIMITE_MAX, UMIDADE BAIXA abaixo de UMID_LIMITE_MIN)
// =====================================================================
// Mesmo princípio do alarme de temperatura, usando o LED azul e o
// buzzer: quanto mais a umidade se afasta do limite, mais rápido o LED
// pisca e o buzzer beepa. Só assume o LED/buzzer quando o alarme de
// TEMPERATURA não está ativo no momento (temperatura tem prioridade,
// já que compartilham o mesmo LED e o mesmo buzzer por hardware).
void atualizarAlarmeUmidade() {
  if (!leituraDhtValida) {
    emAlarmeUmid = false;
    tipoAlarmeUmidAtivo = 0;
    return;
  }

  // Se o alarme de temperatura estiver ativo, ele já está controlando o
  // LED/buzzer neste ciclo - a umidade cede o hardware e só atualiza seu
  // próprio estado (para o pisca/beep dela recomeçar "do zero" quando a
  // temperatura voltar ao normal, em vez de retomar no meio do ciclo).
  if (emAlarme) {
    if (emAlarmeUmid) {
      emAlarmeUmid = false;
      tipoAlarmeUmidAtivo = 0;
      alarmeLedUmidLigado = false;
    }
    return;
  }

  if (umidadeAtual >= UMID_LIMITE_MAX) {
    // ---- ALARME DE UMIDADE ALTA (ar encharcado) ----
    // Cor vai de VERDE-ÁGUA suave (mal passou do limite) até VERDE-ÁGUA
    // saturado/forte (bem acima do limite, pisca/beep no máximo).
    emAlarmeUmid = true;
    tipoAlarmeUmidAtivo = 1;

    float excedente = umidadeAtual - UMID_LIMITE_MAX;
    float proporcao = calcularProporcaoAlarme(excedente);
    unsigned long periodo = calcularPeriodoAlarme(excedente);
    unsigned long agoraMs = millis();

    if (agoraMs - ultimoToggleAlarmeUmid >= (periodo / 2)) {
      ultimoToggleAlarmeUmid = agoraMs;
      alarmeLedUmidLigado = !alarmeLedUmidLigado;

      if (alarmeLedUmidLigado) {
        ledSetColorMisturada(COR_UMID_ALTA_INICIO_R, COR_UMID_ALTA_INICIO_G, COR_UMID_ALTA_INICIO_B,
                              COR_UMID_ALTA_FIM_R, COR_UMID_ALTA_FIM_G, COR_UMID_ALTA_FIM_B,
                              proporcao);
        tone(BUZZER_PIN, BUZZER_FREQUENCIA_UMID_MAX);
      } else {
        ledOff();
        noTone(BUZZER_PIN);
      }
    }

  } else if (umidadeAtual <= UMID_LIMITE_MIN) {
    // ---- ALARME DE UMIDADE BAIXA (ar seco) ----
    // Cor vai de AMARELO-AREIA (mal passou do limite) até ÂMBAR forte
    // (bem abaixo do limite, pisca/beep no máximo).
    emAlarmeUmid = true;
    tipoAlarmeUmidAtivo = 2;

    float excedente = UMID_LIMITE_MIN - umidadeAtual;
    float proporcao = calcularProporcaoAlarme(excedente);
    unsigned long periodo = calcularPeriodoAlarme(excedente);
    unsigned long agoraMs = millis();

    if (agoraMs - ultimoToggleAlarmeUmid >= (periodo / 2)) {
      ultimoToggleAlarmeUmid = agoraMs;
      alarmeLedUmidLigado = !alarmeLedUmidLigado;

      if (alarmeLedUmidLigado) {
        ledSetColorMisturada(COR_UMID_BAIXA_INICIO_R, COR_UMID_BAIXA_INICIO_G, COR_UMID_BAIXA_INICIO_B,
                              COR_UMID_BAIXA_FIM_R, COR_UMID_BAIXA_FIM_G, COR_UMID_BAIXA_FIM_B,
                              proporcao);
        tone(BUZZER_PIN, BUZZER_FREQUENCIA_UMID_MIN);
      } else {
        ledOff();
        noTone(BUZZER_PIN);
      }
    }

  } else {
    // ---- DENTRO DA FAIXA NORMAL: sem alarme de umidade ----
    if (emAlarmeUmid) {
      emAlarmeUmid = false;
      tipoAlarmeUmidAtivo = 0;
      alarmeLedUmidLigado = false;
      ledOff();
      noTone(BUZZER_PIN);
    }
  }
}

// Calcula a PROPORÇÃO (0.0 a 1.0) de "quanto já se acelerou" a partir do
// excedente (quanto passou do limite). 0.0 = acabou de passar do limite
// (ainda lento, cor "fraca"/misturada), 1.0 = já no máximo (rápido, cor
// pura). É a mesma proporção usada tanto para o período do pisca/beep
// quanto para a mistura de cor do LED - assim luz e som ficam sincronizados
// com o quanto o alarme está "forte".
float calcularProporcaoAlarme(float excedente) {
  if (excedente < 0) excedente = 0;

  float sensibilidade = ALARME_SENSIBILIDADE_PERCENT;
  if (sensibilidade < 0.0f)   sensibilidade = 0.0f;
  if (sensibilidade > 100.0f) sensibilidade = 100.0f;

  float faixaAceleracao = ALARME_FAIXA_ACELERACAO_BASE_C * (1.0f - (sensibilidade / 100.0f));
  if (faixaAceleracao < ALARME_FAIXA_ACELERACAO_MINIMA_C) {
    faixaAceleracao = ALARME_FAIXA_ACELERACAO_MINIMA_C;
  }

  if (excedente > faixaAceleracao) excedente = faixaAceleracao;
  return excedente / faixaAceleracao;
}

// Calcula o período (em ms) do pisca/beep do alarme a partir de "quanto
// a temperatura já passou do limite" (excedente, sempre >= 0, tanto faz
// se é excesso de calor ou de frio - quem chama já manda a distância
// correta em módulo).
//
// A "faixa de aceleração" (excedente necessário para chegar na
// velocidade máxima) é derivada da ÚNICA variável de sensibilidade
// (0-100%): quanto maior a sensibilidade, menor a faixa, ou seja, mais
// fácil/rápido de chegar no piscar/beepar mais veloz.
unsigned long calcularPeriodoAlarme(float excedente) {
  float proporcao = calcularProporcaoAlarme(excedente);
  long periodo = BLINK_PERIODO_MAX_MS -
                  (long)(proporcao * (BLINK_PERIODO_MAX_MS - BLINK_PERIODO_MIN_MS));

  if (periodo < (long)BLINK_PERIODO_MIN_MS) periodo = BLINK_PERIODO_MIN_MS;
  return (unsigned long)periodo;
}

// =====================================================================
// MISTURA DE CORES DO LED CONFORME A PROPORÇÃO DO ALARME (0.0 a 1.0)
// =====================================================================
// Interpola linearmente, canal a canal, entre uma "cor de partida" (mal
// ultrapassou o limite - alarme lento) e uma "cor final" (bem longe do
// limite - alarme rápido/máximo). Ex.: calor vai de laranja -> vermelho
// puro; frio vai de ciano -> azul puro.
void ledSetColorMisturada(uint8_t rInicio, uint8_t gInicio, uint8_t bInicio,
                           uint8_t rFim, uint8_t gFim, uint8_t bFim,
                           float proporcao) {
  if (proporcao < 0.0f) proporcao = 0.0f;
  if (proporcao > 1.0f) proporcao = 1.0f;

  float rCalc = (float)rInicio + ((float)rFim - (float)rInicio) * proporcao;
  float gCalc = (float)gInicio + ((float)gFim - (float)gInicio) * proporcao;
  float bCalc = (float)bInicio + ((float)bFim - (float)bInicio) * proporcao;
  ledSetColorPWM((uint8_t)rCalc, (uint8_t)gCalc, (uint8_t)bCalc);
}

// (Paletas de cor COR_CALOR_*/COR_FRIO_*/COR_UMID_*_* ficam configuradas
// lá no topo do arquivo, na seção CONFIGURAÇÕES, junto dos outros
// parâmetros do alarme - precisam estar definidas antes de serem usadas
// aqui embaixo.)

// =====================================================================
// CONTROLE DO LED RGB (COM PWM, PARA MISTURAR CORES)
// =====================================================================
// ledSetColorPWM(r, g, b): cada canal vai de 0 (apagado) a 255 (100%
// aceso), permitindo misturar cores (ex.: vermelho + verde = laranja,
// verde + azul = ciano). É a função "de baixo nível" usada por todo o
// resto do código - inclusive ledSetColor(bool,bool,bool), que virou
// só um "atalho" que chama esta aqui com 0 ou 255 em cada canal.
void ledSetColorPWM(uint8_t r, uint8_t g, uint8_t b) {
#if LED_CATODO_COMUM
  analogWrite(LED_R_PIN, r);
  analogWrite(LED_G_PIN, g);
  analogWrite(LED_B_PIN, b);
#else
  analogWrite(LED_R_PIN, 255 - r);
  analogWrite(LED_G_PIN, 255 - g);
  analogWrite(LED_B_PIN, 255 - b);
#endif
}

// Mantida por compatibilidade com o autoteste (cores puras on/off).
void ledSetColor(bool r, bool g, bool b) {
  ledSetColorPWM(r ? 255 : 0, g ? 255 : 0, b ? 255 : 0);
}

void ledOff() {
  ledSetColorPWM(0, 0, 0);
}