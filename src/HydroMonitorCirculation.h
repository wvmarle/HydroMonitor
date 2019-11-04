#ifndef HYDROMONITORCIRCULATION_H
#define HYDROMONITORCIRCULATION_H

#include <HydroMonitorCore.h>
#include <HydroMonitorLogging.h>

#ifdef CIRCULATION_MCP_PIN
#include <Adafruit_MCP23008.h>
#elif defined(CIRCULATION_MCP17_PIN)
#include <Adafruit_MCP23017.h>
#endif

class HydroMonitorCirculation
{
  public:

    struct Settings {
      uint32_t latestCirculation;                              // UNIX timestamp.
      uint16_t drainageInterval;                            // Interval in days.
    };

    HydroMonitorCirculation(void);

    // Functions as required for all sensors.

#ifdef CIRCULATION_PIN
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*);
#elif defined(CIRCULATION_MCP_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, Adafruit_MCP23008*);
#elif defined(CIRCULATION_MCP17_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, Adafruit_MCP23017*);
#endif                                                      // endif pin definitions.
    void doCirculation(void);
    void settingsHtml(ESP8266WebServer*);
    bool settingsJSON(ESP8266WebServer*);
    void updateSettings(ESP8266WebServer*);

#ifdef CIRCULATION_MCP_PIN)
    Adafruit_MCP23008 *mcp;
#elif defined(CIRCULATION_MCP17_PIN)
    Adafruit_MCP23017 *mcp;
#endif

  private:
    Settings settings;
    HydroMonitorCore core;
    HydroMonitorCore::SensorData *sensorData;
    HydroMonitorLogging *logging;
    void switchPumpOn(void);
    void switchPumpOff(void);
    uint32_t lastWarned;
    uint32_t lastOff;
    bool pumpRunning;
};
#endif
