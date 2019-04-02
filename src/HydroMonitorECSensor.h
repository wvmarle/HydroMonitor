/*
 * HydroMonitorECSensor.h
 * Library for the EC probe.
 *
 * (C) Wouter van Marle / City Hydroponics
 * www.cityhydroponics.hk
 *
 * This library controls a simple but very effective EC probe.
 *
 * Return values:
 * - the EC, a positive number (in normal operation - normally in mS/cm, depending on the calibration values).
 * - -1 (no probe detected).
 */

#ifndef EC_SENSOR_h
#define EC_SENSOR_h

#include <HydroMonitorCore.h>
#include <Average.h>
#include <ESP8266WebServer.h>
#include <Time.h>
#include <HydroMonitorLogging.h>
#include <HydroMonitorSensorBase.h>

#define ECSAMPLES 6                   // Take 2^ECSAMPLES = 64 samples to produce a single reading.

class HydroMonitorECSensor: public HydroMonitorSensorBase
{
  public:
    struct Settings {
    };
    
    HydroMonitorECSensor();           // The constructor.
    
    // The functions required for all sensors.
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*);
    void readSensor(void);            // Measures the EC value, takes the water temperature as input, returns the result.
    void dataHtml(ESP8266WebServer*);            // Provides html code with the sensor data.
    void settingsHtml(ESP8266WebServer*);        // Provides html code with the settings.
    bool settingsJSON(ESP8266WebServer*);        // Provides JSON code with the settings.
    void updateSettings(ESP8266WebServer*);
    
    // Extra calibration-related functions for this sensor.
    void getCalibrationHtml(ESP8266WebServer*);
    void getCalibration(ESP8266WebServer*);
    void doCalibration(ESP8266WebServer*);
    void deleteCalibration(ESP8266WebServer*);
    void enableCalibration(ESP8266WebServer*);
    void doCalibrationAction(ESP8266WebServer*);

  private:
  
    // Variables and functions related to the reading of the sensor.
    uint32_t startCycle;              // Cycle number at the start of a sampling.
    static void capDischarged(void);  // The interrupt handler. Must be made static as this way it can be attached to the interrupt.
    
    // Variables and functions related to the calibration functions.
    uint32_t takeReading();           // Measures the EC value, takes the water temperature as input, returns the result.
    float calibratedSlope;            // The calculated slope of the calibration curve.
    float calibratedIntercept;        // The calculated intercept of the calibration curve.
    void readCalibration();           // Read the current calibration parameters from EEPROM.
    void temperatureCorrection(uint32_t*);
    uint32_t lastWarned;
    void saveCalibrationData();
    Datapoint calibrationData[DATAPOINTS];
    float originalEC;

    // Other.
    HydroMonitorCore core;            // Provides some utility functions.
    Settings settings;                // The settings store.
    HydroMonitorCore::SensorData *sensorData;
    HydroMonitorLogging *logging;
};

#endif

