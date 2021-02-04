#if !defined(_BASESUBSYS_H_)
#define _BASESUBSYS_H_
#include <Arduino.h>
#include "Opcode.h"

class BaseSubsys
{
public:
  BaseSubsys();
  
  //virtual Opcode Read(Opcode opcode, byte &ioByte) = 0;
  //virtual Opcode Write(Opcode opcode, byte ioByte) = 0;

protected:
  static Opcode lastOpcode;
  static int ioCount;
  static byte ioBuffer[128];
};

#endif
