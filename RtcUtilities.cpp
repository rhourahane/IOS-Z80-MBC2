#include "Wire.h"

#include "RtcUtilities.h"
#include "HwDefines.h"
#include "WireUtils.h"
#include "Utils.h"

// ------------------------------------------------------------------------------
// RTC Module routines
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
byte decToBcd(byte val);
byte bcdToDec(byte val);
byte isLeapYear(byte yearXX);
void printDateTime(byte readSourceFlag);
void ChangeRTC();

const byte daysOfMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
byte foundRTC;
RtcTime gtime;

// Read current date/time binary values and the temprerature (2 complement) from the DS3231 RTC
void readRTC(RtcTime &rtcTime)
{
  byte buffer[18];
  ReadRegisters(DS3231_RTC, DS3231_SECRG, 18, buffer);
  
  byte *ptr = buffer;
  rtcTime.second = bcdToDec(*ptr++ & 0x7f);
  rtcTime.minute = bcdToDec(*ptr++);
  rtcTime.hour = bcdToDec(*ptr++ & 0x3f);
  ptr++;
  rtcTime.day = bcdToDec(*ptr++);
  rtcTime.month = bcdToDec(*ptr++);
  rtcTime.year = bcdToDec(*ptr++);

  ptr += 10;
  rtcTime.tempC = *ptr;
}

// ------------------------------------------------------------------------------

void writeRTC(RtcTime &rtcTime)
// Write given date/time binary values to the DS3231 RTC
{
  byte buffer[7];
  byte *ptr = buffer;

  *ptr++ = decToBcd(rtcTime.second);
  *ptr++ = decToBcd(rtcTime.minute);
  *ptr++ = decToBcd(rtcTime.hour);
  *ptr++ = 1;
  *ptr++ = decToBcd(rtcTime.day);
  *ptr++ = decToBcd(rtcTime.month);
  *ptr = decToBcd(rtcTime.year);

  WriteRegisters(DS3231_RTC, DS3231_SECRG, 7, buffer);
}

// ------------------------------------------------------------------------------

byte autoSetRTC()
// Check if the DS3231 RTC is present and set the date/time at compile date/time if 
// the RTC "Oscillator Stop Flag" is set (= date/time failure).
// Return value: 0 if RTC not present, 1 if found.
{
  byte    OscStopFlag;

  if (!ProbeAddress(DS3231_RTC))
  {
    // Rtc not present
    return 0;
  }
  
  Serial.print(F("IOS: Found RTC DS3231 Module ("));
  printDateTime(1);
  Serial.println(F(")"));

  // Print the temperaturefrom the RTC sensor
  Serial.print(F("IOS: RTC DS3231 temperature sensor: "));
  Serial.print(gtime.tempC);
  Serial.println(F("C"));
  
  // Read the "Oscillator Stop Flag"
  ReadRegisters(DS3231_RTC, DS3231_STATRG, 1, &OscStopFlag);
  OscStopFlag &= 0x80;

  // RTC oscillator stopped. RTC must be set at compile date/time
  if (OscStopFlag)
  {
    Serial.println(F("IOS: RTC clock failure!"));
    Serial.print(F("\nDo you want set RTC? [Y/N] >"));
    unsigned long timeStamp = millis();
    char inChar;
    do
    {
      blinkIOSled(&timeStamp);
      inChar = toupper(Serial.read());
    }
    while ((inChar != 'Y') && (inChar != 'N'));
    Serial.println(inChar);
 
    // Set the RTC at the compile date/time and print a message
    if (inChar == 'Y')
    {
      ChangeRTC();
      Serial.print(F("IOS: RTC set at compile time - Now: "));
      printDateTime(1);
      Serial.println();
    }

    // Reset the "Oscillator Stop Flag" 
    OscStopFlag = 0x08;
    WriteRegisters(DS3231_RTC, DS3231_STATRG, 1, &OscStopFlag);
  }
  return 1;
}

// ------------------------------------------------------------------------------

void printDateTime(byte readSourceFlag)
// Print to serial the current date/time from the global variables.
//
// Flag readSourceFlag [0..1] usage:
//    If readSourceFlag = 0 the RTC read is not done
//    If readSourceFlag = 1 the RTC read is done (global variables are updated)
{
  if (readSourceFlag)
  {
    readRTC(gtime);
  }
  Serial.printf(F("%02d/%02d/%02d %02d:%02d:%02d"), gtime.day, gtime.month, gtime.year, gtime.hour, gtime.minute, gtime.second);
}

// ------------------------------------------------------------------------------

