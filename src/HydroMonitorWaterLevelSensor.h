/*
 * HydroMonitorWaterLevelSensor.h
 * Library for measuring the water level using the HC-SR04 ultrasonic range finder.
 *
 * (C) Wouter van Marle / City Hydroponics
 * www.cityhydroponics.hk
 */
 

#ifndef WATERLEVEL_h
#define WATERLEVEL_h

#include <Arduino.h>        // Needed for the String type.
#include <pcf8574_esp.h>    // Needed for the optional port extender on TrigPin.
//#include <Wire.h>

// library interface description
class HydroMonitorWaterLevelSensor
{
  public:
  
    struct Settings {
      double ReservoirHeight;
      unsigned char Samples;
    };

    HydroMonitorWaterLevelSensor(void);
    void begin(Settings, unsigned char, unsigned char, bool);
    void setSettings(Settings);
    double readSensor(void);
    void setPCF8574(PCF857x);
    String settingsHtml(void);

  // library-accessible "private" interface
  private:
    unsigned char trigPin;
    unsigned char echoPin;
    float measureLevel(void);
    bool sensorPresent;
    bool usePCF;
    Settings settings;

};

#endif

