#include "GpioSubsys.h"
#include "WireUtils.h"
#include "HwDefines.h"

Opcode GpioSubsys::Read(Opcode opcode, byte &ioByte)
{
  switch (opcode)
  {
    case GPIOA_READ:
      ReadRegisters(GPIOEXP_ADDR, GPIOA_REG, 1, &ioByte);
    break;
    
    case GPIOB_READ:
      ReadRegisters(GPIOEXP_ADDR, GPIOB_REG, 1, &ioByte);
    break;
  
    default:
      lastOpcode = NO_OP;
      break;
}
  return NO_OP;
}

Opcode GpioSubsys::Write(Opcode opcode, byte ioByte)
{
  switch (opcode)
  {
    case GPIOA_WRITE:
      WriteRegisters(GPIOEXP_ADDR, GPIOA_REG, 1, &ioByte);
    break;
    
    case GPIOB_WRITE:
      WriteRegisters(GPIOEXP_ADDR, GPIOB_REG, 1, &ioByte);
    break;

    case IODIRA_WRITE:
      WriteRegisters(GPIOEXP_ADDR, IODIRA_REG, 1, &ioByte);
    break;

    case IODIRB_WRITE:
      WriteRegisters(GPIOEXP_ADDR, IODIRB_REG, 1, &ioByte);
    break;

    case GPPUA_WRITE:
      WriteRegisters(GPIOEXP_ADDR, GPPUA_REG, 1, &ioByte);
    break;

    case GPPUB_WRITE:
      WriteRegisters(GPIOEXP_ADDR, GPPUB_REG, 1, &ioByte);
    break;

    default:
      lastOpcode = NO_OP;
      break;
  }
  return NO_OP;
}
