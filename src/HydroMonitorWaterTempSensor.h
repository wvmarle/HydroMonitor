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

#include <HydroMonitorBoardDefinitions.h> // Required for this to compile.
#include <HydroMonitorCore.h>
#include <HydroMonitorMySQL.h>
#include <Arduino.h>

#ifdef NTC_ADS_PIN
#include <Adafruit_ADS1015.h>
#endif

#ifdef USE_MS5837
#include <MS5837.h>
#endif

#ifdef USE_NTC
#define NTCSAMPLES 4
#endif

class HydroMonitorWaterTempSensor
{
  public:

    struct Settings {
    };

    HydroMonitorWaterTempSensor();
#ifdef USE_NTC
#ifdef NTC_ADS_PIN
    void begin(HydroMonitorMySQL*, Adafruit_ADS1115*);
    void setADC(Adafruit_ADS1115);      // Sets the ADS1115 ADC.
#elif defined(NTC_PIN)
    void begin(HydroMonitorMySQL*);
#endif
#endif
#ifdef USE_MS5837
    void begin(HydroMonitorMySQL*, MS5837*);
#endif
    float readSensor(void);             // Read the temperature using the probe.
    String dataHtml(void);            // Provides html code with the sensor data.
    String settingsHtml(void);
    void updateSettings(String[], String[], uint8_t);

  private:
#ifdef NTC_ADS_PIN
    Adafruit_ADS1115 *ads1115;
#endif
    Settings settings;
#ifdef USE_MS5837
    MS5837 *ms5837;
#endif
    float waterTemp;
    HydroMonitorMySQL *logging;
};

#endif

