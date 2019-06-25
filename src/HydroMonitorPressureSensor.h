/*
 * HydroMonitorPressureSensor.h
 * Library for the BMP180 atmospheric pressure sensor.
 *
 * (C) Wouter van Marle / City Hydroponics
 * www.cityhydroponics.hk
 */
 
#ifndef PRESSURESENSOR_h
#define PRESSURESENSOR_h

#include <HydroMonitorCore.h>
#include <HydroMonitorLogging.h>
#include <HydroMonitorSensorBase.h>

#ifdef USE_BMP180
#include <BMP180.h>
#elif defined(USE_BMP280) || defined(USE_BME280)
#include <BME280.h>
#endif

class HydroMonitorPressureSensor: public HydroMonitorSensorBase
{
  public:

    struct Settings {
      int16_t altitude;
    };

    HydroMonitorPressureSensor(void);
#ifdef USE_BMP180
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, BMP180*);
#elif defined(USE_BMP280) || defined(USE_BME280)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, BME280*);
#endif
    void readSensor(bool readNow = false);
    void dataHtml(ESP8266WebServer * server);               // Provides html code with the sensor data.
    void settingsHtml(ESP8266WebServer * server);
    bool settingsJSON(ESP8266WebServer * server);
    void updateSettings(ESP8266WebServer * server);

  private:
#ifdef USE_BMP180
    BMP180 *bmp180;
#elif defined(USE_BMP280) || defined(USE_BME280)
    BME280 *bmp280;
#endif
    Settings settings;
    HydroMonitorCore core;
    HydroMonitorCore::SensorData *sensorData;
    HydroMonitorLogging *logging;
};  

#endif

