#if !defined(_WIRESUBSYS_H_)
#define _WIRESUBSYS_H_
#include "BaseSubsys.h"

class WireSubsys : public BaseSubsys
{
public:
  WireSubsys();
  
  Opcodes Read(Opcodes opcode, byte &ioByte);
  Opcodes Write(Opcodes opcode, byte ioByte);
  void Reset(Opcodes opcode);

private:
  Opcodes I2cAddr(byte ioByte);
  Opcodes DoRead(byte &ioByte);
  Opcodes DoWrite(byte ioByte);

private:
  uint8_t addr;
  uint8_t regAddr;
  uint8_t transfer;
};

#endif
