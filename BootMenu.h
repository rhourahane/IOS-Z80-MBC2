#if !defined(_BOOT_MENU_H_)
#define _BOOT_MENU_H_
#include <Arduino.h>
#include "ConfigOptions.h"

const byte maxBootMode = 4;          // Default maximum allowed value for bootMode [0..4]

byte BootMenu(byte bootMode, byte maxDiskSet, byte foundRTC, ConfigOptions &options);

#endif
