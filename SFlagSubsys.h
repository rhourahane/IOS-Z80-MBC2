#if !defined(_SFLAGSUBSYS_H_)
#define _SFLAGSUBSYS_H_
#include "BaseSubsys.h"
#include "SerialSubsys.h"

class SFlagSubsys : public BaseSubsys
{
public:
  SFlagSubsys(SerialSubsys &serial);
    
  Opcode Read(Opcode opcode, byte &ioByte);
  Opcode Write(Opcode opcode, byte ioByte);
  void Reset(Opcode opcode);

  void FoundRtc(byte value);
  void AutoexecFlag(byte value);

private:
  SerialSubsys &serialSubsys;
  byte foundRtc;
  byte autoexecFlag;
};

#endif
