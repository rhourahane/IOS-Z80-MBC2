#include <Arduino.h>
#include <Wire.h>

byte ProbeAddress(byte address)
{
  Wire.beginTransmission(address);
  if (Wire.endTransmission() != 0)
  {
    // transmission not ack'ed so nothing there.
    return 0;
  }
  return 1;
}

byte WriteRegisters(byte address, byte regAddr, int byteCount, byte *buffer)
{
    Wire.beginTransmission(address);
    Wire.write(regAddr);
    Wire.write(buffer, byteCount);
    
    return Wire.endTransmission();
}

byte ReadRegisters(byte address, byte regAddr, int byteCount, byte *buffer)
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
