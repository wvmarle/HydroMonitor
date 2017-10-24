#ifndef HYDROMONITORFERTILISER_H
#define HYDROMONITORFERTILISER_H

#include <pcf8574_esp.h>
#include <HydroMonitorCore.h>
#include <Adafruit_MCP23008.h>
#include <HydroMonitorMySQL.h>

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
    void begin(HydroMonitorMySQL*);
#endif
#ifdef FERTILISER_A_PCF_PIN
    void begin(HydroMonitorMySQL*, PCF857x*);
#endif
#ifdef FERTILISER_A_MCP_PIN
    void begin(HydroMonitorMySQL*, Adafruit_MCP23008*);
#endif
    void doFertiliser(float);
    String settingsHtml(void);    
    void measurePumpA(void);
    void measurePumpB(void);
    void updateSettings(String[], String[], uint8_t);
    void setTargetEC(float);
    void setSolutionVolume(uint16_t);
    void setFertiliserConcentration(uint16_t);
    
  private:
  
    // Hardware parameters.
    uint8_t pumpA;
    uint8_t pumpB;
#ifdef FERTILISER_A_PCF_PIN
    PCF857x *pcf8574;
#endif
#ifdef FERTILISER_A_MCP_PIN
    Adafruit_MCP23008 *mcp23008;
#endif

    // Various parameters.
    float targetEC;
    uint16_t solutionVolume;
    uint16_t concentration;
    
    // Timing related variabls.
    uint32_t startTime;
    uint32_t runBTime;
    uint32_t runATime;
    uint32_t lastTimeAdded;
    uint32_t fertiliserDelay;
    uint32_t startATime;
    uint32_t lastGoodEC;
    
    // Various flags to keep track of what is going on.
    bool addA;
    bool addB;
    bool aRunning;
    bool bRunning;
    bool measuring;
    
    // Utility functions.
    void switchPumpOn(uint8_t);
    void switchPumpOff(uint8_t);

    Settings settings;
    HydroMonitorCore core;
    HydroMonitorMySQL *logging; 

};

#endif
