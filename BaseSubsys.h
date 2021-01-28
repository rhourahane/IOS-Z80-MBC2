#if !defined(_BASESUBSYS_H_)
#define _BASESUBSYS_H_
#include <Arduino.h>
#include "OpCodes.h"

class BaseSubsys
{
public:
  virtual OpCodes Read(OpCodes opcode, byte &ioByte) = 0;
  virtual OpCodes Write(OpCodes opcode, byte ioByte) = 0; 
};

#endif
