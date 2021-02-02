#if !defined(_USERSUBSYS_H_)
#define _USERSUBSYS_H_
#include "BaseSubsys.h"

class UserSubsys : public BaseSubsys
{
public:
  Opcode Read(Opcode opcode, byte &ioByte);
  Opcode Write(Opcode opcode, byte ioByte);
  void Reset(Opcode opcode);
};

#endif
