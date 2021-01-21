#if !defined(_WIRE_UTILS_H_)
#define _WIRE_UTILS_H_
#include <Arduino.h>

byte ProbeAddress(byte address);
byte WriteRegisters(byte address, byte regAddr, int byteCount, byte *buffer);
byte ReadRegisters(byte address, byte regAddr, int byteCount, byte *buffer);
#endif
