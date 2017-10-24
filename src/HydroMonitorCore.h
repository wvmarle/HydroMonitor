/*
 * HydroMonitorCore
 *
 * This module does not do any real work but provides definitions to the other modules of HydroMonitor.
 *
 */

#ifndef HYDROMONITORCORE_H
#define HYDROMONITORCORE_H

#include <Arduino.h>
#include <EEPROM.h>
#include <Time.h>
#include <HydroMonitorBoardDefinitions.h> // Needed for the definitiosn to be available.

// The log functionality: various log levels.
#define LOG_TRACE 4               // everything - very noisy. 
#define LOG_DEBUG 3               // almost everything - noisy.
#define LOG_TESTING 2             // for testing the setup - includes actual readings
#define LOG_INFO 1                // normal info 
#define LOG_OFF 0                 // no logging (production mode)

// WiFi server settings.
#define CONNECT_TIMEOUT   30      // Seconds
#define CONNECT_OK        0       // Status of successful connection to WiFi
#define CONNECT_FAILED    (-99)   // Status of failed connection to WiFi
#define HTTPSPORT         443

// General update/refresh rates.
#define REFRESH_HTML 10           // Refresh rate of the html main page (10 seconds).
#define REFRESH_SENSORS 10000     // Refresh rate of the sensor readings (10 * 1000 milliseconds = 10 seconds).
#define REFRESH_NTP 3600000       // NTP update frequency (60 * 60 * 1000 milliseconds = 1 hour).
#define REFRESH_DATABASE 600000   // Database storage interval (10 * 60 * 1000 milliseconds = 10 minutes).
#define WARNING_INTERVAL 86400000 // Delay between system warnings (24 * 60 * 60 * 1000 milliseconds = 24 hours).

// Internal time keeping and NTP connectivity.
#define TIME_MSG_LEN 11           // time sync to PC is HEADER followed by Unix time_t as ten ASCII digits
#define TIME_HEADER 'T'           // Header tag for serial time sync message
#define TIME_REQUEST 7            // ASCII bell character requests a time sync message
#define NTPMAXTRIES 30
#define LOCAL_NTP_PORT 2390       // local port to listen for UDP packets
#define NTP_SERVER_NAME "time.nist.gov"
#define NTP_PACKET_SIZE 48        // NTP time stamp is in the first 48 bytes of the message

// EEPROM settings.
#define EEPROM_SIZE 4096          // Maximum size allowed on the ESP8266: one Flash sector.
#define EEPROM_SETTINGS 0         // Store the settings at byte 0.

// Maximum datapoints for calibration.
#define DATAPOINTS (uint8_t)10

// MySQL server defaults. N.B. thise values must have enclosing "" as they're strings.
#define MYSQL_HOSTNAME "cityhydroponics.hk" // MySQL server hostname.
#define MYSQL_DATA "data"         // Name of the database storing the sensor data.
#define MYSQL_LOG "messages"      // Name of the database storing the log messages.

// Datapoints for the sensor calibration.
// Each datapoint is 4+4+4 = 12 bytes.
struct Datapoint {
  uint32_t time;                  // timestamp (seconds since epoch) when the reading was taken. 
  float value;                    // Value at which the sensor was calibrated.
  uint32_t reading;               // The (temp, ... corrected) reading taken from the sensor at this value.
};

// Settings data is stored in the lower part of the EEPROM.
// Address 0 is for all sensors that do not have specific settings. These can be given space later.
// All sensors have 8 bytes 
#define EC_SENSOR_EEPROM 0
#define WATERTEMPERATURE_SENSOR_EEPROM 0
#define BRIGHTNESS_SENSOR_EEPROM 0
#define TEMPERATURE_SENSOR_EEPROM 0
#define HUMIDITY_SENSOR_EEPROM 0
#define PH_SENSOR_EEPROM 0
#define DO_SENSOR_EEPROM 0
#define ORP_SENSOR_EEPROM 0

