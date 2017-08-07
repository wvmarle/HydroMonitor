/*
 * HydroMonitorAirTempSensor.h
 *
 * (C) Wouter van Marle / City Hydroponics
 * www.cityhydroponics.hk
 */
 
#ifndef AIRTEMPSENSOR_h
#define AIRTEMPSENSOR_h

#include <Arduino.h> // Needed for the String type.
#include <HydroMonitorDHT.h>

// library interface description
class HydroMonitorAirTempSensor
{
  public:

    struct Settings {
    };

//    struct Parameters {
//      bool enabled;
//    };
    
    HydroMonitorAirTempSensor(void);
    void begin(Settings, String, uint8_t);
    void setSettings(Settings);
    double readSensor(void);
    String settingsHtml(void);
//    Settings getDefaults(void);


  // library-accessible "private" interface
  private:
    bool airTempSensorPresent;
//    float temp;
    float readDHT22(void);
    Settings settings;
    String sensor;
    uint8_t sensorPin;
    HydroMonitorDHT DHT;

};
#endif

