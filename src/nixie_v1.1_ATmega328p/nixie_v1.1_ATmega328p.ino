// ING Nico Pacheco : 31-08-2024, optimizado para Arduino Nano 14-09-2024
/* funcionalidad:
 * al presionar el boton de "ver hora" se activa la secuencia para ver la hora
 * - muestra en el nixie los 2 digitos de la hora actual durante 0.9segundos
 * - espera 0.1 con los nixie apagados
 * - muestra en el nixie los 2 digitos de los minutos actuales durante 0.9segundos
 * - espera 0.1 con los nixie apagados
 * al mantener presionado el boton de "ver hora" durante 1segundo. se activa el cronometro
 * - el cronometro inicia en 1segundo, finaliza cuando se presiona nuevamente el boton "ver hora"
 * - mientras esta activado el cronometro cuenta los segundos de 0 a 99
*/

#include <Wire.h>
#include <I2C_RTC.h>

// definicion de pines
#define Pin_Bateri_ADC  15  //A1
#define Pin_VerHS        6  //D6
#define Pin_Nixie_data   7  //D7
#define Pin_Nixie_Triak  8  //D8
#define Pin_Nixie_Enable 9  //D9
// RTC usa pines de comunicacion I2C:
  // pin a4 / scl arduino nano
  // pin a5 / sda arduino nano
  // pin 19/PB7  attiny2313a
  // pin 17/PB5  attiny2313a
   
// definicion de constantes
#define time_prendido_hs 1100  //ms
#define time_intermedio  100  //ms
#define time_prendido_min 1000 //ms
#define time_prendido_bateria 1560 //ms
#define V_MAX 4.2 // Tensión máxima de la batería (ejemplo: 4.2V para LiPo)
#define V_MIN 3.0 // Tensión mínima de la batería (ejemplo: 3.0V para LiPo)
#define Vmin_ADC 731 // = (V_MIN*1024/V_MAX)


// variables privadas libreria NIXIE
    uint8_t contador_pulsos=0;

static PCF8563 RTC;


void setup() 
{
  delay(100);
  RTC.begin();

      //configuracion de pines
  pinMode(Pin_VerHS, INPUT_PULLUP);
  pinMode(Pin_Bateri_ADC, INPUT);
  pinMode(Pin_Nixie_data, OUTPUT);
  pinMode(Pin_Nixie_Triak, OUTPUT);
  pinMode(Pin_Nixie_Enable, OUTPUT);

 while(!RTC.isRunning()){   // bloquear si no esta conectado el RTC
    nixie_prendido();    enviar_pulsos(44);   delay(time_prendido_hs);
    nixie_apagado();     delay(time_intermedio);
 }

  nixie_apagado();
}

uint32_t time_press=millis();

void loop() {
  if(digitalRead(Pin_VerHS)){
    if( (millis()-time_press>10) && ((millis()-time_press<1000)) ){ // si se presiona "verHS" durante menos de 1seg
      nixie_prendido();    enviar_pulsos(RTC.getHours());   delay(time_prendido_hs);
      nixie_apagado();        delay(time_intermedio);
      nixie_prendido();    enviar_pulsos(RTC.getMinutes()); delay(time_prendido_min);
      nixie_apagado();        delay(time_intermedio);
    }else if(millis()-time_press>1000){ // si se presiona durante mas de 1segundo "verHS"
      nixie_prendido();    enviar_pulsos(porcentajeBateria());   delay(time_prendido_bateria);
      nixie_apagado();        delay(time_intermedio);
    }
  time_press=millis(); // mientras no se presione, se actualiza el tiempo
  }
}

// funciones
void nixie_prendido(){
  digitalWrite(Pin_Nixie_Enable,LOW);
  contador_pulsos=0;
}

void nixie_apagado(){
  digitalWrite(Pin_Nixie_Enable,HIGH);
}

// ingresa el numero de pulsos que requiere
void enviar_pulsos(uint8_t pulsos){
  while(pulsos>0){
    digitalWrite(Pin_Nixie_data,HIGH);
    digitalWrite(Pin_Nixie_data,LOW);
    pulsos--;
  }
}

uint8_t porcentajeBateria() {
  pinMode(Pin_Bateri_ADC, INPUT);
  uint16_t valor_ADC = analogRead(Pin_Bateri_ADC);
  if(valor_ADC<Vmin_ADC) valor_ADC=Vmin_ADC; //Vmin_ADC = (3.0/4.2)*1024
  return map(valor_ADC, Vmin_ADC, 1023, 0, 100);
}
