#include "RtcSubsys.h"
#include "RtcUtilities.h"

RtcSubsys::RtcSubsys()
{
}

Opcode RtcSubsys::Read(Opcode opcode, byte &ioByte)
{
  switch (opcode)
  {
    case DATETIME:
      lastOpcode = ReadTime(ioByte);
      break;
  
    default:
      lastOpcode = NO_OP;
      break;
  }

  return lastOpcode;
}

Opcode RtcSubsys::Write(Opcode opcode, byte ioByte)
{
  return NO_OP;
}

void RtcSubsys::foundRtc(byte found)
{
  rtcFound = found;
}

byte RtcSubsys::foundRtc()
{
  return rtcFound;
}

Opcode RtcSubsys::ReadTime(byte &ioByte)
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
