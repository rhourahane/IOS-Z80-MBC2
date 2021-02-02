#include "RtcSubsys.h"
#include "RtcUtilities.h"

RtcSubsys::RtcSubsys()
{
}

Opcodes RtcSubsys::Read(Opcodes opcode, byte &ioByte)
{
  switch (opcode)
  {
    case DATETIME:
      lastOpcode = ReadTime(ioByte);
      break;
  }

  return lastOpcode;
}

Opcodes RtcSubsys::Write(Opcodes opcode, byte ioByte)
{

}

void RtcSubsys::Reset(Opcodes opcode)
{

}

void RtcSubsys::foundRtc(byte found)
{
  rtcFound = found;
}

byte RtcSubsys::foundRtc()
{
  return rtcFound;
}

Opcodes RtcSubsys::ReadTime(byte &ioByte)
{
  if (rtcFound)
  {
    if (lastOpcode != DATETIME)
    {
      readRTC((RtcTime&)ioBuffer);
      ioCount = 0;
      lastOpcode = DATETIME;
    }

    // Send date/time (binary values) to Z80 bus
    if (ioCount < sizeof(RtcTime))
    {
      ioByte = ioBuffer[ioCount++];
    }
    else
    {
      return NO_OP;
    }
  }
  else
  {
    return NO_OP;
  }

  return DATETIME;
}
