#ifndef HYDROMONITORDRAINAGE_H
#define HYDROMONITORDRAINAGE_H

#include <HydroMonitorCore.h>
#include <HydroMonitorLogging.h>
#include <HydroMonitorSensorBase.h>
#include <HydroMonitorWaterLevelSensor.h>

#ifdef DRAINAGE_MCP_PIN
#include <Adafruit_MCP23008.h>
#elif defined(DRAINAGE_MCP17_PIN)
#include <Adafruit_MCP23017.h>
#endif

class HydroMonitorDrainage
{
  public:

    struct Settings {
      uint32_t latestDrainage;                              // UNIX timestamp.
      uint16_t drainageInterval;                            // Interval in days.
    };

    HydroMonitorDrainage(void);

    // Functions as required for all sensors.

#ifdef USE_WATERLEVEL_SENSOR
#ifdef DRAINAGE_PIN
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, HydroMonitorWaterLevelSensor*);
#elif defined(DRAINAGE_MCP_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, Adafruit_MCP23008*, HydroMonitorWaterLevelSensor*);
#elif defined(DRAINAGE_MCP17_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, Adafruit_MCP23017*, HydroMonitorWaterLevelSensor*);
#endif
#else                                                       // Not using water level sensor.
#ifdef DRAINAGE_PIN
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*);
#elif defined(DRAINAGE_MCP_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, Adafruit_MCP23008*);
#elif defined(DRAINAGE_MCP17_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, Adafruit_MCP23017*);
#endif                                                      // endif pin definitions.
#endif                                                      // endif USE_WATERLEVEL_SENSOR
    void doDrainage(void);
    void settingsHtml(ESP8266WebServer*);
    bool settingsJSON(ESP8266WebServer*);
    void updateSettings(ESP8266WebServer*);
    void drainStart(void);
    void drainStop(void);

#ifdef DRAINAGE_MCP_PIN)
    Adafruit_MCP23008 *mcp;
#elif defined(DRAINAGE_MCP17_PIN)
    Adafruit_MCP23017 *mcp;
#endif

  private:
    Settings settings;
    HydroMonitorCore core;
    HydroMonitorCore::SensorData *sensorData;
    HydroMonitorLogging *logging;
    uint32_t timeStartCounting;
    void switchPumpOn(void);
    void switchPumpOff(void);
    uint32_t lastLevelCheck;
    uint32_t drainageStart;
    bool autoDrainageMode();
    uint32_t lastWarned;

    enum DrainageStates {
      DRAINAGE_IDLE,

      DRAINAGE_AUTOMATIC_DRAINING_START,
      DRAINAGE_AUTOMATIC_DRAINING_RUNNING,
      DRAINAGE_AUTOMATIC_DRAINING_COMPLETE,

      DRAINAGE_MANUAL_DRAINING_START,
      DRAINAGE_MANUAL_DRAINING_RUNNING,
      DRAINAGE_MANUAL_DRAINING_HOLD_EMPTY,
      DRAINAGE_MANUAL_DRAINING_COMPLETE,

      DRAINAGE_MAINTENANCE_RUN,
      DRAINAGE_MAINTENANCE_RUNNING,

      DRAINAGE_DRAIN_EXCESS,
      DRAINAGE_DRAIN_EXCESS_RUNNING,
    };
    DrainageStates drainageState;
    uint32_t drainageCompletedTime;
    HydroMonitorWaterLevelSensor *waterLevelSensor;
#ifdef USE_WATERLEVEL_SENSOR
    uint32_t lastGoodFill;
#endif
    uint32_t lastDrainageRun;
};
#endif
