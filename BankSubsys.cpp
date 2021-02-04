#include "BankSubsys.h"
#include "HwDefines.h"

Opcode BankSubsys::Read(Opcode opcode, byte &ioByte)
{
}

Opcode BankSubsys::Write(Opcode opcode, byte ioByte)
{
  switch (ioByte)
  {
    case 0:
      // Set physical bank 0 (logical bank 1)
      digitalWrite(BANK0, HIGH);
      digitalWrite(BANK1, LOW);
    break;
  
    case 1:
      // Set physical bank 2 (logical bank 3)
      digitalWrite(BANK0, HIGH);
      digitalWrite(BANK1, HIGH);
    break;  
  
    case 2:
      // Set physical bank 3 (logical bank 2)
      digitalWrite(BANK0, LOW);
      digitalWrite(BANK1, HIGH);
    break;  
  }

  return NO_OP;
}
