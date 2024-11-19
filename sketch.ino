// Adicionando as Bibliotecas de suporte
#include <Adafruit_SSD1306.h>   // Biblioteca para o display OLED.
#include <Arduino.h>
#include <Wire.h>
#include <ACS712.h>
#include <WiFi.h>
#include <ThingerESP32.h>


struct Tempo {
  int dia;       // Adicionado para armazenar o número de dias
  int hora;
  int minuto;
  int segundo;
  char texto[30]; // Aumente o tamanho do texto para acomodar o novo formato
} tempoAtual;

// Variáveis globais
#define ACS712 34

#define USERNAME "RicardoBrito"           // Usuário do Thinger.Io 
#define DEVICE_ID "ESP32"                   // Id do ESP32
#define DEVICE_CREDENTIAL "9jC1RqTwTS-J&0YU"      // Credencial do ESP32
#define SSID "Wokwi-GUEST"          // Wifi simulado do próprio dispositivo
#define SSID_PASSWORD ""

unsigned long miliTotal = 0;
unsigned long miliPassado = 0;

int tensao = 3.3;  // Tensão fixa em Volts
int modelo = 66;  // Sensibilidade do sensor ACS712
float offset = 1.65;  // Offset de 1.65V para ~0A
float consumo = 0;
float corrente = 0;
float watt = 0;
float fatorTempo = 1.0 / 3600000.0;  // Fator de tempo em horas
Adafruit_SSD1306 display(128, 32, &Wire, -1);  // Objeto Display

const int sensor_ldr = 17;     // pino de leitura digital do sensor
const int rele =  16;      // pino de comando do modulo rele
 
// Inicializando o cliente Thinger.io
ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

void setup() {
  Serial.begin(115200);
  thing.add_wifi(SSID, SSID_PASSWORD);
  // define o pino relativo ao rele de saida
  pinMode(rele, OUTPUT);
  // define o pino relativo ao sensor como entrada digital
  pinMode(sensor_ldr, INPUT);
  
  // Configuração do display OLED
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  display.setCursor(0, 0);
  display.println(F(" Monitor de Corrente "));
  display.display();
 
  // Configuração do WI-FI
  Serial.print("Conectando-se ao Wi-Fi");
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println(" Conectado!");

  // Adicionar recursos ao Thinger.io
  thing["corrente"] >> [](pson &out) {
    out = corrente; // Envia a corrente para o dashboard
  };
  thing["tensao"] >> [](pson &out) {
    out = tensao; // Envia a tensão para o dashboard
  };
  thing["potencia"] >> [](pson &out) {
    out = watt; // Envia a potência para o dashboard
  };
  thing["consumo"] >> [](pson &out) {
    out = consumo; // Envia o consumo para o dashboard
  };
  thing["tempo"] >> [](pson &out) {
    out = (const char*)tempoAtual.texto; // Envia o tempo formatado para o dashboard
  };
}

void calcularTempo() {
  unsigned long tempoMillis = millis();
  tempoAtual.dia = tempoMillis / 86400000; // 86400000 ms em um dia
  tempoAtual.hora = (tempoMillis / 3600000) % 24;
  tempoAtual.minuto = (tempoMillis / 60000) % 60;
  tempoAtual.segundo = (tempoMillis / 1000) % 60;
  sprintf(tempoAtual.texto, "Tempo: %02dd %02d:%02d:%02d", tempoAtual.dia, tempoAtual.hora, tempoAtual.minuto, tempoAtual.segundo);
  }

void loop() {
  // lê o estado do sensor e armazena na variavel leitura
  int leitura = digitalRead(sensor_ldr);
 
  // verifica se há luz ambiente. Se não houver, aciona rele
  if (leitura == LOW) {
    // aciona rele (obs: este rele é acionado em nivel LOW)
    digitalWrite(rele, LOW);
  } else {
    // desliga rele
    digitalWrite(rele, HIGH);
  }
  // Calcula o tempo decorrido
  miliPassado = millis() - miliTotal;
  miliTotal = millis();

  // Calcula corrente e potência
  float leituraACS712 = (analogRead(ACS712) * (3.3 / 4095.0));  // Converte leitura para Volts
  corrente = (leituraACS712 - offset) / (modelo / 1000.0);  // Calcula a corrente em Amperes
  watt = tensao * corrente;  // Calcula a potência em Watts
  consumo += watt * miliPassado * fatorTempo;  // Atualiza o consumo em Watt-horas

  // Atualiza o display com as medições
  display.fillRect(0, 8, 127, 31, BLACK);
  display.setCursor(0, 8);

  display.print(F("I: "));
  display.print(corrente, 1);
  display.print(F("A"));
  display.setCursor(50, 8);
  display.print(F("V: "));
  display.print(tensao);
  display.println(F("V"));

  display.print(F("W: "));
  display.print(watt, 1);
  display.print(F("W"));

  display.setCursor(50, 16);
  display.print(F("C: "));
  display.print(consumo, 2);
  display.println(F("Wh"));


  // Exibe o tempo decorrido
  calcularTempo();
  display.println(tempoAtual.texto);
  display.display();

  thing.handle();

  // Atraso para evitar sobrecarga
  delay(100);
}