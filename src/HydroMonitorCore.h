/*
 * HydroMonitorCore
 *
 * This module does not do any real work but provides definitions to the other modules of HydroMonitor.
 *
 */

#ifndef HYDROMONITORCORE_H
#define HYDROMONITORCORE_H

#include <Arduino.h>
#include <TimeLib.h>
#include <boards/HydroMonitorBoardDefinitions.h>            // The detailed definitions of what sensors and pins we have defined.
#include <ESP8266WebServer.h>

#ifdef USE_24LC256_EEPROM
#include <24LC256.h>
#else
#include <EEPROM.h>
#endif

// The log functionality: various log levels.
// Don't use 0 as value - that messes up the end-of-message detection in the log file (null terminated).
#define LOG_TRACE 5                                         // detailed info
#define LOG_INFO 4                                          // general system info (default)
#define LOG_WARNING 3                                       // warnings
#define LOG_ERROR 2                                         // errors
#define LOG_OFF 1                                           // no logging
#define MAX_MESSAGE_SIZE 500                                // Maximum allowed message size.

// WiFi server settings.
#define CONNECT_TIMEOUT   30                                // Seconds
#define CONNECT_OK        0                                 // Status of successful connection to WiFi
#define CONNECT_FAILED    (-99)                             // Status of failed connection to WiFi
#define HTTPSPORT         443

// General update/refresh rates.
#define REFRESH_HTML 10                                     // Refresh rate of the html main page (10 seconds).
#define REFRESH_SENSORS 10000                               // Refresh rate of the sensor readings (10 * 1000 milliseconds = 10 seconds).
#define REFRESH_NTP 3600000                                 // NTP update frequency (60 * 60 * 1000 milliseconds = 1 hour).
#define REFRESH_DATABASE 600000                             // Database storage interval (10 * 60 * 1000 milliseconds = 10 minutes).
#define WARNING_INTERVAL 86400000                           // Delay between system warnings (24 * 60 * 60 * 1000 milliseconds = 24 hours).

// Internal time keeping and NTP connectivity.
#define TIME_MSG_LEN 11                                     // time sync to PC is HEADER followed by Unix time_t as ten ASCII digits
#define TIME_HEADER 'T'                                     // Header tag for serial time sync message
#define TIME_REQUEST 7                                      // ASCII bell character requests a time sync message
#define NTPMAXTRIES 30
#define LOCAL_NTP_PORT 2390                                 // local port to listen for UDP packets
#define NTP_SERVER_NAME "pool.ntp.org"
#define NTP_PACKET_SIZE 48                                  // NTP time stamp is in the first 48 bytes of the message

// EEPROM settings.
#define EEPROM_SIZE 4096                                    // Maximum size allowed on the ESP8266: one Flash sector.

// Maximum datapoints for calibration.
#define DATAPOINTS (uint8_t)10

// MySQL server defaults. N.B. thise values must have enclosing "" as they're strings.
#define MYSQL_HOSTNAME "cityhydroponics.hk"                 // MySQL server hostname.
#define MYSQL_DATA "data"                                   // Name of the database storing the sensor data.
#define MYSQL_LOG "messages"                                // Name of the database storing the log messages.


#define LOGGING_HOSTNAME "www.cityhydroponics.hk"
#define LOGGING_HOSTPATH "/HydroMonitor/postData.py/postData"


// Various system status flags - so other modules can check on these and act appropriately.
// The number is the bit postion in SensorData.status (0-31).
const uint8_t STATUS_WATERING               = 0;            // A tray is being watered.
const uint8_t STATUS_DRAINING_RESERVOIR     = 1;            // The reservoir is being drained.
const uint8_t STATUS_FILLING_RESERVOIR      = 2;            // Reservoir is being filled.
const uint8_t STATUS_DOOR_OPEN              = 3;            // The device's door is open.
const uint8_t STATUS_RESERVOIR_LEVEL_LOW    = 4;            // Low level in reservoir - not enough for watering.
const uint8_t STATUS_MAINTENANCE            = 5;            // Maintenance in progress - reservoir manually drained.
const uint8_t STATUS_DRAINAGE_NEEDED        = 6;            // Request immediate automatic drainage sequence of the reservoir.
//const uint8_t STATUS_
//const uint8_t STATUS_
//const uint8_t STATUS_


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
#define NETWORK_EEPROM 0
#define FLOW_SENSOR_EEPROM 0
#define ISOLATED_SENSOR_BOARD_EEPROM 0

