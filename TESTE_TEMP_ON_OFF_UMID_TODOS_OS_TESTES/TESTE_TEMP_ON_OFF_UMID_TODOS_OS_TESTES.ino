
// --- CONFIGURAÇÕES DE HARDWARE E VARIÁVEIS ---

#include <Wire.h>
#include "Adafruit_SHT31.h"

// multiplexador
Adafruit_SHT31 sht31 = Adafruit_SHT31();
#define TCAADDR 0x70  // Endereço do TCA9548A

// bombas
const int pinos_bombas[] = {25,32,33};// definição dos pinos das bombas em um array
const int num_bombas = sizeof(pinos_bombas) / sizeof(pinos_bombas[0]); // definição do numero de bombas
const int frequencia = 1000; // 1kHz para torque máximo na calibração

// reles
const int pinos_reles[] = {2,4}; // definição dos pinos dos reles em um array
const int num_reles = sizeof(pinos_reles) / sizeof(pinos_reles[0]); // definição do numero de reles

// Ligar e desligar testes
const bool teste_umid_ON_OFF = false; // para ativar/desativar o teste de umidade com controle ON-OFF, mude para 'true'/'false'
const bool teste_umid_PI = false; // para ativar/desativar o teste de umidade com controle PI, mude para 'true'/'false'
const bool teste_umid_PID = false; // para ativar/desativar o teste de umidade com controle PID, mude para 'true'/'false'
const bool teste_temp = true; // para ativar/desativar o teste de temperatura, mude para 'true'/'false'
bool primeira_exibicao = true; // imprimir cabeçalho para analise do txt

// variaveis para o teste de temperatura
const float setpoint_temp = 30.0; // Temperatura desejada
const float histerese_temp = 0.5; // Histerese desejada para a temperatura
bool systemOverheat; // Valor boleano para verificar se o sistema está sobre superaquecimento
float tmax = 80.0; // Temperatura máxima até o superaquecimento

// variavel para o setpoint da umidade relativa
const float setpoint_umid = 30.0; // Umidade desejada (%)

// variaveis para o teste de umidade (ON-OFF) 
const float histerese_umid = 3.0; 

// variaveis para o teste de umidade (PI) 
double Kp_PI = 10.8, Ki_PI = 0.325;

// variaveis para o teste de umidade (PID)
double Kp_PID = 14.4, Ki_PID = 1.44, Kd_PID = 36.0; //parametros do PID
float pwm_saida, P, I, D;
double erro, ultimo_erro, acumulo_erro, variacao_erro;
double tempo_decorrido; 
float dt;

// variaveis auxiliares para imprimir os valores dos sensores na tela
float T_0, H_0; // valores para o sensor 0
float T_1, H_1; // valores para o sensor 1
float T_2, H_2; // valores para o sensor 2

// limites do pwm das bombas
const float pwmmin = 0; 
const float pwmmax = 255;
float pwm_anterior = 0;

//constantes e contadores de tempo
unsigned long tempo_atual = 0, tempo_anterior = 0;

unsigned long tempoAnteriorTemp = 0;
unsigned long tempoAnteriorUmid = 0;
unsigned long tempoAnteriorLog  = 0;

const unsigned long INTERVALO_TEMP = 200;   // Resposta rápida da temperatura (200 ms)
const unsigned long INTERVALO_UMID = 1000;  // Tempo de transporte da umidade (1,0 s)
const unsigned long INTERVALO_LOG  = 1000;  // Intervalo de escrita no Serial/CSV (1,0 s)

// --- CONFIGURAÇÕES DE FUNÇÕES ---

// inicia as saidas do multiplexador
void tcaselect(uint8_t i) { 
  if (i > 7) return;
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
}

// faz a leitura dos sensores e guarda os valores em variaveis auxiliares
void lerSensores() { 
  for (uint8_t j = 0; j < 3; j++) {
    tcaselect(j);
    float t = sht31.readTemperature();
    float h = sht31.readHumidity();
    if (j == 0) { T_0 = t; H_0 = h; }
    else if (j == 1) { T_1 = t; H_1 = h; }
    else if (j == 2) { T_2 = t; H_2 = h; }
  }
}

