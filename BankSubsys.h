#if !defined(_BANKSYBSYS_H_)
#define _BANKSYBSYS_H_
#include "BaseSubsys.h"

class BankSubsys : public BaseSubsys
{
public:
  virtual Opcode Read(Opcode opcode, byte &ioByte);
  virtual Opcode Write(Opcode opcode, byte ioByte);
  virtual void Reset(Opcode opcode);
};

#endif
