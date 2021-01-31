#if !defined(_BASESUBSYS_H_)
#define _BASESUBSYS_H_
#include <Arduino.h>
#include "Opcodes.h"

class BaseSubsys
{
public:
  BaseSubsys();
  
  virtual Opcodes Read(Opcodes opcode, byte &ioByte) = 0;
  virtual Opcodes Write(Opcodes opcode, byte ioByte) = 0;
  virtual void Reset(Opcodes opcode) = 0;

protected:
  static Opcodes lastOpcode;
  static int ioCount;
};

#endif
