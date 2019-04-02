#ifndef HYDROMONITORPHMINUS_H
#define HYDROMONITORPHMINUS_H

#include <HydroMonitorCore.h>
#include <HydroMonitorLogging.h>
#ifdef PHMINUS_PCF_PIN
#include <pcf8574_esp.h>
#elif defined(PHMINUS_MCP_PIN)
#include <Adafruit_MCP23008.h>
#elif defined(PHMINUS_MCP17_PIN)
#include <Adafruit_MCP23017.h>
#endif

class HydroMonitorpHMinus {

  public:
  
    struct Settings {
      float pumpSpeed;
    };
    
    HydroMonitorpHMinus(void);
#ifdef PHMINUS_PIN
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*);
#elif defined(PHMINUS_PCF_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, PCF857x*);
#elif defined(PHMINUS_MCP_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, Adafruit_MCP23008*);
#elif defined(PHMINUS_MCP17_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, Adafruit_MCP23017*);
#endif
    void dopH(void);          // Handle the pH dosing.
    void settingsHtml(ESP8266WebServer*);
    bool settingsJSON(ESP8266WebServer*);
    void measurePump(void);
    void updateSettings(ESP8266WebServer*);
    
  private:
    Settings settings;
#ifdef PHMINUS_PCF_PIN
    PCF857x * pcf8574;
#elif defined(PHMINUS_MCP_PIN)
    Adafruit_MCP23008 *mcp;
#elif defined(PHMINUS_MCP17_PIN)
    Adafruit_MCP23017 *mcp;
#endif

    // Timing related variables.
    uint32_t startTime;       // When the pump was switched on.
    uint32_t runTime;         // For how long the pump has to run (in milliseconds)
    uint32_t lastTimeAdded;   // When the last dose of pH-minus was added.
    uint32_t pHDelay;         // How long to wait before starting to adding more pH-minus.
    uint32_t lastGoodpH;      // When the last good pH was seen.
    uint32_t lastWarned;
    
    // Various flags to keep track of what's going on.
    bool addpH;               // Flag that we have to start adding pH-minus.
    bool running;             // Flag that the pump is currently running.
    bool measuring;           // Flag that a flow measurement is going on.
    float originalpH;

    void switchPumpOn(void);  // Switch the pump on.
    void switchPumpOff(void); // Switch the pump off.

    HydroMonitorCore core;
    HydroMonitorCore::SensorData *sensorData;
    HydroMonitorLogging *logging;
};
#endif
