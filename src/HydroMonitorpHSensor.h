#ifndef HYDROMONITORPHSENSOR_H
#define HYDROMONITORPHSENSOR_H

#include <Adafruit_ADS1015.h>
#include <Arduino.h>              // Needed for the String type.
#include <HydroMonitorCore.h>
#include <ESP8266WebServer.h>
#include <HydroMonitorMySQL.h>

class HydroMonitorpHSensor {

  public:
  
    struct Settings {
    };
    
    HydroMonitorpHSensor(void);
#ifdef PH_SENSOR_PIN
    void begin(HydroMonitorCore::SensorData*, HydroMonitorMySQL *logging);
#elif defined(PH_SENSOR_ADS_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorMySQL *log, Adafruit_ADS1115*);
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
