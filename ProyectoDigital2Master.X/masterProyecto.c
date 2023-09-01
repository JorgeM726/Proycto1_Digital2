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



//***************************
// Definición e importación de librerías
//***************************
#include <stdint.h>
#include <pic16f887.h>
#include "I2C.h"
#include <xc.h>
#include "LCD.h"
#include "IOCB.h"
#include "USART.h"
//***************************
// Definición de variables
//***************************
#define _XTAL_FREQ 8000000
//Hora predeterminada
int hour = 16;   
int min = 59;
int sec = 50;
// Hora en la que se activa el riego 18:00:00
int ala_hour = 18;   
int ala_min = 0;
int ala_sec = 0;
//Valor de sensores para comparación
uint8_t qPulse_Value;
uint8_t TemperatureValue;
uint8_t HumidityValue;
//Variables para almacenar strings que se enviarán a LCD
char bufferHour[4]; 
char bufferSec[4];
char bufferMin[4];
char bufferQ[4];
char bufferHum[4];
char bufferTemp[4];
char *bufferAlarm;
float caudal; //almacenamiento de variable para cálculo de sensor de flujo
uint8_t count; //contador para control de servomotor
unsigned int on_time; //tiempo para apagar la señal del servomotor
uint8_t servoPos;

//***************************
// Definición de funciones para que se puedan colocar después del main de lo 
// contrario hay que colocarlos todas las funciones antes del main
//***************************
void setup(void);
void uint8ToString(uint8_t num, char *str);
void reverse(char str[], int length);
int intToStr(int x, char str[], int d);
void floatToStr(float value, char* buffer, int precision);
int d2b (int to_convert);
int  b2d(int to_convert);

void __interrupt() isr (void){
    
     if (INTCONbits.RBIF){
        
        if (PORTBbits.RB0==0){ 
           //Aumentar valor de hora y enviar información a RTC
            hour++;
            if (hour > 23){
                hour = 0;
            }
            I2C_Master_Start();
            I2C_Master_Write(0xD0);
            I2C_Master_Write(0x02);
            I2C_Master_Write(d2b(hour));
            I2C_Master_Stop();
            __delay_ms(100);
        } 
        
        else if (PORTBbits.RB1 ==0){ 
            //Decrementar valor de hora y enviar información a RTC
            hour--;
            I2C_Master_Start();
            I2C_Master_Write(0xD0);
            I2C_Master_Write(0x02);
            I2C_Master_Write(d2b(hour));
            I2C_Master_Stop();
            __delay_ms(100);
        } 
        
        if (PORTBbits.RB2==0){ 
           //Aumentar valor de minutos y enviar información a RTC
            min++;
            if (min > 59){
                min = 0;
            }
            I2C_Master_Start();
            I2C_Master_Write(0xD0);
            I2C_Master_Write(0x01);
            I2C_Master_Write(d2b(min));
            I2C_Master_Stop();
            __delay_ms(100);
        } 
        
        else if (PORTBbits.RB3 ==0){ 
            //Decrementar valor de minutos y enviar información a RTC
            min--;
            
            I2C_Master_Start();
            I2C_Master_Write(0xD0);
            I2C_Master_Write(0x01);
            I2C_Master_Write(d2b(min));
            I2C_Master_Stop();
            __delay_ms(100);
        } 
       
        INTCONbits.RBIF=0;
    }
     if (RCIF){
        PORTD = 0;
        if (RCREG == '1'){
            USART_print(bufferQ); // Enviar valor de caudal al recibir 1
        }
        else if (RCREG == '2'){
            USART_print(bufferTemp); // Enviar valor de temperatura al recibir 2
        }
        else if (RCREG == '3'){
           USART_print(bufferHum); // Enviar valor de humedad al recibir 3
        }
        
        else if (RCREG == '4'){
           USART_print(bufferAlarm); //Enviar estado de la alarma de riego
           bufferAlarm = "0";       //Reiniciar alarma
           
        }
        RCIF = 0; // Limpiar la bandera de interrupción del USART
    }
     
     if (T0IF){ //Cada interrupción del timer sirve para generar una señal de 20 ms luegode 255 interrupciones
         TMR0 = 217 ; //recargar valor a timer 
         count++; //aumentar cuenta del número de iteraciones
         
         if (count <= on_time)  //Si aún no se cumple el final del periodo necesario para el servo, mantener encendido el pin
    {
        RA1=1; 
    }
    
    if (count >= on_time)
    {
        RA1=0;      //Si ya se cumplió, apagar el pin
        
    }
     T0IF = 0;    
     }
    return;
}

