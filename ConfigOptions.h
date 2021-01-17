#if !defined(_CONGIG_OPTIONS_H_)
#define _CONGIG_OPTIONS_H_
#include <Arduino.h>

struct ConfigOptions
{
  byte Valid;               // Set to 1 if values have been saved starts as 0xFF
  
  // Use bit fields to reduce space used by flags.
  byte BootMode : 3;        // Set the program to boot (from flash or SD)
  byte AutoexecFlag : 1;    // Set to 1 if AUTOEXEC must be executed at CP/M cold boot, 0 otherwise
  byte ClockMode : 1;       // Z80 clock HI/LO speed selector (0 = 8/10MHz, 1 = 4/5MHz)
  byte DiskSet;             // Disk set to boot from if BootMode OS boot
};
const int configAddr = 0;

#endif
