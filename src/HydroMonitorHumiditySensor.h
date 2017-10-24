/*
 * HydroMonitorHumiditySensor.h
 *
 * (C) Wouter van Marle / City Hydroponics
 * www.cityhydroponics.hk
 */
 
#ifndef HYDROMONITORHUMIDITYSENSOR_h
#define HYDROMONITORHUMIDITYSENSOR_h

#include <HydroMonitorCore.h>
#include <HydroMonitorMySQL.h>

#ifdef DHT22_PIN
#include <DHT22.h>
#endif
#ifdef USE_BME280
#include <BME280.h>
#endif

class HydroMonitorHumiditySensor
{
  public:
    
    struct Settings {
    };
    
    // Constructor.
    HydroMonitorHumiditySensor(void);
    
    // The functions required for all sensors.    
#ifdef DHT22_PIN
    void begin(HydroMonitorMySQL *logging, DHT22*);
#endif
#ifdef USE_BME280
    void begin(HydroMonitorMySQL *logging, BME280*);
#endif
    float readSensor(void);
    float calcDewPoint(float, float);
    String dataHtml(void);            // Provides html code with the sensor data.
    String settingsHtml(void);
    void updateSettings(String[], String[], uint8_t);

  private:
#ifdef DHT22_PIN
    DHT22 *dht22;
#endif
#ifdef USE_BME280
    BME280 *bme280;
#endif
    float humidity;

    HydroMonitorCore core;
    Settings settings;
    HydroMonitorMySQL *logging;
};
#endif