// Byte 0 is simply skipped; starting at byte 1.
#define WATERLEVEL_SENSOR_EEPROM 1                          // 4 bytes
#define PRESSURE_SENSOR_EEPROM 21                           // 2 bytes
#define GROWLIGHT_EEPROM 39                                 // 9 bytes
#define FERTILISER_EEPROM 64                                // 8 bytes
#define PHMINUS_EEPROM 96                                   // 4 bytes
#define RESERVOIR_EEPROM 126                                // 2 bytes
#define MYSQL_EEPROM 144                                    // 164 bytes
#define OTA_PASSWORD_EEPROM 408                             // 32 bytes
#define GROWING_PARAMETERS_EEPROM 456                       // 84 bytes
#define LOGGING_EEPROM 556                                  // 317 bytes
#define DRAINAGE_EEPROM 889                                 // 4 bytes
#define FREE_EEPROM 909                                     // Above this address it's free to use.

// Datapoints for the sensor calibration.
// Each datapoint is 4+4+4+4+1 = 17 bytes.
struct Datapoint {
  uint32_t timestamp;                                       // timestamp (seconds since epoch) when the reading was taken. 
  float value;                                              // Temperature corrected value at which the sensor was calibrated.
  float nominalValue;                                       // Nominal value of the parameter at which the sensor was calibrated.
  uint32_t reading;                                         // The (temp, ... corrected) reading taken from the sensor at this value.
  bool enabled;                                             // Whether this datapoint is enabled or not.
};

// Calibration data is stored in the top part of the EEPROM.
#define EC_SENSOR_CALIBRATION_EEPROM EEPROM_SIZE - 1 * sizeof(Datapoint) * DATAPOINTS   // Calibration data of EC sensor.
#define PH_SENSOR_CALIBRATION_EEPROM EEPROM_SIZE - 2 * sizeof(Datapoint) * DATAPOINTS   // Calibration data of pH sensor.
#define DO_SENSOR_CALIBRATION_EEPROM EEPROM_SIZE - 3 * sizeof(Datapoint) * DATAPOINTS
#define ORP_SENSOR_CALIBRATION_EEPROM EEPROM_SIZE - 4 * sizeof(Datapoint) * DATAPOINTS
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
      uint16_t fertiliserConcentration;
      float targetEC;
#endif
#ifdef USE_BRIGHTNESS_SENSOR
      int32_t brightness;
#endif
#if defined(USE_WATERTEMPERATURE_SENSOR) || defined(USE_ISOLATED_SENSOR_BOARD)
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
      float pHMinusConcentration;
      float targetpH;
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
#if defined(USE_EC_SENSOR) || defined(USE_PH_SENSOR)
      uint16_t solutionVolume;
#endif
#ifdef USE_FLOW_SENSOR
      float flow;
#endif
#ifdef USE_ISOLATED_SENSOR_BOARD
      uint16_t ecReading;
      uint16_t phReading;
#endif
#ifdef USE_24LC256_EEPROM
      E24LC256* EEPROM;
#endif
      char systemName[65];
      float timezone;
      uint32_t systemStatus;
    };
    
    void begin(SensorData*);
    
    // Various utility functions used by other modules.
    bool isNumeric(String);
    void leastSquares(float *, uint32_t *, uint8_t, float *, float *);
    void calibrationHtml(ESP8266WebServer *, char *, char *, Datapoint *);
    void calibrationData(ESP8266WebServer *, Datapoint *);
    void datetime(char*, time_t t);
    void datetime(char*);

    String urlencode(String);
    String urldecode(String);
    
  private:
    SensorData* sensorData;
    unsigned char h2int(char);
};


#endif
