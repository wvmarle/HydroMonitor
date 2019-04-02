/*
 * HydroMonitorReservoir
 *
 * Manages the water reservoir level through a solenoid valve and with the help of the water level sensor.
 *
 */


/*
#ifdef USE_RESERVOIR
 
#ifndef USE_WATERLEVEL_SENSOR
#error Can't handle the reservoir filler without water level sensor.
#endif
*/
 
#ifndef HYDROMONITORRESERVOIR_H
#define HYDROMONITORRESERVOIR_H

#include <HydroMonitorWaterLevelSensor.h>
#include <HydroMonitorCore.h>
#include <HydroMonitorLogging.h>

#ifdef RESERVOIR_MCP_PIN
#include <Adafruit_MCP23008.h>
#elif defined(RESERVOIR_MCP17_PIN)
#include <Adafruit_MCP23017.h>
#elif defined(RESERVOIR_PCF_PIN)
#include <pcf8574_esp.h>
#endif

class HydroMonitorReservoir {

  public:
  
    struct Settings {
      uint8_t maxFill;
      uint8_t minFill;
    };
    
    HydroMonitorReservoir(void);     // The constructor.
#ifdef RESERVOIR_MCP_PIN
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, Adafruit_MCP23008*, HydroMonitorWaterLevelSensor*);
#elif defined(RESERVOIR_MCP17_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, Adafruit_MCP23017*, HydroMonitorWaterLevelSensor*);
#elif defined(RESERVOIR_PCF_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, PCF857x*, HydroMonitorWaterLevelSensor*);
#elif defined(RESERVOIR_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, HydroMonitorWaterLevelSensor*);
#endif
    void doReservoir(void);
    String settingsHtml(void);
    void updateSettings(String[], String[], uint8_t);
    
  private:
    HydroMonitorWaterLevelSensor *waterLevelSensor;
#ifdef RESERVOIR_MCP_PIN
    Adafruit_MCP23008 *mcp;
#elif defined(RESERVOIR_MCP17_PIN)
    Adafruit_MCP23017 *mcp;
#elif defined(RESERVOIR_PCF_PIN)
    PCF857x *pcf8574;
#endif

    // Timing related variables.
    uint32_t lastGoodFill;
    uint32_t lastLevelCheck;
    uint32_t startAddWater;
    uint32_t reservoirEmptyTime;
    uint32_t lastWarned;
    uint32_t lastBeep;
    
    bool oldBeep;
    bool beep;
    
    void openValve(void);
    void closeValve(void);
    Settings settings;
    HydroMonitorCore core;
    HydroMonitorCore::SensorData *sensorData;
    HydroMonitorLogging *logging;
    bool initialFillingDone;
    bool initialFillingInProgress;
    bool floatswitchTriggered;
    
    const uint32_t BEEP_FREQUENCY = 2000;
    const uint32_t BEEP_DURATION = 200;

};
#endif
//#endif