void ChangeRTC()
// Change manually the RTC Date/Time from keyboard
{
  // Read RTC
  readRTC(gtime);

  // Change RTC date/time from keyboard
  byte partIndex = 0;
  char inChar = 0;
  Serial.println(F("\nIOS: RTC manual setting:"));
  Serial.println(F("\nPress T/U to increment +10/+1 or CR to accept"));
  do
  {
    do
    {
      switch (partIndex)
      {
        case 0:
          Serial.printf(F(" Year -> %02d"), gtime.year);
        break;
        
        case 1:
          Serial.printf(F(" Month -> %02d"), gtime.month);
        break;

        case 2:
          Serial.printf(F(" Day -> %02d  "), gtime.day);
        break;

        case 3:
          Serial.printf(F(" Hours -> %02d"), gtime.hour);
        break;

        case 4:
          Serial.printf(F(" Minutes -> %02d"), gtime.minute);
        break;

        case 5:
          Serial.printf(F(" Seconds -> %02d"), gtime.second);
        break;
      }

      unsigned long timeStamp = millis();
      do
      {
        blinkIOSled(&timeStamp);
        inChar = toupper(Serial.read());
      }
      while ((inChar != 'U') && (inChar != 'T') && (inChar != 13));
      
      if ((inChar == 'U'))
      {
        // Change units
        switch (partIndex)
        {
          case 0:
            gtime.year++;
            if (gtime.year > 99)
            {
              gtime.year = 0;
            }
          break;

          case 1:
            gtime.month++;
            if (gtime.month > 12)
            {
              gtime.month = 1;
            }
          break;

          case 2:
            gtime.day++;
            if (gtime.month == 2)
            {
              if (gtime.day > (daysOfMonth[gtime.month - 1] + isLeapYear(gtime.year)))
              {
                gtime.day = 1;
              }
            }
            else
            {
              if (gtime.day > (daysOfMonth[gtime.month - 1]))
              {
                gtime.day = 1;
              }
            }
          break;

          case 3:
            gtime.hour++;
            if (gtime.hour > 23)
            {
              gtime.hour = 0;
            }
          break;

          case 4:
            gtime.minute++;
            if (gtime.minute > 59)
            {
              gtime.minute = 0;
            }
          break;

          case 5:
            gtime.second++;
            if (gtime.second > 59)
            {
              gtime.second = 0;
            }
          break;
        }
      }
      if ((inChar == 'T'))
      {
        // Change tens
        switch (partIndex)
        {
          case 0:
            gtime.year = gtime.year + 10;
            if (gtime.year > 99)
            {
              gtime.year = gtime.year - (gtime.year / 10) * 10; 
            }
          break;

          case 1:
            if (gtime.month > 10)
            {
              gtime.month = gtime.month - 10;
            }
            else if (gtime.month < 3)
            {
              gtime.month = gtime.month + 10;
            }
          break;

          case 2:
            gtime.day = gtime.day + 10;
            if (gtime.day > (daysOfMonth[gtime.month - 1] + isLeapYear(gtime.year)))
            {
              gtime.day = gtime.day - (gtime.day / 10) * 10;
            }
            if (gtime.day == 0)
            {
              gtime.day = 1;
            }
          break;

          case 3:
            gtime.hour = gtime.hour + 10;
            if (gtime.hour > 23)
            {
              gtime.hour = gtime.hour - (gtime.hour / 10 ) * 10;
            }
          break;

          case 4:
            gtime.minute = gtime.minute + 10;
            if (gtime.minute > 59)
            {
              gtime.minute = gtime.minute - (gtime.minute / 10 ) * 10;
            }
          break;

          case 5:
            gtime.second = gtime.second + 10;
            if (gtime.second > 59)
            {
              gtime.second = gtime.second - (gtime.second / 10 ) * 10;
            }
          break;
        }
      }
      Serial.write(13);
    }
    while (inChar != 13);
    partIndex++;
  }
  while (partIndex < 6);  

  // Write new date/time into the RTC
  writeRTC(gtime);
  Serial.println(F(" ...done      "));
  Serial.print(F("IOS: RTC date/time updated ("));
  printDateTime(1);
  Serial.println(F(")"));
}



byte decToBcd(byte val)
// Convert a binary byte to a two digits BCD byte
{
  return( (val/10*16) + (val%10) );
}

// ------------------------------------------------------------------------------

byte bcdToDec(byte val)
// Convert binary coded decimal to normal decimal numbers
{
  return( (val/16*10) + (val%16) );
}

// ------------------------------------------------------------------------------

byte isLeapYear(byte yearXX)
// Check if the year 2000+XX (where XX is the argument yearXX [00..99]) is a leap year.
// Returns 1 if it is leap, 0 otherwise.
// This function works in the [2000..2099] years range. It should be enough...
{
  if (((2000 + yearXX) % 4) == 0) return 1;
  else return 0;
}
