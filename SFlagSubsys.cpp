#include "SFlagSubsys.h"

SFlagSubsys::SFlagSubsys(SerialSubsys &serial) : serialSubsys(serial)
{
  
}

Opcodes SFlagSubsys::Read(Opcodes opcode, byte &ioByte)
{
  switch (opcode)
  {
    case SYSFLAGS:
      ioByte = autoexecFlag | (foundRtc << 1) | ((Serial.available() > 0) << 2) | ((serialSubsys.LastRxIsEmpty() > 0) << 3);
      break;
  }

  return NO_OP;
}

Opcodes SFlagSubsys::Write(Opcodes opcode, byte ioByte)
{
  return NO_OP;
}

void SFlagSubsys::Reset(Opcodes opcode)
{
}

void SFlagSubsys::FoundRtc(byte value)
{
  foundRtc = value;
}

void SFlagSubsys::AutoexecFlag(byte value)
{
  autoexecFlag = value;
}
