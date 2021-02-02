#if !defined(_SFLAGSUBSYS_H_)
#define _SFLAGSUBSYS_H_
#include "BaseSubsys.h"
#include "SerialSubsys.h"

class SFlagSubsys : public BaseSubsys
{
public:
  SFlagSubsys(SerialSubsys &serial);
    
  Opcodes Read(Opcodes opcode, byte &ioByte);
  Opcodes Write(Opcodes opcode, byte ioByte);
  void Reset(Opcodes opcode);

  void FoundRtc(byte value);
  void AutoexecFlag(byte value);

private:
  SerialSubsys &serialSubsys;
  byte foundRtc;
  byte autoexecFlag;
};

#endif
