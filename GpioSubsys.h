#if !defined(_GPIOSUBSYS_H_)
#define _GPIOSUBSYS_H_
#include "BaseSubsys.h"

class GpioSubsys : public BaseSubsys
{
public:
  Opcode Read(Opcode opcode, byte &ioByte);
  Opcode Write(Opcode opcode, byte ioByte);
};

#endif
