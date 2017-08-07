/*
 * HydroMonitorAirPressureSensor.h
 * Library for the BMP180 atmospheric pressure sensor.
 *
 * (C) Wouter van Marle / City Hydroponics
 * www.cityhydroponics.hk
 */
 
#ifndef PRESSURE_h
#define PRESSURE_h

#include <SFE_BMP180.h>

// library interface description
class HydroMonitorAirPressureSensor
{
  public:

    struct Parameters {
      bool enabled;
    };
    
    struct Settings {
      int Altitude;
    };

    HydroMonitorAirPressureSensor(void);
    void begin(Settings);
    bool getStatus(void);
    double readSensor(void);
    Settings getDefaults(void);
    void setSettings(Settings);
    String settingsHtml(void);

  // library-accessible "private" interface
  private:
    SFE_BMP180 pressureSensor;
    bool pressureSensorPresent;
    float pressure;
    Settings settings;

};

#endif

