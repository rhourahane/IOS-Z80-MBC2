#if !defined(_SERIALSUBSYS_H_)
#define _SERIALSUBSYS_H_
#include "BaseSubsys.h"

class SerialSubsys: public BaseSubsys
{
public:
  SerialSubsys();
  
  Opcode Read(Opcode opcode, byte &ioByte);
  Opcode Write(Opcode opcode, byte ioByte);
  void Reset(Opcode opcode);

  byte LastRxIsEmpty();
  void LastRxIsEmpty(byte value);
  
private:
  byte lastRxIsEmpty;
};
#endif
