/*
 * HydroMonitorGrowlight.h
 * Library to manage the growing light.
 *
 * (C) Wouter van Marle / City Hydroponics
 * www.cityhydroponics.hk
 */
 

// ensure this library description is only included once
#ifndef GROWLIGHT_h
#define GROWLIGHT_h

#include <Arduino.h> // Needed for the String type.

// library interface description
class HydroMonitorGrowlight
{
  // user-accessible "public" interface
  public:
    
    struct Settings {
      unsigned int SwitchBrightness;
      unsigned int SwitchDelay;
      unsigned char OnHour;
      unsigned char OnMinute;
      unsigned char OffHour;
      unsigned char OffMinute;
      bool DaylightAutomatic;
    };

    HydroMonitorGrowlight(void);    // Constructor.
    void begin(Settings, unsigned char);
    void setSettings(Settings);
    void checkGrowlight(unsigned int);       // Switches the growlight on/off based on given lux value, 
                                    // taking time delay and on/off hours into account.
    void on(void);                  // Switches the growlight on, regardless of lux level or time of day. Disables automatic control.
    void off(void);                 // Switches the growlight off, regardless of lux level or time of day.  Disables automatic control.
    void automatic(void);
    bool getStatus(void);
    String settingsHtml(void);

  // library-accessible "private" interface
  private:
    unsigned char growlightPin;               // The GPIO pin the growlight is connected to.
    unsigned long lowlux;           // The time (in millis()) since the brightness fell below switchBrightness.
    unsigned long highlux;          // The time (in millis()) since the brightness rose above switchBrightness.
    bool manualMode;                // Sets manual mode for the light controls.
    bool startup;
    Settings settings;
};

#endif

