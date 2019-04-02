#ifndef HYDROMONITORPHSENSOR_H
#define HYDROMONITORPHSENSOR_H

#include <Adafruit_ADS1015.h>
#include <Adafruit_MCP23008.h>
#include <Arduino.h>              // Needed for the String type.
#include <HydroMonitorCore.h>
#include <ESP8266WebServer.h>
#include <HydroMonitorLogging.h>
#include <HydroMonitorSensorBase.h>

class HydroMonitorpHSensor: public HydroMonitorSensorBase 
{
  public:
  
    struct Settings {
    };
    
    HydroMonitorpHSensor(void);
#ifdef PH_SENSOR_PIN
#ifdef PH_POS_MCP_PIN
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging *logging, Adafruit_MCP23008*);
#else
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging *logging);
#endif
#elif defined(PH_SENSOR_ADS_PIN)
#ifdef PH_POS_MCP_PIN
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging *log, Adafruit_ADS1115*, Adafruit_MCP23008*);
#else
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging *log, Adafruit_ADS1115*);
#endif
#endif
    void readSensor(void);
    void dataHtml(ESP8266WebServer*);            // Provides html code with the sensor data.
    void settingsHtml(ESP8266WebServer*); 
    bool settingsJSON(ESP8266WebServer*); 
    void getCalibrationHtml(ESP8266WebServer*);
    void updateSettings(ESP8266WebServer*);

    void getCalibration(ESP8266WebServer*);
    void doCalibration(ESP8266WebServer*);
    void deleteCalibration(ESP8266WebServer*);
    void enableCalibration(ESP8266WebServer*);
    void doCalibrationAction(ESP8266WebServer*);
    
  private:
    uint32_t takeReading(void);
#ifdef PH_SENSOR_ADS_PIN
    Adafruit_ADS1115 *ads1115;
#endif
#ifdef PH_POS_MCP_PIN
    Adafruit_MCP23008* mcp23008;
#endif
    Settings settings;
    void readCalibration();
    void saveCalibrationData();
  
    float calibratedSlope;
    float calibratedIntercept;
    Datapoint calibrationData[DATAPOINTS];
    uint32_t lastWarned;
    HydroMonitorCore core;
    HydroMonitorCore::SensorData *sensorData;
    HydroMonitorLogging *logging;
};

#endif
