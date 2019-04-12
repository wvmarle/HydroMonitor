/*
 * HydroMonitorWaterTempSensor.h
 * Library for the water temperature sensors: either an NTC or the MS5837 pressure sensor.
 *
 * (C) Wouter van Marle / City Hydroponics
 * www.cityhydroponics.hk
 */
 
// ensure this library description is only included once
#ifndef HYDROMONITORWATERTEMPSENSOR_h
#define HYDROMONITORWATERTEMPSENSOR_h

#include <HydroMonitorCore.h>
#include <HydroMonitorLogging.h>
#include <Arduino.h>
#include <HydroMonitorSensorBase.h>

#ifdef NTC_ADS_PIN
#include <Adafruit_ADS1015.h>
#elif defined(USE_MS5837)
#include <MS5837.h>
#elif defined(USE_DS18B20)
#include <OneWire.h>
#include <DallasTemperature.h>
#endif

#ifdef USE_NTC
#define NTCSAMPLES 4
#endif

class HydroMonitorWaterTempSensor: public HydroMonitorSensorBase
{
  public:

    struct Settings {
    };

    HydroMonitorWaterTempSensor();
#ifdef USE_NTC
#ifdef NTC_ADS_PIN
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, Adafruit_ADS1115*);
    void setADC(Adafruit_ADS1115);                          // Sets the ADS1115 ADC.
#elif defined(NTC_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*);
#endif
#elif defined(USE_MS5837)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, MS5837*);
#elif defined(USE_DS18B20)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, DallasTemperature*, OneWire*);
#elif defined(USE_ISOLATED_SENSOR_BOARD)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*);
#endif
    void readSensor(void);                                  // Read the temperature using the probe.
    void dataHtml(ESP8266WebServer*);                       // Provides html code with the sensor data.
    void settingsHtml(ESP8266WebServer*);
    bool settingsJSON(ESP8266WebServer*);
    void updateSettings(ESP8266WebServer*);

  private:
#ifdef NTC_ADS_PIN
    Adafruit_ADS1115 *ads1115;
#elif defined(USE_MS5837)
    MS5837 *ms5837;
#elif defined(USE_DS18B20)
    DallasTemperature *ds18b20;
    DeviceAddress deviceAddress;
#endif
    Settings settings;
    HydroMonitorCore::SensorData *sensorData;
    HydroMonitorLogging *logging;
    uint32_t lastWarned;
};

#endif

