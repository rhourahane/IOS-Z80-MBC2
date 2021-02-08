#include "SFlagSubsys.h"

SFlagSubsys::SFlagSubsys(SerialSubsys &serial) : serialSubsys(serial)
{
  
}

Opcode SFlagSubsys::Read(Opcode opcode, byte &ioByte)
{
  switch (opcode)
  {
    case SYSFLAGS:
      ioByte = autoexecFlag | (foundRtc << 1) | ((Serial.available() > 0) << 2) | ((serialSubsys.LastRxIsEmpty() > 0) << 3);
      break;

    default:
      lastOpcode = NO_OP;
      break;
  }

  return NO_OP;
}

Opcode SFlagSubsys::Write(Opcode opcode, byte ioByte)
{
  return NO_OP;
}

void SFlagSubsys::FoundRtc(byte value)
{
  foundRtc = value;
}

void SFlagSubsys::AutoexecFlag(byte value)
{
  autoexecFlag = value;
}