// limita o valor de pwm das bombas caso ele estejá próximo a 5% do valor desejado do setpoint
int limitador(int umidade, int setpoint_umid, int pwm_saida, int pwm_anterior)
{
    int erro = setpoint_umid - umidade;
    if (abs(erro) < setpoint_umid * 0.05)
    {
        if ((pwm_saida - pwm_anterior) > 5)
        {
            pwm_saida = pwm_anterior + 5;
        }
        else if ((pwm_saida - pwm_anterior) < -5)
        {
            pwm_saida = pwm_anterior - 5;
        }
    }
    return pwm_saida;
}

// limita o valor de pwm entre um valor mínimo e máximo 
void limite(void){ 
  if (pwm_saida > pwmmax){
    pwm_saida = pwmmax;
  }
  if (pwm_saida < pwmmin){
    pwm_saida = pwmmin;
  }
}

// desliga os reles
void desligar(const int pinos_reles[], int num_reles){ 
  for(int k = 0; k < num_reles; k++){
    digitalWrite(pinos_reles[k], LOW);
  }
}

// liga so reles
void ligar(const int pinos_reles[], int num_reles){ 
  for(int l = 0; l < num_reles; l++){
    digitalWrite(pinos_reles[l], HIGH);
  }
}

// aciona as bombas com valores desejados de pwm
void acionar_bombas(const int pinos_bombas[], int num_bombas, int pwm_saida){ 
  for(int m = 0; m < num_bombas; m++){
    analogWrite(pinos_bombas[m],pwm_saida);
  }
}

// imprimir valores
void imprimir_valores(bool primeira_exibicao, float segundosDecorridos, float T_0, float H_0, float T_1, float H_1, float T_2, float H_2){
  if(primeira_exibicao){
      primeira_exibicao = false;
      Serial.println("Tempo,T0,H0,T1,H1,T2,H2,pwm"); //exibição inicial para cabeçalho do codigo em python
      Serial.print(segundosDecorridos); Serial.print(","); //exibição do tempo 
      Serial.print(T_0); Serial.print(","); Serial.print(H_0); Serial.print(","); //exibição da temperatura e umidade do sensor 0
      Serial.print(T_1); Serial.print(","); Serial.print(H_1); Serial.print(","); //exibição da temperatura e umidade do sensor 1
      Serial.print(T_2); Serial.print(","); Serial.print(H_2); Serial.print(","); //exibição da temperatura e umidade do sensor 2
      Serial.println(pwm_saida); //exibição do valor de saida do pwm
    } 
    else {
      Serial.print(segundosDecorridos); Serial.print(","); //exibição do tempo 
      Serial.print(T_0); Serial.print(","); Serial.print(H_0); Serial.print(","); //exibição da temperatura e umidade do sensor 0
      Serial.print(T_1); Serial.print(","); Serial.print(H_1); Serial.print(","); //exibição da temperatura e umidade do sensor 1
      Serial.print(T_2); Serial.print(","); Serial.print(H_2); Serial.print(","); //exibição da temperatura e umidade do sensor 2
      Serial.println(pwm_saida); //exibição do valor de saida do pwm
    }
}

void setup() {
  Serial.begin(115200);
  for(uint8_t i = 0; i < num_bombas; i++){ // definição do desligamento das bombas ao iniciar
    pinMode(pinos_bombas[i], OUTPUT);
    analogWriteFrequency(pinos_bombas[i], frequencia);
    analogWrite(pinos_bombas[i], 0);
  }
  for(uint8_t j = 0; j < num_reles; j++){ // definição do desligamento dos reles ao iniciar
    pinMode(pinos_reles[j], OUTPUT);
    digitalWrite(pinos_reles[j], LOW);
  }
  Wire.begin(21, 22); 
  bool systemOverheat = false;
  float tmax = 80.0;
  bool primeira_exibicao = true;
  for(uint8_t i = 0; i < 3; i++) {
    tcaselect(i);
    if (!sht31.begin(0x44)) {
      Serial.print("Sensor no canal ");
      Serial.print(i);
      Serial.println(" não encontrado!");
    } else {
      Serial.print("Sensor no canal ");
      Serial.print(i);
      Serial.println(" inicializado.");
    }
  }
  if (!sht31.begin(0x44)) {// endereço padrão do SHT31
    Serial.println("Sensor SHT31 não encontrado!");
    while (1) delay(1);
  }
}

