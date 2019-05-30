#ifndef HYDROMONITORFLOWSENSOR_H
#define HYDROMONITORFLOWSENSOR_H

#include <HydroMonitorCore.h>
#include <ESP8266WebServer.h>
#include <HydroMonitorLogging.h>
#include <HydroMonitorSensorBase.h>

class HydroMonitorFlowSensor: public HydroMonitorSensorBase
{
  public:

    struct Settings {
    };

    HydroMonitorFlowSensor(void);

    // Functions as required for all sensors.
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging *logging);
    void readSensor(void);
    String dataHtml(void);            // Provides html code with the sensor data.
    String settingsHtml(void);
    void updateSettings(String[], String[], uint8_t);

  private:
    Settings settings;
    HydroMonitorCore core;
    HydroMonitorCore::SensorData *sensorData;
    HydroMonitorLogging *logging;
    uint32_t timeStartCounting;
    static void countPulse(void);
};

#endif