// Byte 0 is simply skipped; starting at byte 1.
// Every sensor gets 16 bytes for future use; MySQL 100 bytes extra; network 50 bytes extra.
#define WATERLEVEL_SENSOR_EEPROM 1    // 4 bytes
#define PRESSURE_SENSOR_EEPROM 21     // 2 bytes
#define GROWLIGHT_EEPROM 39           // 9 bytes
#define FERTILISER_EEPROM 64          // 8 bytes
#define PHMINUS_EEPROM 96             // 4 bytes
#define RESERVOIR_EEPROM 126          // 2 bytes
#define MYSQL_EEPROM 144              // 164 bytes
#define NETWORK_EEPROM 408            // 17 bytes
#define OTA_PASSWORD_EEPROM 555       // 32 bytes
#define GROWING_PARAMETERS_EEPROM 587 // 80 bytes
// Next set of settings: 683.
    
// Calibration data is stored in the top part of the EEPROM.
#define EC_SENSOR_CALIBRATION_EEPROM EEPROM_SIZE - 1 * ((sizeof(Datapoint) + 2) * DATAPOINTS)   // Calibration data of EC sensor.
#define PH_SENSOR_CALIBRATION_EEPROM EEPROM_SIZE - 2 * ((sizeof(Datapoint) + 2) * DATAPOINTS)   // Calibration data of pH sensor.
#define DO_SENSOR_CALIBRATION_EEPROM EEPROM_SIZE - 3 * ((sizeof(Datapoint) + 2) * DATAPOINTS)
#define ORP_SENSOR_CALIBRATION_EEPROM EEPROM_SIZE - 4 * ((sizeof(Datapoint) + 2) * DATAPOINTS)
//#define _SENSOR_CALIBRATION EEPROM_SIZE - 5 * ((sizeof(Datapoint) + 2) * DATAPOINTS)
//#define _SENSOR_CALIBRATION EEPROM_SIZE - 6 * ((sizeof(Datapoint) + 2) * DATAPOINTS)
//#define _SENSOR_CALIBRATION EEPROM_SIZE - 7 * ((sizeof(Datapoint) + 2) * DATAPOINTS)
//#define _SENSOR_CALIBRATION EEPROM_SIZE - 8 * ((sizeof(Datapoint) + 2) * DATAPOINTS)

class HydroMonitorCore
{

  public:
    HydroMonitorCore(void);

    struct SensorData {
#ifdef USE_EC_SENSOR
      float EC;
#endif
#ifdef USE_BRIGHTNESS_SENSOR
      int32_t brightness;
#endif
#ifdef USE_WATERTEMPERATURE_SENSOR
      float waterTemp;
#endif
#ifdef USE_WATERLEVEL_SENSOR
      float waterLevel;
#endif
#ifdef USE_PRESSURE_SENSOR
      float pressure;
#endif
#ifdef USE_TEMPERATURE_SENSOR
      float temperature;
#endif
#ifdef USE_HUMIDITY_SENSOR
      float humidity;
#endif
#ifdef USE_PH_SENSOR
      float pH;
#endif
#ifdef USE_DO_SENSOR
      float DO;
#endif
#ifdef USE_ORP_SENSOR
      float ORP;
#endif
#ifdef USE_GROWLIGHT
      bool growlight;
#endif
    };
    
    // Various utility functions used by other modules.
    bool isNumeric(String);
    void leastSquares(float *, uint32_t *, uint8_t, float *, float *);
    void readCalibration(uint16_t, uint32_t *, float *, uint32_t *, bool *);
    void writeCalibration(uint16_t, uint32_t *, float *, uint32_t *, bool *);
    String calibrationHtml(char *, char *, uint32_t *, float *, uint32_t *, bool *);
    String calibrationData(uint32_t *, float *, uint32_t *, bool *);
    String datetime(time_t t);
    String datetime();
    
    struct CalibrationData {
      uint16_t enabled;
      Datapoint datapoint[DATAPOINTS];
    };

  private:
    
};


#endif
