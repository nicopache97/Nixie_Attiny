
// ING Nico Pacheco : 31-08-2024, optimizado 07-09-2024 ATtiny85
/* funcionalidad:
  //  Ejemplo envia 1 pulso cada 1/2 segundo, habilita y deshabilita al presionar el pulsador (con antirrebote de 50ms)
*/



    #include <TinyWireM.h>  //lib_deps = TinyWireM 


// definicion de pines ATtiny85
#define Pin_Bateri_ADC    5 //ADC1 pin fisico 1 Reset
#define Pin_Nixie_Enable  3 //B3 pin fisico 2
#define Pin_Nixie_data    4 //B4 pin fisico 3
// pin fisico 4 = GND
#define SDA 0               //   pin fisico 5 = SDA
#define Pin_VerHS         1 //B1 pin fisico 6
#define SCL 2               //   pin fisico 7 = SCL
// pin fisico 8 = VCC

// RTC usa pines de comunicacion I2C:
  // pin a4 / scl arduino nano
  // pin a5 / sda arduino nano
  // pin 19/PB7  attiny2313a
  // pin 17/PB5  attiny2313a
  // Pin fisico 5 / SDA ATtiny85 - B0
  // Pin fisico 7 / SCL ATtiny85 - B2
   
// definicion de constantes
#define time_prendido_hs 1100  //ms
#define time_intermedio  100  //ms
#define time_prendido_min 1000 //ms
#define time_prendido_bateria 1560 //ms
#define V_MAX 4.2 // Tensión máxima de la batería (ejemplo: 4.2V para LiPo)
#define V_MIN 3.0 // Tensión mínima de la batería (ejemplo: 3.0V para LiPo)
#define Vmin_ADC 731 // = (V_MIN*1024/V_MAX)
#define PCF8563_ADDR 0x51 // constante identificador PCF8563

// variables privadas libreria NIXIE
    uint8_t contador_pulsos=0;
    uint32_t time_press=millis();

// Prototipos de Funciones
void nixie_prendido();
void nixie_apagado();
void nixie_numero(uint8_t numero);
void enviar_pulsos(uint8_t pulsos);
int porcentajeBateria();


bool RTC_BEGIN(){
    TinyWireM.begin(); // join i2c bus
	//TinyWireM.setClock(400000); //Optional - set I2C SCL to Low Speed Mode of 400kHz

    TinyWireM.beginTransmission (PCF8563_ADDR);
    return (TinyWireM.endTransmission() == 0 ?  true : false);
}

bool RTC_isRunning(void){
	uint8_t reg_00, reg_02; 

	TinyWireM.beginTransmission(PCF8563_ADDR);
	TinyWireM.write(0x00);
	TinyWireM.endTransmission();

	TinyWireM.requestFrom(PCF8563_ADDR, 1);
	reg_00 = TinyWireM.read(); 

	TinyWireM.beginTransmission(PCF8563_ADDR);
	TinyWireM.write(0x02);
	TinyWireM.endTransmission();

	TinyWireM.requestFrom(PCF8563_ADDR, 1);
	reg_02 = TinyWireM.read(); 

	reg_00 = bitRead(reg_00, 5); // Read STOP Bit to check RTC source clock runs
	reg_02 = bitRead(reg_02, 7); // Read VL Bit to check Clock Integrity 

	return (!(reg_00 | reg_02));
}
uint8_t bcd2bin(uint8_t val){
	return val - 6 * (val >> 4);
}
uint8_t RTC_getHours(){
    uint8_t hour;
    TinyWireM.beginTransmission(PCF8563_ADDR);
    TinyWireM.write(0x04);  // Hour Register
    TinyWireM.endTransmission();
    TinyWireM.requestFrom(PCF8563_ADDR, 1);
    hour = TinyWireM.read();
    return (bcd2bin(hour));
}
uint8_t RTC_getMinutes(){
    uint8_t minutes;
    TinyWireM.beginTransmission(PCF8563_ADDR);
    TinyWireM.write(0x03);  // Minute Register
    TinyWireM.endTransmission();
    TinyWireM.requestFrom(PCF8563_ADDR, 1);
    minutes = TinyWireM.read() & ~(1<<7); //fix clear to bit 7
    return (bcd2bin(minutes));
}

// ****** SETUP *****
void setup() {
  delay(1000);
  RTC_BEGIN();

      //configuracion de pines
  pinMode(Pin_VerHS, INPUT_PULLUP);
  pinMode(Pin_Bateri_ADC, INPUT);
  pinMode(Pin_Nixie_data, OUTPUT);
  pinMode(Pin_Nixie_Enable, OUTPUT);
  
 while(!RTC_isRunning()){   // bloquear si no esta conectado el RTC
    nixie_prendido();    enviar_pulsos(44);   delay(time_prendido_hs);
    nixie_apagado();     delay(time_intermedio);
 }

  nixie_apagado();

}

bool mode=1; //prendido
long time_pulso;
void loop() {
  //  Ejemplo envia 1 pulso cada 1/2 segundo, habilita y deshabilita al presionar el pulsador (con antirrebote de 50ms)
  if(digitalRead(Pin_VerHS)){
    if( (millis()-time_press>50)){
      mode=!mode;
      time_pulso=millis();
    }
    time_press=millis(); // mientras no se presione, se actualiza el tiempo
  }
  if( (mode) && (millis()-time_pulso>500) ){// si esta habilitado contar, y paso 1 segundo
    nixie_prendido();    enviar_pulsos(1); time_pulso=millis();
  }
  if(!mode){
    nixie_apagado();
  }
}

// funciones
void nixie_prendido(){
  digitalWrite(Pin_Nixie_Enable,HIGH);
  contador_pulsos=0;
}

void nixie_apagado(){
  digitalWrite(Pin_Nixie_Enable,LOW);
}

// ingresa numero calcula la cantidad de pulsos faltantes
void nixie_numero(uint8_t numero){
  if(numero>=contador_pulsos){
      enviar_pulsos(numero-contador_pulsos);
  }else{
      enviar_pulsos(numero+100-contador_pulsos);
  }
}

// ingresa el numero de pulsos que requiere
void enviar_pulsos(uint8_t pulsos){
  while(pulsos>0){
    delay(1);
    digitalWrite(Pin_Nixie_data,HIGH);
    digitalWrite(Pin_Nixie_data,LOW);
    pulsos--;
  }
}

int porcentajeBateria() {
  pinMode(Pin_Bateri_ADC, INPUT);
  int valor_ADC = analogRead(Pin_Bateri_ADC);
  if(valor_ADC<Vmin_ADC) valor_ADC=Vmin_ADC; //Vmin_ADC = (3.0/4.2)*1024
  return map(valor_ADC, Vmin_ADC, 1023, 0, 100);
}