void loop() {
  tempo_atual = millis();

  // leitura dos sensores   
  if (tempo_atual - tempoAnteriorTemp >= INTERVALO_TEMP) {
    tempoAnteriorTemp = tempo_atual;
    lerSensores(); // atualiza a leitura dos sensores
  }

  // exibição de valores
  if (tempo_atual - tempoAnteriorLog >= INTERVALO_LOG) {
    tempoAnteriorLog = tempo_atual;
    float segundosDecorridos = tempo_atual / 1000.0;
    imprimir_valores(primeira_exibicao, segundosDecorridos, T_0, H_0, T_1, H_1, T_2, H_2);
  }

  // controle de temperatura
  if (teste_temp) {
    if (T0 > tmax) { // verificação de superaquecimento (sensor 0)
        systemOverheat = true;
    } 
    else {
      systemOverheat = false;
    }
    if (systemOverheat) { // desligar para segurança
      desligar(pinos_reles,num_reles);
    } 
    else { // Controle ON/OFF da resistência por histerese (sensor 2)
      if (T2 < (setpoint_temp - histerese_temp)) {
        ligar(pinos_reles,num_reles);
      } 
      else if (T2 > (setpoint_temp + histerese_temp)) {
        desligar(pinos_reles,num_reles);
      }
    }
  }
  
  // controle de umidade ON-OFF
  if(teste_umid_ON_OFF){
    if (H_2 < (setpoint_umid - histerese_umid)){
      acionar_bombas(pinos_bombas,num_bombas,255);
    }
    else if(H_2 > (setpoint_umid + histerese_umid)){
      acionar_bombas(pinos_bombas,num_bombas,0);
    }
  }

  // controle de umidade PI
  if(teste_umid_PI){
    if (tempo_atual - tempoAnteriorUmid >= INTERVALO_UMID) {
      dt = (tempo_atual - tempoAnteriorUmid) / 1000.0;
      tempoAnteriorUmid = tempo_atual;
      pwm_anterior = pwm_saida;
          
      erro = setpoint_umid - H_2; //erro atual

      acumulo_erro += (erro * dt); //erro acumulado ao longo do tempo

      P = Kp_PID * erro; // valor proporcional

      I = Ki_PID * acumulo_erro; // valor integral

      pwm_saida = P + I ; // pwm de saida

      ultimo_erro = erro; // iguala o ultimo erro ao erro atua
      pwm_saida = limitador(H_2, setpoint_umid, pwm_saida, pwm_anterior); // limitação do valor do pwm para valores proximos ao setpoint

      limite(); // limita o valor do pwm
      acionar_bombas(pinos_bombas,num_bombas,pwm_saida); // aciona as bombas e seus valores de saida 
    }
  }

  // controle de umidade PID
  if(teste_umid_PID){
    if (tempo_atual - tempoAnteriorUmid >= INTERVALO_UMID) {
      dt = (tempo_atual - tempoAnteriorUmid) / 1000.0;
      tempoAnteriorUmid = tempo_atual;
      pwm_anterior = pwm_saida;
          
      erro = setpoint_umid - H_2; //erro atual

      acumulo_erro += (erro * dt); //erro acumulado ao longo do tempo

      variacao_erro = ((erro - ultimo_erro) / dt); //variação do erro em relação ao ultimo erro

      P = Kp_PID * erro; // valor proporcional

      I = Ki_PID * acumulo_erro; // valor integral

      D = Kd_PID * variacao_erro; // valor diferencial

      pwm_saida = P + I + D; // pwm de saida

      ultimo_erro = erro; // iguala o ultimo erro ao erro atua
      pwm_saida = limitador(H_2, setpoint_umid, pwm_saida, pwm_anterior); // limitação do valor do pwm para valores proximos ao setpoint

      limite(); // limita o valor do pwm
      acionar_bombas(pinos_bombas,num_bombas,pwm_saida); // aciona as bombas e seus valores de saida 
    }
  }
}

