#include "UserSubsys.h"
#include "HwDefines.h"

Opcode UserSubsys::Read(Opcode opcode, byte &ioByte)
{
  if (opcode == USER_KEY)
  {
    byte tempByte = digitalRead(USER);    // Save USER led status
    pinMode(USER, INPUT_PULLUP);          // Read USER Key
    ioByte = !digitalRead(USER);

    // Restore USER led status
    pinMode(USER, OUTPUT); 
    digitalWrite(USER, tempByte);
  }

  return NO_OP;
}

Opcode UserSubsys::Write(Opcode opcode, byte ioByte)
{
  if (opcode == USER_LED)
  {
    if (ioByte & B00000001)
    {
      digitalWrite(USER, LOW); 
    }
    else
    {
      digitalWrite(USER, HIGH);
    }
  }

  return NO_OP;
}


void UserSubsys::Reset(Opcode opcode)
{
}
