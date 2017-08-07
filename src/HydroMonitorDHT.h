/* DHT library

MIT license
written by Adafruit Industries - stripped down by City Hydroponics.
*/
#ifndef HYDROMONITORDHT_H
#define HYDROMONITORDHT_H

#include "Arduino.h"

class HydroMonitorDHT {
  public:
   HydroMonitorDHT(void);
   void begin(uint8_t pin);
   float readTemperature(bool S=false);
   float convertCtoF(float);
   float readHumidity();
   boolean read(bool force=false);

 private:
  uint8_t data[5];
  uint8_t _pin, _type;
  #ifdef __AVR
    // Use direct GPIO access on an 8-bit AVR so keep track of the port and bitmask
    // for the digital pin connected to the DHT.  Other platforms will use digitalRead.
    uint8_t _bit, _port;
  #endif
  uint32_t _lastreadtime, _maxcycles;
  bool _lastresult;

  uint32_t expectPulse(bool level);

};

class InterruptLock {
  public:
   InterruptLock() {
    noInterrupts();
   }
   ~InterruptLock() {
    interrupts();
   }

};

#endif