//***************************
// Main
//***************************
void main(void) {
    setup();
    USART_print(" ");
    
    //Enviar tiempo predefinido (segundos)
    I2C_Master_Start();
    I2C_Master_Write(0xD0);
    I2C_Master_Write(0x00);
    I2C_Master_Write(d2b(sec));
    I2C_Master_Stop();
    __delay_ms(100);
    
    
     //minutos
    I2C_Master_Start();
    I2C_Master_Write(0xD0);
    I2C_Master_Write(0x01);
    I2C_Master_Write(d2b(min));
    I2C_Master_Stop();
    __delay_ms(100);
    
     //horas 
    I2C_Master_Start();
    I2C_Master_Write(0xD0);
    I2C_Master_Write(0x02);
    I2C_Master_Write(d2b(hour));
    I2C_Master_Stop();
    __delay_ms(100);
            
    while(1){
        
        //comunicación con sensor de flujo
        I2C_Master_Start();
        I2C_Master_Write(0x50);
        I2C_Master_Write(0);
        I2C_Master_Stop();
        __delay_ms(100);
       
        I2C_Master_Start();
        I2C_Master_Write(0x51);
        qPulse_Value = I2C_Master_Read(0);
        I2C_Master_Stop();
        __delay_ms(100);
        
        
        //comunicación con sensor de temperatura
        
        I2C_Master_Start();
        I2C_Master_Write(0x80);
        I2C_Master_Write(0);
        I2C_Master_Stop();
        __delay_ms(100);
       
        
        //Recibir valor de humedad
        I2C_Master_Start();
        I2C_Master_Write(0x81);
        HumidityValue = I2C_Master_Read(0);
        I2C_Master_Stop();
        __delay_ms(100);
        
        
        //reiniciar comunicación con sensor de temperatura
        I2C_Master_Start();
        I2C_Master_Write(0x80);
        I2C_Master_Write(1);
        I2C_Master_Stop();
        __delay_ms(100);
       
        //Recibir valor de humedad
        I2C_Master_Start();
        I2C_Master_Write(0x81);
        TemperatureValue = I2C_Master_Read(0);
        I2C_Master_Stop();
        __delay_ms(100);
        
        
        
        
        
        //leer segundos
        I2C_Master_Start();
        I2C_Master_Write(0xD0);
        I2C_Master_Write(0x00);
        I2C_Master_RepeatedStart();
        I2C_Master_Write(0xD1);
        sec = b2d(I2C_Master_Read(0));
        I2C_Master_Stop();
        
          //leer minutos
        I2C_Master_Start();
        I2C_Master_Write(0xD0);
        I2C_Master_Write(0x01);
        I2C_Master_RepeatedStart();
        I2C_Master_Write(0xD1);
        min = b2d(I2C_Master_Read(0));
        I2C_Master_Stop();
        
           //leer horas
        I2C_Master_Start();
        I2C_Master_Write(0xD0);
        I2C_Master_Write(0x02);
        I2C_Master_RepeatedStart();
        I2C_Master_Write(0xD1);
        hour = b2d(I2C_Master_Read(0));
        I2C_Master_Stop();
        
        //enviar valores de tiempo a buffers para mostrar en pantalla
       uint8ToString(hour,bufferHour);
        uint8ToString(sec,bufferSec);
        uint8ToString(min,bufferMin);
        
        //Escribir datos en LCDtiempo 
                
        Lcd_Clear();
        __delay_us(50);
        Lcd_Set_Cursor(1,1);
        Lcd_Write_String("Time: ");
        __delay_us(50);
        Lcd_Write_String(bufferHour);
        __delay_us(50);
        Lcd_Write_Char(':');
        __delay_us(50);
        Lcd_Write_String(bufferMin);
        __delay_us(50);
        Lcd_Write_Char(':');
        __delay_us(50);
        Lcd_Write_String(bufferSec);
        
        __delay_us(50);
        
        //cálculo de caudal y almacenamiento de valor de sensores en buffers para moestrar en pantalla
        caudal = qPulse_Value/(98/2);
        floatToStr(caudal, bufferQ, 1);
        uint8ToString(TemperatureValue, bufferTemp);
        uint8ToString(HumidityValue,bufferHum);
        
       
        
        //Escribir valor de sensores en pantalla
        __delay_us(50);
        Lcd_Set_Cursor(2,1);
        __delay_us(50);
        Lcd_Write_String("Q:");
        __delay_us(50);
        Lcd_Write_String(bufferQ);
        __delay_us(50);
        Lcd_Write_String(" ");
    __delay_us(50);
        Lcd_Write_String("H:");
        __delay_us(50);
        Lcd_Write_String(bufferHum);
        
        __delay_us(50);
        Lcd_Write_String(" ");
        __delay_us(50);
        Lcd_Write_String("T:");
        __delay_us(50);
        Lcd_Write_String(bufferTemp);
         __delay_us(50);
         
         //Revisar si se activa la alarma 
        if (ala_hour == hour){
            if (ala_min == min){
                if (ala_sec == sec){
                PORTA = 0b00000001;    //Encender pin de control de puente H
                bufferAlarm = "1";
                
                //Si es momento para riego, leer valor de humedad para definir el tiempo de riego
                if (HumidityValue<= 69){
                    __delay_ms(9000);
                } else if (HumidityValue >= 91){
                    __delay_ms(2900);
                }else {
                    __delay_ms(3600);
                }
                PORTA = 0; 
                }
            }          
        }
         
         if (TemperatureValue >= 27 ){
             on_time = 21; //1.5ms de señal encendida
         }else {
             on_time = 10; //1ms de señal encendida
         }
       
         
         
         
    }
    return;
}
//***************************
// Función de Inicialización
//***************************
void setup(void){
     // CONFIGURACION DEL OSCILADOR
    OSCCONbits.IRCF2 = 1;
    OSCCONbits.IRCF1 = 1;
    OSCCONbits.IRCF0 = 1; // 8MHZ
    OSCCONbits.SCS = 1;  // OSCILADOR INTERNO 
    ANSEL = 0;
    ANSELH = 0;
    TRISA =0;
    PORTA = 0;
    ioc_init(0xFF);
    TRISD = 0;
    PORTB = 0;
    PORTD = 0;
    
    USART_init_baud(9600);
    
    INTCONbits.PEIE = 1;        // Int. de perifericos
    INTCONbits.GIE = 1;         // Int. globales
    I2C_Master_Init(100000);        // Inicializar Comuncación I2C
    
    Lcd_Init();
     Lcd_Clear();
        Lcd_Set_Cursor(1,1);
        
        Lcd_Write_String("Bienvenido");
        Lcd_Set_Cursor(2,1);
        Lcd_Write_String("Cargando");
        
        
        
T0CS = 0;  // Clock interno
T0SE = 0;  // Flanco
PSA = 0;   // prescaler a tmr0
PS2 = 0;   // prescaler de 1:4
PS1 = 0;
PS0 = 1;
TMR0 = 217;   //frecuencia de 12820.51Hz para que después de 255 interrupciones, se tenga un periodo de 20ms
T0IE = 1;
T0IF = 0;
}

