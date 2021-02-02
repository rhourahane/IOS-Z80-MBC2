#if !defined(_WIRESUBSYS_H_)
#define _WIRESUBSYS_H_
#include "BaseSubsys.h"

class WireSubsys : public BaseSubsys
{
public:
  WireSubsys();
  
  Opcode Read(Opcode opcode, byte &ioByte);
  Opcode Write(Opcode opcode, byte ioByte);
  void Reset(Opcode opcode);

private:
  Opcode I2cAddr(byte ioByte);
  Opcode DoRead(byte &ioByte);
  Opcode DoWrite(byte ioByte);

private:
  uint8_t addr;
  uint8_t regAddr;
  uint8_t transfer;
};

#endif
