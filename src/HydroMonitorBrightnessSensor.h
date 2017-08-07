/*
 * HydroMonitorBrightnessSensor.h
 * Library for the TSL2561 brightness probe.
 *
 * (C) Wouter van Marle / City Hydroponics
 * www.cityhydroponics.hk
 */
 

// ensure this library description is only included once
#ifndef BRIGHTNESS_h
#define BRIGHTNESS_h

#include <Adafruit_TSL2561_U.h>

// library interface description
class HydroMonitorBrightnessSensor
{
  // user-accessible "public" interface
  public:
    
    struct Settings {
    };

    HydroMonitorBrightnessSensor(void);
    void begin(Settings);
    int readSensor(void);
    String settingsHtml(void);
    void setSettings(Settings);
        
  // library-accessible "private" interface
  private:
    Settings settings;
    bool brightnessSensorPresent;
    Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

};

#endif

