#include <Arduino.h>
#include "Utils.h"
#include "HwDefines.h"

void FlushRxBuffer()
{
  while (Serial.available() > 0)
  {
    Serial.read();
  }
}

void blinkIOSled(unsigned long *timestamp)
// Blink led IOS using a timestamp
{
  if ((millis() - *timestamp) > 200)
  {
    digitalWrite(LED_IOS,!digitalRead(LED_IOS));
    *timestamp = millis();
  }
}
