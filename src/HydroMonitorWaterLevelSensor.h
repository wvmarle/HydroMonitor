/*
 * HydroMonitorWaterLevelSensor.h
 * Library for measuring the water level using the HC-SR04 ultrasonic range finder.
 *
 * (C) Wouter van Marle / City Hydroponics
 * www.cityhydroponics.hk
 */
 

#ifndef HYDROMONITORWATERLEVELSENSOR_h
#define HYDROMONITORWATERLEVELSENSOR_h

#include <Arduino.h>        // Needed for the String type.
#include <HydroMonitorCore.h>
#include <HydroMonitorBoardDefinitions.h>
#include <HydroMonitorMySQL.h>
#include <HydroMonitorSensorBase.h>

#if defined(TRIG_PCF_PIN)
#include <pcf8574_esp.h>    // Needed for the optional port extender on TrigPin.
#elif defined(TRIG_MCP_PIN)
#include <Adafruit_MCP23008.h>
#endif
#if defined(USE_MS5837)
#include <MS5837.h>
#endif

#define HCSR04SAMPLES 5 // Oversampling rate - power of 2 (5 = 32x, 6=64x).

class HydroMonitorWaterLevelSensor: public HydroMonitorSensorBase
{
  public:
  
    struct Settings {
      float reservoirHeight;
      float zeroLevel;
    };

    HydroMonitorWaterLevelSensor(void);
#ifdef USE_HCSR04
#ifdef TRIG_PIN
    void begin(HydroMonitorCore::SensorData*, HydroMonitorMySQL*);
#elif defined(TRIG_MCP_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorMySQL*, Adafruit_MCP23008*);
#elif defined(TRIG_PCF_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorMySQL*, PCF857x*);
#endif
#endif

#ifdef USE_MS5837
    void begin(HydroMonitorCore::SensorData*, HydroMonitorMySQL*, MS5837*);
    void setZero(void);
#endif

    void readSensor(void);
    String dataHtml(void);            // Provides html code with the sensor data.
    String settingsHtml(void);
    void updateSettings(String[], String[], uint8_t);

  private:
#ifdef USE_HCSR04
    float measureLevel(void);
#endif
#ifdef USE_MS5837
    MS5837 *ms5837;
#endif

#ifdef TRIG_PCF_PIN
    PCF857x *pcf8574;
#elif defined(TRIG_MCP_PIN)
    Adafruit_MCP23008 *mcp23008;
#endif
    uint32_t lastWarned;
    HydroMonitorCore core;
    Settings settings;
    HydroMonitorCore::SensorData *sensorData;
    HydroMonitorMySQL *logging;
    void warning(void);
};
#endif

