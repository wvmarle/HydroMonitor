/*
 * HydroMonitorHumiditySensor.h
 *
 * (C) Wouter van Marle / City Hydroponics
 * www.cityhydroponics.hk
 */
 
#ifndef HUMIDITYSENSOR_h
#define HUMIDITYSENSOR_h

#include <Arduino.h> // Needed for the String type.
#include <HydroMonitorDHT.h>

// library interface description
class HydroMonitorHumiditySensor
{
  public:
    
    struct Settings {
    };
    
//    struct Parameters {
//      bool enabled;
//    };

    HydroMonitorHumiditySensor(void);    // Constructor.
    void begin(Settings, String, uint8_t);
    void setSettings(Settings);
    double readSensor(void);
    String settingsHtml(void);

  private:
    Settings settings;
    String sensor;
    bool humiditySensorPresent;
    uint8_t sensorPin;
    float readDHT22(void);
    HydroMonitorDHT DHT;
};

#endif

