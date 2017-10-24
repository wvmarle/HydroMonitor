#ifndef HYDROMONITORPHMINUS_H
#define HYDROMONITORPHMINUS_H

#include <pcf8574_esp.h>
#include <Adafruit_MCP23008.h>
#include <HydroMonitorCore.h>
#include <HydroMonitorMySQL.h>

class HydroMonitorpHMinus {

  public:
  
    struct Settings {
      float pumpSpeed;
    };
    
    HydroMonitorpHMinus(void);
#ifdef PHMINUS_PIN
    void begin(HydroMonitorCore::SensorData*, HydroMonitorMySQL*);
#elif defined(PHMINUS_PCF_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorMySQL*, PCF857x*);
#elif defined(PHMINUS_MCP_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorMySQL*, Adafruit_MCP23008*);
#endif
    void dopH(void);          // Handle the pH dosing.
    String settingsHtml(void);
    void measurePump(void);
    void updateSettings(String[], String[], uint8_t);
    
  private:
    Settings settings;
#ifdef PHMINUS_PCF_PIN
    PCF857x * pcf8574;
#elif defined(PHMINUS_MCP_PIN)
    Adafruit_MCP23008 *mcp23008;
#endif
    uint32_t startTime;       // When the pump was switched on.
    uint32_t runTime;         // For how long the pump has to run (in milliseconds)
    uint32_t lastTimeAdded;   // When the last dose of pH-minus was added.
    uint32_t pHDelay;         // How long to wait before starting to adding more pH-minus.
    uint32_t lastGoodpH;      // When the last good pH was seen.
    bool addpH;               // Flag that we have to start adding pH-minus.
    bool running;             // Flag that the pump is currently running.
    bool measuring;           // Flag that a flow measurement is going on.
    void switchPumpOn(void);  // Switch the pump on.
    void switchPumpOff(void); // Switch the pump off.
    HydroMonitorCore core;
    HydroMonitorCore::SensorData *sensorData;
    HydroMonitorMySQL *logging;
};

#endif
