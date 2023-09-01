
#include <xc.h>
#include <stdint.h>
#include "USART.h"
#define _XTAL_FREQ 8000000
void USART_init_baud(const unsigned long int baudrate)
{
   
    TXSTAbits.SYNC = 0;
    TXSTAbits.BRGH = 1;
    BAUDCTLbits.BRG16 = 1;
    RCSTAbits.SPEN = 1;
    RCSTAbits.RX9 = 0;
    RCSTAbits.CREN = 1;
    TXSTAbits.TXEN = 1;
    PIR1bits.RCIF = 0;
    PIE1bits.RCIE = 1;
    unsigned int value = 0;
    
    value = (_XTAL_FREQ /(4*baudrate))-1;
    
    
    if(value < 256)
    {
       SPBRG = value;
    }
    
    
}

    


void USART_send(const char data)
{
    while(!TRMT);
    TXREG = data;
}

unsigned char USART_TSR_control(void)
{
    return TRMT;
}

void USART_print(const char *string)
{
    int i = 0;
    
    for(i; string[i] != '\0'; i++)
    {
        USART_send(string[i]);
    }
}

unsigned char USART_read_available(void)
{
    return RCIF;
}

char USART_read(void)
{
    while(!RCIF);
    return RCREG;
}
