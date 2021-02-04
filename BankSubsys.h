#if !defined(_BANKSYBSYS_H_)
#define _BANKSYBSYS_H_
#include "BaseSubsys.h"

class BankSubsys : public BaseSubsys
{
public:
  Opcode Read(Opcode opcode, byte &ioByte);
  Opcode Write(Opcode opcode, byte ioByte);
};

#endif
