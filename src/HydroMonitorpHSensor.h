#ifndef HYDROMONITORPHSENSOR_H
#define HYDROMONITORPHSENSOR_H

#include <Adafruit_ADS1015.h>
#include <Adafruit_MCP23008.h>
#include <Arduino.h>              // Needed for the String type.
#include <HydroMonitorCore.h>
#include <ESP8266WebServer.h>
#include <HydroMonitorMySQL.h>
#include <HydroMonitorSensorBase.h>

class HydroMonitorpHSensor: public HydroMonitorSensorBase 
{
  public:
  
    struct Settings {
    };
    
    HydroMonitorpHSensor(void);
#ifdef PH_SENSOR_PIN
#ifdef PH_POS_MCP_PIN
    void begin(HydroMonitorCore::SensorData*, HydroMonitorMySQL *logging, Adafruit_MCP23008*);
#else
    void begin(HydroMonitorCore::SensorData*, HydroMonitorMySQL *logging);
#endif
#elif defined(PH_SENSOR_ADS_PIN)
#ifdef PH_POS_MCP_PIN
    void begin(HydroMonitorCore::SensorData*, HydroMonitorMySQL *log, Adafruit_ADS1115*, Adafruit_MCP23008*);
#else
    void begin(HydroMonitorCore::SensorData*, HydroMonitorMySQL *log, Adafruit_ADS1115*);
#endif
#endif
    void readSensor(void);
    String dataHtml(void);            // Provides html code with the sensor data.
    String settingsHtml(void); 
    String getCalibrationHtml(void);
    String getCalibrationData(void);
    void updateSettings(String[], String[], uint8_t);
    void doCalibration(ESP8266WebServer*);
    
  private:
    uint32_t takeReading(void);
#ifdef PH_SENSOR_ADS_PIN
    Adafruit_ADS1115 *ads1115;
#endif
#ifdef PH_POS_MCP_PIN
    Adafruit_MCP23008* mcp23008;
#endif
    Settings settings;
    void readCalibration(void);

    float calibratedSlope;
    float calibratedIntercept;
    uint32_t timestamp[DATAPOINTS];
    float value[DATAPOINTS];
    uint32_t reading[DATAPOINTS];
    bool enabled[DATAPOINTS];
    uint32_t lastWarned;
    HydroMonitorCore core;
    HydroMonitorCore::SensorData *sensorData;
    HydroMonitorMySQL *logging;
};

#endif
