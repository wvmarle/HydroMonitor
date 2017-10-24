/*
 * HydroMonitorTemperatureSensor.h
 *
 * (C) Wouter van Marle / City Hydroponics
 * www.cityhydroponics.hk
 */
 
#ifndef TEMPERATURESENSOR_h
#define TEMPERATURESENSOR_h

#include <HydroMonitorCore.h>
#include <HydroMonitorMySQL.h>
#ifdef DHT22_PIN
#include <DHT22.h>
#endif
#ifdef USE_BMP180
#include <BMP180.h>
#elif defined(USE_BMP280) || defined(USE_BME280)
#include <BME280.h>
#endif

class HydroMonitorTemperatureSensor
{
  public:

    struct Settings {
    };

    // Constructor.
    HydroMonitorTemperatureSensor(void);
    
    // The functions required for all sensors.
#ifdef DHT22_PIN
    void begin(HydroMonitorMySQL *logging, DHT22*);   // Use the DHT22 sensor.
#endif
#ifdef USE_BMP180
    void begin(HydroMonitorMySQL *logging, BMP180*);  // Use the BMP180 sensor.
#elif defined(USE_BMP280) || defined(USE_BME280)
    void begin(HydroMonitorMySQL *logging, BME280*);  // Use the BMP280 or BME280 sensor.
#endif
    float readSensor(void);
    String dataHtml(void);            // Provides html code with the sensor data.
    String settingsHtml(void);
    void updateSettings(String[], String[], uint8_t);

  private:
    Settings settings;
#ifdef DHT22_PIN
    DHT22 *dht22;
#endif
#ifdef USE_BMP180
    BMP180 *bmp180;
#elif defined(USE_BMP280) || defined(USE_BME280)
    BME280 *bmp280;
#endif
    float temperature;
    HydroMonitorCore core;
    HydroMonitorMySQL *logging;
};
#endif

