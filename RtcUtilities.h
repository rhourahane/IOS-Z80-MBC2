#if !defined(_RTC_UTILITIES_H_)
#define _RTC_UTILITIES_H_
#include <time.h>
#include <Arduino.h>

// DS3231 RTC variables
extern byte foundRTC;
extern struct tm gtime;
extern int8_t tempC;

byte autoSetRTC();
void readRTC(struct tm &time, int8_t *tempC);
void ChangeRTC();

#endif
