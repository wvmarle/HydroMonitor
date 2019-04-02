#ifndef HYDROMONITORFERTILISER_H
#define HYDROMONITORFERTILISER_H

#include <HydroMonitorCore.h>
#include <HydroMonitorLogging.h>

#ifdef FERTILISER_A_PCF_PIN
#include <pcf8574_esp.h>
#elif defined(FERTILISER_A_MCP_PIN)
#include <Adafruit_MCP23008.h>
#elif defined(FERTILISER_A_MCP17_PIN)
#include <Adafruit_MCP23017.h>
#endif

class HydroMonitorFertiliser {

  public:
    struct Settings {
      float pumpASpeed;
      float pumpBSpeed;
    };

    // Constructor.    
    HydroMonitorFertiliser(void);

    // The various functions to set up and control this module.
#ifdef FERTILISER_A_PIN
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*);
#elif defined(FERTILISER_A_PCF_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, PCF857x*);
#elif defined(FERTILISER_A_MCP_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, Adafruit_MCP23008*);
#elif defined(FERTILISER_A_MCP17_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, Adafruit_MCP23017*);
#endif
    void doFertiliser(void);
    void settingsHtml(ESP8266WebServer*);
    bool settingsJSON(ESP8266WebServer*);
    void measurePumpA(void);
    void measurePumpB(void);
    void updateSettings(ESP8266WebServer*);
    void doDrainageNow();
    
  private:
  
    // Hardware parameters.
    uint8_t pumpA;
    uint8_t pumpB;
#ifdef FERTILISER_A_PCF_PIN
    PCF857x *pcf8574;
#elif defined(FERTILISER_A_MCP_PIN)
    Adafruit_MCP23008 *mcp;
#elif defined(FERTILISER_A_MCP17_PIN)
  Adafruit_MCP23017 *mcp;
#endif

    // Timing related variables.
    uint32_t startTime;
    uint32_t runBTime;
    uint32_t runATime;
    uint32_t lastTimeAdded;
    uint32_t fertiliserDelay;
    uint32_t startATime;
    uint32_t lastGoodEC;
    uint32_t lastWarned;
    
    // Various flags to keep track of what is going on.
    bool addA;
    bool addB;
    bool aRunning;
    bool bRunning;
    bool measuring;
    float originalEC;
    
    // Utility functions.
    void switchPumpOn(uint8_t);
    void switchPumpOff(uint8_t);

    Settings settings;
    HydroMonitorCore core;
    HydroMonitorCore::SensorData *sensorData;
    HydroMonitorLogging *logging; 
};
#endif
