/*
 * HydroMonitorBrightnessSensor.h
 * Library for the TSL2561 brightness probe.
 *
 * (C) Wouter van Marle / City Hydroponics
 * www.cityhydroponics.hk
 */
 
#ifndef BRIGHTNESS_h
#define BRIGHTNESS_h

#include <HydroMonitorBoardDefinitions.h>
#include <HydroMonitorCore.h>
#include <HydroMonitorMySQL.h>
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
    void begin(HydroMonitorCore::SensorData*, HydroMonitorMySQL*);
    void readSensor(void);
    String dataHtml(void);            // Provides html code with the sensor data.
    String settingsHtml(void);
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
    HydroMonitorMySQL *logging;
    HydroMonitorCore::SensorData *sensorData;
};

#endif
