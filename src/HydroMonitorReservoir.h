/*
   HydroMonitorReservoir

   Manages the water reservoir level through a solenoid valve and with the help of the water level sensor.

*/


/*
  #ifdef USE_RESERVOIR

  #ifndef USE_WATERLEVEL_SENSOR
  #error Can't handle the reservoir filler without water level sensor.
  #endif
*/

#ifndef HYDROMONITORRESERVOIR_H
#define HYDROMONITORRESERVOIR_H

#include <HydroMonitorCore.h>
#include <HydroMonitorLogging.h>
#ifdef USE_WATERLEVEL_SENSOR
#include <HydroMonitorWaterLevelSensor.h>
#endif

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

    HydroMonitorReservoir(void);                            // The constructor.
#ifdef USE_WATERLEVEL_SENSOR                                // If we use the water level sensor, this set of constructors.
#ifdef RESERVOIR_MCP_PIN                                    // Check which pin type is defined.
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, Adafruit_MCP23008*, HydroMonitorWaterLevelSensor*);
#elif defined(RESERVOIR_MCP17_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, Adafruit_MCP23017*, HydroMonitorWaterLevelSensor*);
#elif defined(RESERVOIR_PCF_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, PCF857x*, HydroMonitorWaterLevelSensor*);
#elif defined(RESERVOIR_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, HydroMonitorWaterLevelSensor*);
#endif                                                      // #endif of the pin definitions.
#else                                                       // Not using the water level sensor.
#ifdef RESERVOIR_MCP_PIN                                    // Check which pin type is defined.
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, Adafruit_MCP23008*);
#elif defined(RESERVOIR_MCP17_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, Adafruit_MCP23017*);
#elif defined(RESERVOIR_PCF_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, PCF857x*);
#elif defined(RESERVOIR_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*);
#endif                                                      // #endif of the pin definitions.
#endif                                                      // #endif of USE_WATERLEVEL_SENSOR
    void doReservoir(void);
    String settingsHtml(void);
    void updateSettings(String[], String[], uint8_t);

  private:
#ifdef USE_WATERLEVEL_SENSOR
    HydroMonitorWaterLevelSensor *waterLevelSensor;
#endif
#ifdef RESERVOIR_MCP_PIN
    Adafruit_MCP23008 *mcp;
#elif defined(RESERVOIR_MCP17_PIN)
    Adafruit_MCP23017 *mcp;
#elif defined(RESERVOIR_PCF_PIN)
    PCF857x *pcf8574;
#endif

    // Timing related variables.
#ifdef USE_WATERLEVEL_SENSOR
    uint32_t lastGoodFill;
    uint32_t lastLevelCheck;
#endif
#ifndef USE_WATERLEVEL_SENSOR
    uint32_t lastClear;
#endif
    uint32_t startAddWater;
    uint32_t reservoirEmptyTime;
    uint32_t lastWarned = -WARNING_INTERVAL;
    uint32_t lastBeep;

    bool oldBeep;
    bool beep;

    void openValve(void);
    void closeValve(void);
    Settings settings;
    HydroMonitorCore core;
    HydroMonitorCore::SensorData *sensorData;
    HydroMonitorLogging *logging;
#ifdef USE_WATERLEVEL_SENSOR
    bool initialFillingDone;
    bool initialFillingInProgress;
#else
    bool isWeeklyTopUp;
#endif
    bool floatswitchTriggered;

    const uint32_t BEEP_FREQUENCY = 500;
    const uint32_t BEEP_DURATION = 200;

};
#endif
