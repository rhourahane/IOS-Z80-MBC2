#if !defined(_GPIOSUBSYS_H_)
#define _GPIOSUBSYS_H_
#include "BaseSubsys.h"

class GpioSubsys : public BaseSubsys
{
public:
  OpCodes Read(OpCodes opcode, byte &ioByte);
  OpCodes Write(OpCodes opcode, byte ioByte);

private:
  
};

#endif
