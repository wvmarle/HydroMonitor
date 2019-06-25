#ifndef HYDROMONITORPHSENSOR_H
#define HYDROMONITORPHSENSOR_H

#ifdef PH_SENSOR_ADS_PIN
#include <Adafruit_ADS1015.h>
#endif
#ifdef PH_POS_MCP_PIN
#include <Adafruit_MCP23008.h>
#endif
//#include <Arduino.h>              // Needed for the String type.
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
#if defined(PH_SENSOR_PIN) || defined(USE_ISOLATED_SENSOR_BOARD)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging *logging);
#elif defined(PH_SENSOR_ADS_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging *log, Adafruit_ADS1115*);
#endif
    void readSensor(bool readNow = false);
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
