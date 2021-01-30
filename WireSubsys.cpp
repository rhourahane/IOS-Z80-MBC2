#include <Wire.h>
#include "WireSubsys.h"
#include "WireUtils.h"

WireSubsys::WireSubsys() : addr(0), transfer(0)
{
  
}

Opcodes WireSubsys::Read(Opcodes opcode, byte &ioByte)
{
  switch (opcode)
  {
    case I2CPROBE:
      ioByte = ProbeAddress(addr);
      lastOpcode = NO_OP;
     break;

    case I2CREAD:
      lastOpcode = DoRead(ioByte);
    break;
  }

  return lastOpcode;
}

Opcodes WireSubsys::Write(Opcodes opcode, byte ioByte)
{
  switch (opcode)
  {
    case I2CADDR:
      lastOpcode = I2cAddr(ioByte);
    break;

    case I2CWRITE:
      lastOpcode = DoWrite(ioByte);
    break;
  }

  return lastOpcode;
}

void WireSubsys::Reset(Opcodes opcode)
{
  
}

Opcodes WireSubsys::I2cAddr(byte ioByte)
{
  if (lastOpcode != I2CADDR)
  {
    lastOpcode = I2CADDR;
    ioCount = 0;
  }
  
  switch (ioCount)
  {
    case 0:
      addr = ioByte;
      ioCount++;
    break;

     case 1:
      transfer = ioByte;
      lastOpcode = NO_OP;
    break;    
  }

  return lastOpcode;
}

Opcodes WireSubsys::DoRead(byte &ioByte)
{
  if (lastOpcode != I2CREAD)
  {
    Serial.printf(F("Reading %d bytes @ %d\n\r"), transfer, addr);
    lastOpcode = I2CREAD;
    ioCount = 0;
    Wire.requestFrom(addr, transfer);
    while (Wire.available())
    {
      byte rdByte = Wire.read();
      buffer[ioCount++] = rdByte;
      Serial.printf(F("Read %d from %d\n\r"), rdByte, addr);
      if (ioCount == transfer)
      {
        break;
      }
    }

    ioCount = 0;
  }

  ioByte = buffer[ioCount++];
  if (ioCount == transfer)
  {
    lastOpcode = NO_OP;
  }
  
  return lastOpcode;
}

Opcodes WireSubsys::DoWrite(byte ioByte)
{
  if (lastOpcode != I2CWRITE)
  {
    ioCount = 0;
    lastOpcode = I2CWRITE;
  }

  Serial.printf(F("Written %d to %d\n\r"), ioByte, addr);
  buffer[++ioCount] = ioByte;
  if (ioCount == transfer)
  {
    Serial.printf(F("Writing %d bytes @ %d\n\r"), transfer, addr);
    Wire.beginTransmission(addr);
    Wire.write(buffer, ioCount);
    byte result = Wire.endTransmission();
    Serial.printf(F("EndTransmission %d after %d bytes\n\r"), result, ioCount);
    lastOpcode = NO_OP;  
  }

  return lastOpcode;
}
