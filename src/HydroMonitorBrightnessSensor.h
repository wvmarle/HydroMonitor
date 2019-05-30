/*
   HydroMonitorBrightnessSensor.h
   Library for the TSL2561 brightness probe.

   (C) Wouter van Marle / City Hydroponics
   www.cityhydroponics.hk
*/

#ifndef BRIGHTNESS_h
#define BRIGHTNESS_h

#include <HydroMonitorCore.h>
#include <HydroMonitorLogging.h>
#include <HydroMonitorSensorBase.h>

#include <EEPROM.h>
#include <Arduino.h>

#if defined(USE_TSL2561)
#include <TSL2561.h>
#elif defined(USE_TSL2591)
#include <TSL2591.h>
#endif

class HydroMonitorBrightnessSensor: public HydroMonitorSensorBase

{
  public:

    struct Settings {
    };

    HydroMonitorBrightnessSensor(void);
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*);
    void readSensor(void);
    void dataHtml(ESP8266WebServer*);            // Provides html code with the sensor data.
    void settingsHtml(ESP8266WebServer*);
    void updateSettings(String[], String[], uint8_t);

  private:
    void setSettings(Settings);
    Settings settings;
#if defined(USE_TSL2561)
    TSL2561 tsl = TSL2561(TSL2561_ADDR_FLOAT, 12345);
#elif defined(USE_TSL2591)
    TSL2591 tsl;
#endif
    bool brightnessSensorPresent;
    HydroMonitorLogging *logging;
    HydroMonitorCore::SensorData *sensorData;
};

#endif
