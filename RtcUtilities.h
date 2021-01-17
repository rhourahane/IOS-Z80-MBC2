#if !defined(_RTC_UTILITIES_H_)
#define _RTC_UTILITIES_H_
#include <Arduino.h>

// DS3231 RTC variables
extern byte foundRTC;
extern byte seconds;
extern byte minutes;
extern byte hours;
extern byte day;
extern byte month;
extern byte year;
extern byte tempC;

byte autoSetRTC();
void readRTC(byte *second, byte *minute, byte *hour, byte *day, byte *month, byte *year, byte *tempC);
void ChangeRTC();

#endif
