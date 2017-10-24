/*
 * HydroMonitorReservoir
 *
 * Manages the water reservoir level through a solenoid valve and with the help of the water level sensor.
 *
 */
 
#ifndef HYDROMONITORRESERVOIR_H
#define HYDROMONITORRESERVOIR_H

#include <HydroMonitorWaterLevelSensor.h>
#include <pcf8574_esp.h>
#include <Adafruit_MCP23008.h>
#include <HydroMonitorCore.h>
#include <HydroMonitorMySQL.h>

class HydroMonitorReservoir {

  public:
  
    struct Settings {
      uint8_t maxFill;
      uint8_t minFill;
    };
    
    HydroMonitorReservoir(void);     // The constructor.
#ifdef RESERVOIR_MCP_PIN
    void begin(HydroMonitorMySQL*, Adafruit_MCP23008*, HydroMonitorWaterLevelSensor*);
#elif defined(RESERVOIR_PCF_PIN)
    void begin(HydroMonitorMySQL*, PCF857x*, HydroMonitorWaterLevelSensor*);
#elif defined(RESERVOIR_PIN)
    void begin(HydroMonitorMySQL*, HydroMonitorWaterLevelSensor*);
#endif
#ifdef USE_MS5837
    void doReservoir(float, float);
#elif defined(USE_HCSR04)
    void doReservoir(float);
#endif
    String settingsHtml(void);
    void updateSettings(String[], String[], uint8_t);
    
  private:
    HydroMonitorWaterLevelSensor *waterLevelSensor;
#ifdef RESERVOIR_MCP_PIN
    Adafruit_MCP23008 *mcp23008;
#elif defined(RESERVOIR_PCF_PIN)
    PCF857x *pcf8574;
#endif
    bool addWater;
    uint32_t lastGoodFill;
    uint32_t lastLevel;
    uint32_t startAddWater;
    void openValve(void);
    void closeValve(void);

    Settings settings;
    HydroMonitorCore core;
    HydroMonitorMySQL *logging;
};
#endif
