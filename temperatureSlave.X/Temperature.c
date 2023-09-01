// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT// Oscillator Selection bits (RCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, RC on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)


//*****************************************************************************
// Definición e importación de librerías
//*****************************************************************************
#include <stdint.h>
#include <pic16f887.h>
#include "I2C.h"
#include "LCD.h"
#include <xc.h>
//*****************************************************************************
// Definición de variables
//*****************************************************************************
#define _XTAL_FREQ 8000000
#define DHT11_PIN      RB0
#define DHT11_PIN_DIR  TRISB0

uint8_t selector=0; //Indica qué dato se enviará al maestro
uint8_t z; //Utilizado para almacenar buffer de spi
uint8_t useless; //almacena datos enviados por maestro
uint8_t sendData =0; //dato a enviar a maestro
uint8_t highByte; //La temperatura se recibe en porciones de 4 bits, por lo que se usan variables para unirlos
uint8_t lowByte;
uint8_t joinedTemp;
uint8_t highByteH; //La humedad también se recibe como bits separados,por lo que se usa variables para unirlos
uint8_t lowByteH;
uint8_t joinedHum;
        

//*****************************************************************************
// Definición de funciones para que se puedan colocar después del main de lo 
// contrario hay que colocarlos todas las funciones antes del main
//*****************************************************************************
void setup(void);
short Time_out = 0;
unsigned char T_Byte1, T_Byte2, RH_Byte1, RH_Byte2, CheckSum ;

//Función para iniciar la comunicación coon el DTH11
void Start_Signal(void) {
  DHT11_PIN_DIR = 0;     // definir el pin del sensor como input
  DHT11_PIN = 0;         // limpiar el valor del pin

  __delay_ms(25);        //Esperar a que se iniciela señal
  DHT11_PIN = 1;         // enviar señal al sensor

  __delay_us(30);        //esperar 30 us
  DHT11_PIN_DIR = 1;     // configurar el pin como input
}

//Revisar la respuesta
__bit Check_Response() {
  TMR1H = 0;                 // resetear Timer1
  TMR1L = 0;
  TMR1ON = 1;                // activar Timer1 

  while(!DHT11_PIN && TMR1L < 100);  // Esperar a HIGH del pin del sensor (revisar respuesta baja de 80us)

  if(TMR1L > 99)                     // si response time > 99µS  ==> Response error
    return 0;                        // return 0 (el dispositivo tuvo problema de respuesta)

  else
  {
    TMR1H = 0;               // resetear Timer1
    TMR1L = 0;

    while(DHT11_PIN && TMR1L < 100); // Esperar a LOW del pin del sensor (revisar respuesta alta de 80us)

    if(TMR1L > 99)                   //  si response time > 99µS  ==> Response error
      return 0;                      //return 0 (el dispositivo tuvo problema de respuesta)

    else
      return 1;                      // return 1 (el dispositivo envió la respuesta)
  }
}

//leer datos
__bit Read_Data(unsigned char* dht_data)
{
  *dht_data = 0;

  for(char i = 0; i < 8; i++)
  {
    TMR1H = 0;             // resetear Timer1
    TMR1L = 0;

    while(!DHT11_PIN)      // Esperar a dato de DHT11_PIN 
      if(TMR1L > 100) {    // SI hay time out error(Generalmente toma 50µs)
        return 1;
      }

    TMR1H = 0;             // resetear Timer1
    TMR1L = 0;

    while(DHT11_PIN)       // esperar a que se apague el DHT11_PIN 
      if(TMR1L > 100) {    // SI hay time out error ((Generalmente toma 26-28µs paara 0 y 70µs para 1)
        return 1;          // return 1 (timeout error)
      }

     if(TMR1L > 50)                  // si high time > 50  ==>  Sensor sent 1
       *dht_data |= (1 << (7 - i));  // set bit (7 - i)
  }

  return 0;                          // (Lectura correcta de datos)
}

