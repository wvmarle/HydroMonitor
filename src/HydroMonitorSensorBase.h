/*
   HydroMonitorSensorBase.h

   (C) Wouter van Marle / City Hydroponics
   www.cityhydroponics.hk
*/

#ifndef HYDROMONITORSENSORBASE_h
#define HYDROMONITORSENSORBASE_h

class HydroMonitorSensorBase
{
  public:
    virtual void readSensor(bool readNow = false);          // Take a sensor reading: once every REFRESH_SENSORS ms, or immediately if readNow = true.
    virtual void dataHtml(ESP8266WebServer*);               // Return the sensor data as html: one or more table rows.
    virtual void settingsHtml(ESP8266WebServer*);           // Return the sensor settings as html: one or more table rows.
    virtual bool settingsJSON(ESP8266WebServer*);           // return true if have anything, false if empty (needed for proper JSON formatting).
    virtual void updateSettings(ESP8266WebServer*);         // Update the sensor's settings, using the argName() and arg() values.
};

#endif
