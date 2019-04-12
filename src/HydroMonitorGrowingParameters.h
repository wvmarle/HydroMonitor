#ifndef HYDROMONITORGROWINGPARAMETERS_H
#define HYDROMONITORGROWINGPARAMETERS_H

#include <HydroMonitorCore.h>
#include <HydroMonitorLogging.h>

class HydroMonitorGrowingParameters {

  public:
  
    struct Settings {
      char systemName[65];
      uint16_t solutionVolume;
      uint16_t fertiliserConcentration;
      float targetEC;
      float pHMinusConcentration;
      float targetpH;
      float timezone;
    };

    // Constructor.    
    HydroMonitorGrowingParameters(void);

    // The various functions to set up and control this module.
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*);
    void settingsHtml(ESP8266WebServer*);    
    void settingsJSON(ESP8266WebServer*);    
    void updateSettings(ESP8266WebServer*);
    
    // The getters. Variables are set in updateSettings.
    uint16_t getSolutionVolume(void);
    float getTargetEC(void);
    uint16_t getFertiliserConcentration(void);
    float getpHMinusConcentration(void);
    float getTargetpH(void);
    
  private:
    void updateSensorData(void);
    Settings settings;
    HydroMonitorCore core;
    HydroMonitorCore::SensorData *sensorData;
    HydroMonitorLogging *logging;
};

#endif
