#if !defined(_WIRE_UTILS_H_)
#define _WIRE_UTILS_H_
#include <Arduino.h>

byte ProbeAddress(uint8_t address);
byte WriteRegisters(uint8_t address, uint8_t regAddr, uint8_t byteCount, byte *buffer);
byte ReadRegisters(uint8_t address, uint8_t regAddr, uint8_t byteCount, byte *buffer);
#endif
