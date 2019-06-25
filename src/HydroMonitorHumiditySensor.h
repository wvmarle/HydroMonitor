/*
   HydroMonitorHumiditySensor.h

   (C) Wouter van Marle / City Hydroponics
   www.cityhydroponics.hk
*/

#ifndef HYDROMONITORHUMIDITYSENSOR_h
#define HYDROMONITORHUMIDITYSENSOR_h

#include <HydroMonitorCore.h>
#include <HydroMonitorLogging.h>
#include <HydroMonitorSensorBase.h>

#ifdef USE_DHT22
#include <DHT22.h>
#endif
#ifdef USE_BME280
#include <BME280.h>
#endif

class HydroMonitorHumiditySensor: public HydroMonitorSensorBase
{
  public:

    struct Settings {
    };

    // Constructor.
    HydroMonitorHumiditySensor(void);

    // The functions required for all sensors.
#ifdef USE_DHT22
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging *logging, DHT22*);
#endif
#ifdef USE_BME280
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging *logging, BME280*);
#endif
    void readSensor(bool readNow = false);
    float calcDewpoint(float, float);
    float calcDewpoint(void);
    void dataHtml(ESP8266WebServer *);                      // Provides html code with the sensor data.
    void settingsHtml(ESP8266WebServer * server);
    bool settingsJSON(ESP8266WebServer * server);
    void updateSettings(ESP8266WebServer * server);

  private:
#ifdef USE_DHT22
    DHT22 *dht22;
#endif
#ifdef USE_BME280
    BME280 *bme280;
#endif

    HydroMonitorCore core;
    Settings settings;
    HydroMonitorCore::SensorData *sensorData;
    HydroMonitorLogging *logging;
};
#endif

