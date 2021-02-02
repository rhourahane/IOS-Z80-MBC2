#if !defined(_RTCSUBSYS_H_)
#define _RTCSUBSYS_H_
#include "BaseSubsys.h"
#include "RtcUtilities.h"

class RtcSubsys : BaseSubsys
{
public:
  RtcSubsys();
  
  Opcode Read(Opcode opcode, byte &ioByte);
  Opcode Write(Opcode opcode, byte ioByte);
  void Reset(Opcode opcode);

  void foundRtc(byte found);
  byte foundRtc();

private:
  Opcode ReadTime(byte &ioByte);

private:
  byte rtcFound;
};
#endif
