#if !defined(_RTCSUBSYS_H_)
#define _RTCSUBSYS_H_
#include "BaseSubsys.h"
#include "RtcUtilities.h"

class RtcSubsys : BaseSubsys
{
public:
  RtcSubsys();
  
  Opcodes Read(Opcodes opcode, byte &ioByte);
  Opcodes Write(Opcodes opcode, byte ioByte);
  void Reset(Opcodes opcode);

  void foundRtc(byte found);
  byte foundRtc();

private:
  Opcodes ReadTime(byte &ioByte);

private:
  byte rtcFound;
};
#endif
