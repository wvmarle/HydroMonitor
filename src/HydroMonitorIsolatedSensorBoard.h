/*
   HydroMonitorECSensor.h
   Library for the EC probe.

   (C) Wouter van Marle / City Hydroponics
   www.cityhydroponics.hk

   This library controls a simple but very effective EC probe.

   Return values:
   - the EC, a positive number (in normal operation - normally in mS/cm, depending on the calibration values).
   - -1 (no probe detected).
*/

#ifndef HYDROMONITOR_ISOLATED_SENSOR_BOARD_H
#define HYDROMONITOR_ISOLATED_SENSOR_BOARD_H

#include <HydroMonitorCore.h>
#include <HydroMonitorLogging.h>
#include <HydroMonitorSensorBase.h>
#include <SoftwareSerial.h>

enum ReadingState {
  READING_IDLE,
  READING_VALUE,
  READING_TEMPERATURE,
  READING_EC,
  READING_PH
};

class HydroMonitorIsolatedSensorBoard: public HydroMonitorSensorBase
{
  public:
    struct Settings {
    };

    HydroMonitorIsolatedSensorBoard();                      // The constructor.

    // The functions required for all sensors.
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, SoftwareSerial*);
    void readSensor(bool readNow = false);                  // Measures the EC value, takes the water temperature as input, returns the result.
    void dataHtml(ESP8266WebServer*);                       // Provides html code with the sensor data.
    void settingsHtml(ESP8266WebServer*);                   // Provides html code with the settings.
    bool settingsJSON(ESP8266WebServer*);                   // Provides JSON code with the settings.
    void updateSettings(ESP8266WebServer*);

  private:

    // Variables and functions related to the reading of the sensor.

    // Other.
    HydroMonitorCore core;                                  // Provides some utility functions.
    Settings settings;                                      // The settings store.
    HydroMonitorCore::SensorData *sensorData;
    HydroMonitorLogging *logging;
    SoftwareSerial *sensorSerial;
    ReadingState readingState;
    char buffer[6];
    uint8_t count;
};

#endif

