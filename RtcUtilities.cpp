#include "Wire.h"

#include "RtcUtilities.h"
#include "HwDefines.h"
#include "Utils.h"

// ------------------------------------------------------------------------------
// RTC Module routines
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
byte decToBcd(byte val);
byte bcdToDec(byte val);
byte isLeapYear(byte yearXX);
void printDateTime(byte readSourceFlag);

const byte daysOfMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
byte foundRTC;
byte seconds;
byte minutes;
byte hours;
byte day;
byte month;
byte year;
byte tempC;

void readRTC(byte *second, byte *minute, byte *hour, byte *day, byte *month, byte *year, byte *tempC)
// Read current date/time binary values and the temprerature (2 complement) from the DS3231 RTC
{
  byte    i;
  Wire.beginTransmission(DS3231_RTC);
  Wire.write(DS3231_SECRG);                       // Set the DS3231 Seconds Register
  Wire.endTransmission();
  // Read from RTC and convert to binary
  Wire.requestFrom(DS3231_RTC, 18);
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  Wire.read();                                    // Jump over the DoW
  *day = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
  for (i = 0; i < 10; i++) Wire.read();           // Jump over 10 registers
  *tempC = Wire.read();
}

// ------------------------------------------------------------------------------

void writeRTC(byte second, byte minute, byte hour, byte day, byte month, byte year)
// Write given date/time binary values to the DS3231 RTC
{
  Wire.beginTransmission(DS3231_RTC);
  Wire.write(DS3231_SECRG);                       // Set the DS3231 Seconds Register
  Wire.write(decToBcd(second));
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));
  Wire.write(1);                                  // Day of week not used (always set to 1 = Sunday)
  Wire.write(decToBcd(day));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.endTransmission();
}

// ------------------------------------------------------------------------------

byte autoSetRTC()
// Check if the DS3231 RTC is present and set the date/time at compile date/time if 
// the RTC "Oscillator Stop Flag" is set (= date/time failure).
// Return value: 0 if RTC not present, 1 if found.
{
  byte    OscStopFlag;

  Wire.beginTransmission(DS3231_RTC);
  if (Wire.endTransmission() != 0)
  {
    return 0;      // RTC not found
  }
  Serial.print(F("IOS: Found RTC DS3231 Module ("));
  printDateTime(1);
  Serial.println(F(")"));

  // Print the temperaturefrom the RTC sensor
  Serial.print(F("IOS: RTC DS3231 temperature sensor: "));
  Serial.print((int8_t)tempC);
  Serial.println(F("C"));
  
  // Read the "Oscillator Stop Flag"
  Wire.beginTransmission(DS3231_RTC);
  Wire.write(DS3231_STATRG);                      // Set the DS3231 Status Register
  Wire.endTransmission();
  Wire.requestFrom(DS3231_RTC, 1);
  OscStopFlag = Wire.read() & 0x80;               // Read the "Oscillator Stop Flag"

  if (OscStopFlag)
  // RTC oscillator stopped. RTC must be set at compile date/time
  {
    String  compTimeStr(F(" __TIME__"));    // Compile timestamp string
    String  compDateStr(F("__DATE__"));    // Compile datestamp string
    
    // Convert compile time strings to numeric values
    seconds = compTimeStr.substring(6,8).toInt();
    minutes = compTimeStr.substring(3,5).toInt();
    hours = compTimeStr.substring(0,2).toInt();
    day = compDateStr.substring(4,6).toInt();
    switch (compDateStr[0]) 
      {
        case 'J': month = compDateStr[1] == 'a' ? 1 : month = compDateStr[2] == 'n' ? 6 : 7; break;
        case 'F': month = 2; break;
        case 'A': month = compDateStr[2] == 'r' ? 4 : 8; break;
        case 'M': month = compDateStr[2] == 'r' ? 3 : 5; break;
        case 'S': month = 9; break;
        case 'O': month = 10; break;
        case 'N': month = 11; break;
        case 'D': month = 12; break;
      };
    year = compDateStr.substring(9,11).toInt();

    // Ask for RTC setting al compile date/time
    Serial.println(F("IOS: RTC clock failure!"));
    Serial.print(F("\nDo you want set RTC at IOS compile time ("));
    printDateTime(0);
    Serial.print(F(")? [Y/N] >"));
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
      writeRTC(seconds, minutes, hours, day, month, year);
      Serial.print(F("IOS: RTC set at compile time - Now: "));
      printDateTime(1);
      Serial.println();
    }

    // Reset the "Oscillator Stop Flag" 
    Wire.beginTransmission(DS3231_RTC);
    Wire.write(DS3231_STATRG);                    // Set the DS3231 Status Register
    Wire.write(0x08);                             // Reset the "Oscillator Stop Flag" (32KHz output left enabled)
    Wire.endTransmission();
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
    readRTC(&seconds, &minutes, &hours, &day,  &month,  &year, &tempC);
  }
  Serial.printf(F("%2d/%02d/%02d %02d:%02d:%02d"), day, month, year, hours, minutes, seconds);
}

// ------------------------------------------------------------------------------

void ChangeRTC()
// Change manually the RTC Date/Time from keyboard
{
  // Read RTC
  readRTC(&seconds, &minutes, &hours, &day,  &month,  &year, &tempC);

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
          Serial.printf(F(" Year -> %02d"), year);
        break;
        
        case 1:
          Serial.printf(F(" Month -> %02d"), month);
        break;

        case 2:
          Serial.printf(F(" Day -> %02d  "), day);
        break;

        case 3:
          Serial.printf(F(" Hours -> %02d"), hours);
        break;

        case 4:
          Serial.printf(F(" Minutes -> %02d"), minutes);
        break;

        case 5:
          Serial.printf(F(" Seconds -> %02d"), seconds);
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
            year++;
            if (year > 99) year = 0;
          break;

          case 1:
            month++;
            if (month > 12) month = 1;
          break;

          case 2:
            day++;
            if (month == 2)
            {
              if (day > (daysOfMonth[month - 1] + isLeapYear(year))) day = 1;
            }
            else
            {
              if (day > (daysOfMonth[month - 1])) day = 1;
            }
          break;

          case 3:
            hours++;
            if (hours > 23) hours = 0;
          break;

          case 4:
            minutes++;
            if (minutes > 59) minutes = 0;
          break;

          case 5:
            seconds++;
            if (seconds > 59) seconds = 0;
          break;
        }
      }
      if ((inChar == 'T'))
      {
        // Change tens
        switch (partIndex)
        {
          case 0:
            year = year + 10;
            if (year > 99) year = year - (year / 10) * 10; 
          break;

          case 1:
            if (month > 10) month = month - 10;
            else if (month < 3) month = month + 10;
          break;

          case 2:
            day = day + 10;
            if (day > (daysOfMonth[month - 1] + isLeapYear(year))) day = day - (day / 10) * 10;
            if (day == 0) day = 1;
          break;

          case 3:
            hours = hours + 10;
            if (hours > 23) hours = hours - (hours / 10 ) * 10;
          break;

          case 4:
            minutes = minutes + 10;
            if (minutes > 59) minutes = minutes - (minutes / 10 ) * 10;
          break;

          case 5:
            seconds = seconds + 10;
            if (seconds > 59) seconds = seconds - (seconds / 10 ) * 10;
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
  writeRTC(seconds, minutes, hours, day, month, year);
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
