/*
 * HydroMonitorWaterLevelSensor.h
 * Library for measuring the water level using the HC-SR04 ultrasonic range finder.
 *
 * (C) Wouter van Marle / City Hydroponics
 * www.cityhydroponics.hk
 */
 

#ifndef HYDROMONITORWATERLEVELSENSOR_h
#define HYDROMONITORWATERLEVELSENSOR_h

#include <HydroMonitorCore.h>
#include <HydroMonitorLogging.h>
#include <HydroMonitorSensorBase.h>

#ifdef USE_HCSR04
#if defined(TRIG_PCF_PIN)
#include <pcf8574_esp.h>                                    // Needed for the optional port extender on TrigPin.
#elif defined(TRIG_MCP_PIN)
#include <Adafruit_MCP23008.h>
#endif
#elif defined(USE_MS5837)
#include <MS5837.h>
#elif defined(USE_DS1603L)
#include <DS1603L.h>
#elif defined (USE_FLOATSWITCHES)
#if defined(FLOATSWITCH_HIGH_MCP17_PIN) || defined(FLOATSWITCH_MEDIUM_MCP17_PIN) || defined(FLOATSWITCH_LOW_MCP17_PIN)
#include <Adafruit_MCP23017.h>
#endif
#endif

#define HCSR04SAMPLES 5                                     // Oversampling rate - power of 2 (5 = 32x, 6=64x).

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
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*);
#elif defined(TRIG_MCP_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, Adafruit_MCP23008*);
#elif defined(TRIG_PCF_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, PCF857x*);
#endif
#elif defined(USE_MS5837)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, MS5837*);
    void setZero(void);
#elif defined(USE_DS1603L)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, DS1603L*);
#elif defined(USE_MPXV5004)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*);
    void setZero(void);
    void setMax(void);
#elif defined(USE_FLOATSWITCHES)
#if defined(FLOATSWITCH_HIGH_MCP17_PIN) || defined(FLOATSWITCH_MEDIUM_MCP17_PIN) || defined(FLOATSWITCH_LOW_MCP17_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, Adafruit_MCP23017*);
#else
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*);
#endif


#endif

    void readSensor(bool readNow = false);
    void dataHtml(ESP8266WebServer*);                       // Provides html code with the sensor data.
    void settingsHtml(ESP8266WebServer*);
    bool settingsJSON(ESP8266WebServer*);
    void updateSettings(ESP8266WebServer*);

  private:
#ifdef USE_HCSR04
    float measureLevel(void);
#ifdef TRIG_PCF_PIN
    PCF857x *pcf8574;
#elif defined(TRIG_MCP_PIN)
    Adafruit_MCP23008 *mcp23008;
#endif

#elif defined(USE_MS5837)
    MS5837 *ms5837;
    
#elif defined(USE_DS1603L)
    DS1603L* ds1603l;

#elif defined(USE_FLOATSWITCHES)
#if defined(FLOATSWITCH_HIGH_MCP17_PIN) || defined(FLOATSWITCH_MEDIUM_MCP17_PIN) || defined(FLOATSWITCH_LOW_MCP17_PIN)
    Adafruit_MCP23017 *mcp23017;
#endif

#endif

    uint32_t lastWarned;
    HydroMonitorCore core;
    Settings settings;
    HydroMonitorCore::SensorData *sensorData;
    HydroMonitorLogging *logging;
    void warning(void);
};
#endif

