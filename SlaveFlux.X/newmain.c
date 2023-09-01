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

// #pragma config statements should precede project file includes.


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


uint8_t z; //Utilizado para almacenar buffer de spi
uint8_t valor =0; //almacena datos enviados por maestro
uint8_t sendValue=0; //valor a enviaar a maestro
unsigned int pulsos = 0;   

//*****************************************************************************
// Definición de funciones para que se puedan colocar después del main de lo 
// contrario hay que colocarlos todas las funciones antes del main
//*****************************************************************************
void setup(void);


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
            valor = SSPBUF;             // Guardar en el PORTD el valor del buffer de recepción
            __delay_us(250);
            
        }else if(!SSPSTATbits.D_nA && SSPSTATbits.R_nW){
            z = SSPBUF;
            BF = 0;
            SSPBUF = sendValue; //Enviar dato a maestro
            SSPCONbits.CKP = 1;
            __delay_us(250);
            while(SSPSTATbits.BF);
        }
       
        PIR1bits.SSPIF = 0;     
    }
   
   //Cuando estén activadas las interrupciones del puerto B, aumentar contador para conocer el número de pulsos
   if (INTCONbits.RBIF){
        
        if (PORTBbits.RB0==0){ 
            
            pulsos ++;
        } 
        
        
        INTCONbits.RBIF=0;
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
       
            //Activar interrupciones en puerto B durante un segundo 
            INTCONbits.RBIE=1;
            __delay_ms(1000);
            INTCONbits.RBIE=0;
            
        //Almacenar el número de pulsos en valor de envío
        sendValue = pulsos;
        //Reiniciar contador de pulsos para siguiente iteración
        pulsos = 0;
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
    TRISB = 0xFF;
    WPUB = 0xFF;
    IOCB = 0xFF;
    TRISA = 0;
    PORTA = 0;
    OPTION_REGbits.nRBPU=0; 
    INTCONbits.RBIE=0; //Comenzar con interrupciones del puerto B apagadas  
    INTCONbits.RBIF=0; 
    
    
     // CONFIGURACION DEL OSCILADOR
    OSCCONbits.IRCF2 = 1;
    OSCCONbits.IRCF1 = 1;
    OSCCONbits.IRCF0 = 1; // 8MHZ
    OSCCONbits.SCS = 1;  

    
  
    
    I2C_Slave_Init(0x50);   //Iniciar esclavo en la dirección 50
}
