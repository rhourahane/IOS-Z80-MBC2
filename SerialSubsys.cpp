#include "SerialSubsys.h"
#include "HwDefines.h"

SerialSubsys::SerialSubsys() : lastRxIsEmpty(0)
{
}

Opcode SerialSubsys::Read(Opcode opcode, byte &ioByte)
{
  ioByte = 0xFF;
  if (Serial.available() > 0)
  {
    ioByte = Serial.read();

    // Reset the "Last Rx char was empty" flag
    lastRxIsEmpty = 0;
  }
  else
  {
    // Set the "Last Rx char was empty" flag
    lastRxIsEmpty = 1;
  }

  digitalWrite(INT_, HIGH);
}

Opcode SerialSubsys::Write(Opcode opcode, byte ioByte)
{
  switch (opcode)
  {
    case SERIAL_TX:
      Serial.write(ioByte);
      break;
  }

  return NO_OP;
}

byte SerialSubsys::LastRxIsEmpty()
{
  return lastRxIsEmpty;
}

void SerialSubsys::LastRxIsEmpty(byte value)
{
  lastRxIsEmpty = value;
}
