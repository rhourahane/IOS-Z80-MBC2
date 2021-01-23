#include <Arduino.h>
#include <Wire.h>

byte ProbeAddress(uint8_t address)
{
  Wire.beginTransmission(address);
  if (Wire.endTransmission() != 0)
  {
    // transmission not ack'ed so nothing there.
    return 0;
  }
  return 1;
}

byte WriteRegisters(uint8_t address, uint8_t regAddr, uint8_t byteCount, byte *buffer)
{
    Wire.beginTransmission(address);
    Wire.write(regAddr);
    Wire.write(buffer, byteCount);
    
    return Wire.endTransmission();
}

byte ReadRegisters(uint8_t address, uint8_t regAddr, uint8_t byteCount, byte *buffer)
{
  // Setup the register address
  Wire.beginTransmission(address);
  Wire.write(regAddr);
  Wire.endTransmission();
  
  // Read data from device
  int readCount = 0;
  Wire.requestFrom(address, byteCount);
  while (Wire.available())
  {
    *buffer = Wire.read();
    ++buffer;
    ++readCount;
  }

  return readCount;
}
