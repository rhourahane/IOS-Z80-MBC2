#include "BaseSubsys.h"

BaseSubsys::BaseSubsys()
{
}

Opcode BaseSubsys::lastOpcode = NO_OP;
word BaseSubsys::ioCount = 0;
byte BaseSubsys::ioBuffer[128];
