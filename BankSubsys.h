#if !defined(_BANKSYBSYS_H_)
#define _BANKSYBSYS_H_
#include "BaseSubsys.h"

class BankSubsys : public BaseSubsys
{
public:
  virtual Opcodes Read(Opcodes opcode, byte &ioByte);
  virtual Opcodes Write(Opcodes opcode, byte ioByte);
  virtual void Reset(Opcodes opcode);
};

#endif
