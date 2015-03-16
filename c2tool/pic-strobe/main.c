#include <pic12f1501.h>
#include <stdint.h>

static __code uint16_t __at (_CONFIG1) configword1 = _FOSC_INTOSC & _WDTE_OFF & _MCLRE_ON & _CP_OFF & _PWRTE_OFF & _BOREN_OFF & _CLKOUTEN_OFF;
static __code uint16_t __at (_CONFIG2) configword2 = _WRT_OFF & _STVREN_ON & _LVP_ON;

#define FOSC	16000000

void init() {
  OSCCON  = 0b01111000; // int osc: 16Mhz

  INTCON  = 0;          // Clear interrupt flag bits
  GIE     = 0;          // Global irq disable
  T0IE    = 0;          // Timer0 irq disable
  TMR0    = 0;          // Clear timer0 register

  // switch off analog
  ANSELA  = 0;
  ADCON0  = 0;
  ADCON1  = 0;
  ADCON2  = 0;
  CM1CON0 = 0;
  CM1CON1 = 0;
}

void main() {
  init();

  // set PORTA5 as input & PORTA2 to output
  TRISA5 = 1;
  TRISA2 = 0;

  /* generates symetric 0.5 MHZ signal */
  while(1) {
    while(RA5) {}
    TRISA2 = 0; // set ouuput 
    LATA2 = 0;
    LATA2 = 0;
    LATA2 = 1;
    TRISA2 = 1; // high z
    while(!RA5) {}
  }
}