//*****************************************************************************
// Código de Interrupción 
//*****************************************************************************
void __interrupt() isr(void){
   if(PIR1bits.SSPIF == 1){ 

        SSPCONbits.CKP = 0;
       
        if ((SSPCONbits.SSPOV) || (SSPCONbits.WCOL)){
            z = SSPBUF;                 // Leer dato anterior para limpiar buffer
            SSPCONbits.SSPOV = 0;       // limpiar overflow
            SSPCONbits.WCOL = 0;        // limpiar bit de colisión
            SSPCONbits.CKP = 1;         // permitir reloj
        }

        if(!SSPSTATbits.D_nA && !SSPSTATbits.R_nW) {
            //__delay_us(7);
            z = SSPBUF;                 // Lectura del SSBUF para limpiar el buffer y la bandera BF
            //__delay_us(2);
            PIR1bits.SSPIF = 0;         // Limpia bandera de interrupción recepción/transmisión SSP
            SSPCONbits.CKP = 1;         // Habilita entrada de pulsos de reloj SCL
            while(!SSPSTATbits.BF);     // Esperar a que la recepción se complete
            useless = SSPBUF;             // Guardar en useless el valor del buffer de recepción
            __delay_us(250);
            
        }else if(!SSPSTATbits.D_nA && SSPSTATbits.R_nW){
            z = SSPBUF;
            BF = 0;
            selector ++;     //incrementar selector 
             if (selector %2 == 0){ //revisar si selector es par o impar para alternar los datos que se envían al maestro
                sendData = joinedTemp; //si es par, enviar temperatura
            }else if (selector % 2 != 0){
                sendData = joinedHum;  //si es impar, enviar humedad
            } 
        
            SSPBUF = sendData;
            
            SSPCONbits.CKP = 1;
            __delay_us(250);
            while(SSPSTATbits.BF);
        }
       
        PIR1bits.SSPIF = 0;     
    }
  
}
//*****************************************************************************
// Main
//*****************************************************************************
void main(void) {
    setup();
   
    //*************************************************************************
    // Loop infinito
    //*************************************************************************
    while(1){
       
         Start_Signal(); 
      if(Check_Response())    // Revisar si hay respuesta del sensor (Si sí, leer humdad y temperatura)
    {
      // Leer y guardar datos y revisar errores de time out
      if(Read_Data(&RH_Byte1) || Read_Data(&RH_Byte2) || Read_Data(&T_Byte1) || Read_Data(&T_Byte2) || Read_Data(&CheckSum))
      {
        joinedTemp = 0;
        joinedHum = 0;
      }

      else         // Si no hay error de time out
      {
        if(CheckSum == ((RH_Byte1 + RH_Byte2 + T_Byte1 + T_Byte2) & 0xFF))
        {                                       // Y no hay error de checksum
          
          highByte = T_Byte1 / 10  + '0';; //descomponer valores y unirlos para guardado
          lowByte = T_Byte1 % 10  + '0';
          joinedTemp = (highByte - '0') * 10 + (lowByte - '0');
          
         
          highByteH = RH_Byte1 / 10 + '0'; //descomponer valores y unirlos para guardado
          lowByteH = RH_Byte1 % 10 + '0';
          joinedHum = (highByteH - '0') * 10 + (lowByteH - '0');
         
        }

        // si hay error de checksum
        else
        {
         joinedTemp = 0;
        joinedHum = 0;
        }

      }
    }

    // Si hay problema en la respuesta del sensor
    else
    {
      joinedTemp = 0;
        joinedHum = 0;
    }

    TMR1ON = 0;        // apagar timer1
    __delay_ms(1000);  // 

  }
    
    return;
}
//*****************************************************************************
// Función de Inicialización
//*****************************************************************************
void setup(void){
    ANSEL = 0; 
    ANSELH = 0; 
    TRISD = 0;
    PORTD = 0;
     // CONFIGURACION DEL OSCILADOR
    OSCCONbits.IRCF2 = 1;
    OSCCONbits.IRCF1 = 1;
    OSCCONbits.IRCF0 = 1; // 8MHZ
    OSCCONbits.SCS = 1;  
    T1CON  = 0x10;        // Timer1 con interna con prescaler  de 1:2 
    TMR1H  = 0;           // resetear Timer1
    TMR1L  = 0;
    
    TRISB = 0;
    
    
    PORTB = 0;
    
    
    I2C_Slave_Init(0x80);   //Iniciar esclavo en la dirección 80
}