int  b2d(int to_convert){
   return (to_convert >> 4) * 10 + (to_convert & 0x0F); 
}

int d2b (int to_convert){
    unsigned int bcd=0;
    unsigned int multiplier = 1;
    
    while (to_convert >0){
        unsigned int digit = to_convert % 10;
        bcd += digit * multiplier;
        multiplier *= 16;
        to_convert /= 10;
    }
   return bcd;
}

void uint8ToString(uint8_t num, char *str) {
    uint8_t temp = num;
    int8_t i = 0;

    // Handle the case when the input number is 0 separately
    if (temp == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    // Extract digits from the number and store them in reverse order
    while (temp > 0) {
        str[i++] = '0' + (temp % 10);
        temp /= 10;
    }

    // Add null terminator
    str[i] = '\0';

    // Reverse the string to get the correct order of digits
    int8_t left = 0;
    int8_t right = i - 1;
    while (left < right) {
        char tempChar = str[left];
        str[left] = str[right];
        str[right] = tempChar;
        left++;
        right--;
    }
}

void reverse(char str[], int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

int intToStr(int x, char str[], int d) {
    int i = 0;
    while (x) {
        str[i++] = (x % 10) + '0';
        x = x / 10;
    }
    while (i < d) {
        str[i++] = '0';
    }
    reverse(str, i);
    str[i] = '\0';
    return i;
}

void floatToStr(float value, char* buffer, int precision) {
    // Handle negative numbers
    if (value < 0) {
        value = -value;
        *buffer++ = '-';
    }

    // Extract integer part
    int integerPart = (int)value;

    // Extract floating part and round it
    float floatingPart = value - integerPart;
    for (int i = 0; i < precision; i++) {
        floatingPart *= 10;
    }
    int roundedFloatingPart = (int)(floatingPart + 0.5);

    // Convert integer part to string
    int integerLength = intToStr(integerPart, buffer, 0);
    buffer += integerLength;

    // Add decimal point if needed
    if (precision > 0) {
        *buffer++ = '.';
    }

    // Convert floating part to string
    int floatingLength = intToStr(roundedFloatingPart, buffer, precision);
    buffer += floatingLength;

    // Null-terminate the string
    *buffer = '\0';
}