#if !defined(_RTC_UTILITIES_H_)
#define _RTC_UTILITIES_H_
#include <Arduino.h>

struct RtcTime
{
  byte second;
  byte minute;
  byte hour;
  byte day;
  byte month;
  byte year;
  int8_t tempC;
};

// DS3231 RTC variables
extern byte foundRTC;
extern RtcTime gtime;

byte autoSetRTC();
void readRTC(RtcTime &rtcTime);
void ChangeRTC();

#endif
