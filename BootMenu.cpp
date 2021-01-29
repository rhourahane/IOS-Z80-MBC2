#include <EEPROM.h>

#include "BootMenu.h"
#include "HwDefines.h"
#include "RtcUtilities.h"
#include "OsBootInfo.h"
#include "Utils.h"

extern void PrintSystemOptions(const ConfigOptions &options, const __FlashStringHelper *msg);

byte ChangeDiskSet(byte curDiskSet, byte maxDiskSet);

byte BootMenu(byte bootMode, byte maxDiskSet, byte foundRTC, ConfigOptions &options)
{
  char minBootChar   = '1';        // Minimum allowed ASCII value selection (boot selection)
  char maxSelChar    = '8';        // Maximum allowed ASCII value selection (boot selection)
  char inChar;
  
  do
  {
    FlushRxBuffer();
    Serial.println(F("\n\rIOS: Select boot mode or system parameters:\n\r"));
    if (bootMode <= maxBootMode)
    // Previous valid boot mode read, so enable '0' selection
    {
      minBootChar = '0';
      Serial.printf(F(" 0: No change (%d)\r\n"), bootMode + 1);
    }
    Serial.println(F(" 1: Basic"));   
    Serial.println(F(" 2: Forth"));
    Serial.print(F(" 3: Load OS from "));
    printOsName(options.DiskSet);
    Serial.println(F("\r\n 4: Autoboot"));
    Serial.println(F(" 5: iLoad"));
    Serial.printf(F(" 6: Change Z80 clock speed (->%dMHz)\r\n"), options.ClockMode ? CLOCK_LOW : CLOCK_HIGH);
    Serial.print(F(" 7: Toggle CP/M Autoexec (->"));
    if (options.AutoexecFlag) Serial.print(F("ON"));
    else Serial.print(F("OFF"));
    Serial.println(F(")"));
    Serial.print(F(" 8: Change "));
    printOsName(options.DiskSet);
    Serial.println();

    // If RTC module is present add a menu choice
    if (foundRTC)
    {
      Serial.println(F(" 9: Change RTC time/date"));
      maxSelChar = '9';
    }

    // Ask a choice
    Serial.println();
    unsigned long timeStamp = millis();
    Serial.print(F("Enter your choice >"));
    do
    {
      blinkIOSled(&timeStamp);
      inChar = Serial.read();
    }               
    while ((inChar < minBootChar) || (inChar > maxSelChar));
    Serial.print(inChar);
    Serial.println(F("  Ok"));

    // Make the selected action for the system paramters choice
    switch (inChar)
    {
      case '6':                                   // Change the clock speed of the Z80 CPU
        options.ClockMode = !options.ClockMode;                   // Toggle Z80 clock speed mode (High/Low)
      break;

      case '7':                                   // Toggle CP/M AUTOEXEC execution on cold boot
        options.AutoexecFlag = !options.AutoexecFlag;             // Toggle AUTOEXEC executiont status
      break;

      case '8':                                   // Change current Disk Set
        options.DiskSet = ChangeDiskSet(options.DiskSet, maxDiskSet);
      break;

      case '9':                                   // Change RTC Date/Time
        ChangeRTC();                              // Change RTC Date/Time if requested
      break;
    }
  } while (inChar > '5');
      
  // Save selectd boot program if changed
  bootMode = inChar - '1';                      // Calculate bootMode from inChar
  if (bootMode <= maxBootMode)
  {
    options.BootMode = bootMode; // Save to the internal EEPROM if required
  }
  else
  {
    bootMode = options.BootMode;    // Reload boot mode if '0' or > '5' choice selected
  }

  EEPROM.put(configAddr, options);
  PrintSystemOptions(options, F("Saved updated "));

  return bootMode;
}

byte ChangeDiskSet(byte curDiskSet, byte maxDiskSet)
{
  byte newSet = maxDiskSet + 1;
  Serial.println();
  Serial.print(F("Current selection: "));
  printOsName(curDiskSet);
  Serial.println();
  Serial.println();
  do
  {
    for (int setNum = 0; setNum != maxDiskSet; ++setNum)
    {
      OsBootInfo tmpInfo = GetDiskSetBootInfo(setNum);
      Serial.printf(F(" %d: Disk Set %d (%s)\n\r"), setNum, setNum, tmpInfo.OsName);
    }
    Serial.print(F("Enter your choice or ESC to return>"));
    unsigned long timeStamp = millis();
    FlushRxBuffer();
    while(Serial.available() < 1)
    {
      blinkIOSled(&timeStamp);  // Wait a key
    }
    char inChar = Serial.read();
    if (isDigit(inChar))
    {
      newSet = inChar - '0';
    }
    else if (inChar == 27)
    {
      newSet = curDiskSet;
    }
  } while (newSet >= maxDiskSet);
  Serial.println(F("   Ok"));
  return newSet;
}
