#if !defined(_SERIALSUBSYS_H_)
#define _SERIALSUBSYS_H_
#include "BaseSubsys.h"

class SerialSubsys: public BaseSubsys
{
public:
  SerialSubsys();
  
  Opcodes Read(Opcodes opcode, byte &ioByte);
  Opcodes Write(Opcodes opcode, byte ioByte);
  void Reset(Opcodes opcode);

  byte LastRxIsEmpty();
  void LastRxIsEmpty(byte value);
  
private:
  byte lastRxIsEmpty;
};
#endif
