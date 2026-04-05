#include <Arduino.h>

void attachPCInt(uint8_t pcInt, void (*pcisr)(void));

uint8_t digitalPinToPCInterrupt(uint8_t pin);