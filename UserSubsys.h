#if !defined(_USERSUBSYS_H_)
#define _USERSUBSYS_H_
#include "BaseSubsys.h"

class UserSubsys : public BaseSubsys
{
public:
  Opcodes Read(Opcodes opcode, byte &ioByte);
  Opcodes Write(Opcodes opcode, byte ioByte);
  void Reset(Opcodes opcode);
};

#endif
