#include "pcint.h"

typedef void(*PCISR)(void); 

PCISR pcISR[] = {0, 0, 0, 0, 0, 0, 0, 0}; 

static volatile uint8_t lastB = PINB;

void attachPCInt(uint8_t pcInt, void (*pcisr)(void))
{
    cli();
    PCICR = (1 << PCIE0);

    PCMSK0 |= (1 << pcInt);
    pcISR[pcInt] = pcisr;

    PCIFR = (1 << PCIF0);

    lastB &= ~(1 << pcInt);
    lastB |= PINB & (1 << pcInt);
    sei();
}

ISR(PCINT0_vect)
{
    volatile uint8_t pinsB = PINB;

    volatile uint8_t deltaB = pinsB ^ lastB;

    volatile uint8_t maskedDeltaB = deltaB & PCMSK0;
    
    if((maskedDeltaB & (1 << PCINT0))) {pcISR[PCINT0]();}   
    if((maskedDeltaB & (1 << PCINT1))) {pcISR[PCINT1]();}
    if((maskedDeltaB & (1 << PCINT2))) {pcISR[PCINT2]();}
    if((maskedDeltaB & (1 << PCINT3))) {pcISR[PCINT3]();}
    if((maskedDeltaB & (1 << PCINT4))) {pcISR[PCINT4]();}
    if((maskedDeltaB & (1 << PCINT5))) {pcISR[PCINT5]();}
    if((maskedDeltaB & (1 << PCINT6))) {pcISR[PCINT6]();}
    if((maskedDeltaB & (1 << PCINT7))) {pcISR[PCINT7]();}

    lastB = pinsB;
}

uint8_t digitalPinToPCInterrupt(uint8_t pin)
{
  uint8_t pcInt = NOT_AN_INTERRUPT;

#if defined(__AVR_ATmega32U4__)
  switch(pin)
  {
    case 17: pcInt = PCINT0; break;
    case 15: pcInt = PCINT1; break;
    case 16: pcInt = PCINT2; break;
    case 14: pcInt = PCINT3; break;
    case  8: pcInt = PCINT4; break;
    case  9: pcInt = PCINT5; break;
    case 10: pcInt = PCINT6; break;
    case 11: pcInt = PCINT7; break;
    default: break;
  }
#endif

  return pcInt;
}