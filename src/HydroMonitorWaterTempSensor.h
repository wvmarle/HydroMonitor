/*
 * HydroMonitorWaterTempSensor.h
 * Library for the TSL2561 brightness probe.
 *
 * (C) Wouter van Marle / City Hydroponics
 * www.cityhydroponics.hk
 */
 

// ensure this library description is only included once
#ifndef WATERTEMP_h
#define WATERTEMP_h

#include <Arduino.h> // Needed for the String type.
#include <Adafruit_ADS1015.h>

class HydroMonitorWaterTempSensor
{
  public:

    struct Settings {
      unsigned char Samples;            // The number of samples required for a reading.
    };

    HydroMonitorWaterTempSensor();
    void begin(Settings, unsigned char, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, Adafruit_ADS1115);
    void begin(Settings, unsigned char, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);
    void setSettings(Settings);
    double readSensor(void);            // Read the temperature using the probe.
    void setADC(Adafruit_ADS1115);      // Sets the ADS1115 ADC.
    String settingsHtml(void);

  private:
    unsigned char sensorPin;                      // The pin the NTC is connected to.
    int seriesResistor;                 // The value of the series resistor.
    int NTCNominal;
    bool NTCPresent;
    int BCoefficient;
    float TNominal;
    int ADCMax;
    Adafruit_ADS1115 ads1115;
    bool useAds1115;                    // Whether we're using an ADS1115 or the internal ADC.
    Settings settings;

};

#endif

