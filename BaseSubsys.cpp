#include "BaseSubsys.h"

BaseSubsys::BaseSubsys()
{
}

Opcode BaseSubsys::lastOpcode = NO_OP;
int BaseSubsys::ioCount = 0;
byte BaseSubsys::ioBuffer[128];
