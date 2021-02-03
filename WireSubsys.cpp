#include <Wire.h>
#include "WireSubsys.h"
#include "WireUtils.h"

WireSubsys::WireSubsys() : addr(0), transfer(0)
{
  
}

Opcode WireSubsys::Read(Opcode opcode, byte &ioByte)
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

Opcode WireSubsys::Write(Opcode opcode, byte ioByte)
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

Opcode WireSubsys::I2cAddr(byte ioByte)
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

Opcode WireSubsys::DoRead(byte &ioByte)
{
  if (lastOpcode != I2CREAD)
  {
    lastOpcode = I2CREAD;
    ioCount = 0;
    Wire.requestFrom(addr, transfer);
  }

  if (Wire.available())
  {
    ioByte = Wire.read();
    ++ioCount;
    if (ioCount == transfer)
    {
      lastOpcode = NO_OP;
    }
  }
  else
  {
    lastOpcode = NO_OP;
  }
  
  return lastOpcode;
}

Opcode WireSubsys::DoWrite(byte ioByte)
{
  if (lastOpcode != I2CWRITE)
  {
    ioCount = 0;
    lastOpcode = I2CWRITE;
    Wire.beginTransmission(addr);
  }

  Wire.write(ioByte);
  ++ioCount;
  if (ioCount == transfer)
  {
    byte result = Wire.endTransmission();
    if (result != 0)
    {
      Serial.printf(F("EndTransmission failed with %d\n\r"), result);
    }
    lastOpcode = NO_OP;  
  }

  return lastOpcode;
}
