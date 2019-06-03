/*
   HydroMonitorGrowlight.h
   Library to manage the growing light.

   (C) Wouter van Marle / City Hydroponics
   www.cityhydroponics.hk
*/

#ifndef GROWLIGHT_h
#define GROWLIGHT_h

#include <HydroMonitorCore.h>
#ifdef GROWLIGHT_PCF_PIN
#include <pcf8574_esp.h>
#elif defined(GROWLIGHT_MCP_PIN)
#include <Adafruit_MCP23008.h>
#endif
#include <TimeLib.h>
#include <HydroMonitorLogging.h>

class HydroMonitorGrowlight
{
  public:

    struct Settings {
      uint16_t switchBrightness;
      uint16_t switchDelay;
      uint8_t onHour;
      uint8_t onMinute;
      uint8_t offHour;
      uint8_t offMinute;
      bool daylightAutomatic;
    };

    // The constructor.
    HydroMonitorGrowlight(void);

    // General functions for this module.
#ifdef GROWLIGHT_PIN
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*);
#elif defined(GROWLIGHT_PCF_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, PCF857x*);
#elif defined(GROWLIGHT_MCP_PIN)
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, Adafruit_MCP23008*);
#endif
    void checkGrowlight();                                  // Switches the growlight on/off based on given lux value, taking time delay and on/off hours into account.
    void on(void);                                          // Switches the growlight on, regardless of lux level or time of day. Disables automatic control.
    void off(void);                                         // Switches the growlight off, regardless of lux level or time of day.  Disables automatic control.
    void automatic(void);                                   // Set grow light to automatic control.
    bool settingsJSON(ESP8266WebServer *);                  // Provides json code with the sensor data.
    void settingsHtml(ESP8266WebServer *);                  // Provides html code with the sensor data.
    void updateSettings(String[], String[], uint8_t);

  private:

    // Hardware parameters.
#ifdef GROWLIGHT_PCF_PIN)
    PCF857x *pcf8574;
#elif defined(GROWLIGHT_MCP_PIN)
    Adafruit_MCP23008 *mcp23008;
#endif

    // Timing related variables.
    uint32_t lowlux;                                        // The time (in millis()) since the brightness fell below switchBrightness.
    uint32_t highlux;                                       // The time (in millis()) since the brightness rose above switchBrightness.
    bool manualMode;                                        // Sets manual mode for the light controls.

    // Utility functions.
    void switchLightOn(void);
    void switchLightOff(void);
    HydroMonitorCore core;
    Settings settings;
    HydroMonitorCore::SensorData *sensorData;
    HydroMonitorLogging *logging;
};

#endif

