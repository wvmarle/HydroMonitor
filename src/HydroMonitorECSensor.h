/*
 * HydroMonitorECSensor.h
 * Library for the EC probe.
 *
 * (C) Wouter van Marle / City Hydroponics
 * www.cityhydroponics.hk
 *
 * This library controls a simple EC probe.
 *
 * Return values:
 * - the EC, a positive number (in normal operation - normally in mS/cm, depending on the calibration values).
 * - -1 (no probe detected).
 */
 

// ensure this library description is only included once
#ifndef EC_SENSOR_h
#define EC_SENSOR_h

#include <Arduino.h>              // Needed for the String type.

// library interface description
class HydroMonitorECSensor
{
  // user-accessible "public" interface
  public:
    struct Settings {
      double CalibratedSlope; 
      double CalibratedIntercept;
      unsigned char Samples;
    };

    HydroMonitorECSensor();       // The constructor.
    void begin(Settings, unsigned char, unsigned char, unsigned char);
    double readSensor(double);    // Measures the EC value, takes the water temperature as input, returns the result.
    void setCalibrationMode(bool);// Switches to calibration mode (return discharge time instead of EC value).
    void setSettings(Settings);
    String settingsHtml(void);    // Provides html code with the settings.

  // library-accessible "private" interface
  private:
  
    // Hardware parameters.
    unsigned char capPos;                   // Pin number of capPos of this probe.
    unsigned char capNeg;                   // Pin number of capNeg of this probe.
    unsigned char ECpin;                    // Pin number of ECpin of this probe.
    
    // Other.
    double waterTemp;
    unsigned long startCycle;     // Cycle number at the start of a sampling.
    unsigned long startTime;      // the time stamp (in microseconds) the measurement starts.
    volatile unsigned long endTime; // the time stamp (in microseconds) the measurement is finished.
    unsigned int dischargeTime;   // the time it took for the capacitor to discharge.
    unsigned int chargeDelay;     // The time (in microseconds) given to the cap to fully charge/discharge - at least 5x RC.
    unsigned int timeout;         // Discharge timeout in milliseconds - if not triggered within this time, the EC probe 
                                  // is probably not there.
    bool calibrationMode;         // Whether we're in calibration mode.
    Settings settings;
    
    // Must be made static as this way it can be attached to the interrupt.
    static void capDischarged(void); // The interrupt handler.
};

#endif

