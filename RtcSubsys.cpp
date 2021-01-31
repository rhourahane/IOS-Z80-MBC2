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
      readRTC(curTime, &tempC);
      ioCount = 0;
      lastOpcode = DATETIME;
    }

    // Send date/time (binary values) to Z80 bus
    if (ioCount < 7)
    {
      switch (ioCount)
      {
        case 0:
          ioByte = curTime.tm_sec;
          break;
        case 1:
          ioByte = curTime.tm_min;
          break;
        case 2:
          ioByte = curTime.tm_hour;
          break;
        case 3:
          ioByte = curTime.tm_mday;
          break;
        case 4:
          ioByte = curTime.tm_mon;
          break;
        case 5:
          ioByte = curTime.tm_year;
          break;
        case 6:
          ioByte = tempC;
          return NO_OP;
          break;
      }
      ioCount++;
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
