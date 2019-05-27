/*
 * HydroMonitorCore
 *
 * This module does not do any real work but provides definitions to the other modules of HydroMonitor.
 *
 */

#ifndef HYDROMONITORCORE_H
#define HYDROMONITORCORE_H

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
const uint8_t LOG_TRACE =     5;                            // detailed info
const uint8_t LOG_INFO =      4;                            // general system info (default)
const uint8_t LOG_WARNING =   3;                            // warnings
const uint8_t LOG_ERROR =     2;                            // errors
const uint8_t LOG_OFF =       1;                            // no logging
const uint16_t MAX_MESSAGE_SIZE = 500;                      // Maximum allowed message size.

// WiFi server settings.
const uint16_t CONNECT_TIMEOUT = 30;                        // Seconds
const uint16_t CONNECT_OK = 0;                              // Status of successful connection to WiFi
const int16_t CONNECT_FAILED = -99;                         // Status of failed connection to WiFi
const uint16_t HTTPSPORT = 443;

// General update/refresh rates.
const uint32_t REFRESH_HTML = 10;                           // Refresh rate of the html main page (10 seconds).
const uint32_t REFRESH_SENSORS = 10 * 1000ul;               // Refresh rate of the sensor readings (10 * 1000 milliseconds = 10 seconds).
const uint32_t REFRESH_NTP = 60 * 60 * 1000ul;              // NTP update frequency (60 * 60 * 1000 milliseconds = 1 hour).
const uint32_t REFRESH_DATABASE = 10 * 60 * 1000ul;         // Database storage interval (10 * 60 * 1000 milliseconds = 10 minutes).
const uint32_t WARNING_INTERVAL = 24 * 60 * 60 * 1000ul;    // Delay between system warnings (24 * 60 * 60 * 1000 milliseconds = 24 hours).

// Internal time keeping and NTP connectivity.
const uint8_t TIME_MSG_LEN = 11;                            // time sync to PC is HEADER followed by Unix time_t as ten ASCII digits
const char TIME_HEADER = 'T';                               // Header tag for serial time sync message
const uint8_t TIME_REQUEST = 7;                             // ASCII bell character requests a time sync message
const uint8_t NTPMAXTRIES = 30;
const uint16_t LOCAL_NTP_PORT = 2390;                       // local port to listen for UDP packets
const uint8_t NTP_PACKET_SIZE = 48;                         // NTP time stamp is in the first 48 bytes of the message
const char NTP_SERVER_NAME[] = "pool.ntp.org";

// EEPROM settings.
const uint16_t EEPROM_SIZE = 4096;                          // Maximum size allowed on the ESP8266: one Flash sector.

// Maximum datapoints for calibration.
const uint8_t DATAPOINTS = 10;

// Logging server defaults. N.B. these values must have enclosing "" as they're strings.
const char LOGGING_HOSTNAME[] = "www.cityhydroponics.hk";
const char LOGGING_HOSTPATH[] = "/HydroMonitor/postData.py/postData";

// Various system status flags - so other modules can check on these and act appropriately.
// The number is the bit postion in SensorData.status (0-31).
const uint8_t STATUS_WATERING               = 0;            // A tray is being watered.
const uint8_t STATUS_DRAINING_RESERVOIR     = 1;            // The reservoir is being drained.
const uint8_t STATUS_FILLING_RESERVOIR      = 2;            // Reservoir is being filled.
const uint8_t STATUS_DOOR_OPEN              = 3;            // The device's door is open.
const uint8_t STATUS_RESERVOIR_LEVEL_LOW    = 4;            // Low level in reservoir - not enough for watering.
const uint8_t STATUS_MAINTENANCE            = 5;            // Maintenance in progress - reservoir manually drained.
const uint8_t STATUS_DRAINAGE_NEEDED        = 6;            // Request immediate automatic drainage sequence of the reservoir.
#ifndef USE_WATERLEVEL_SENSOR
const uint8_t STATUS_RESERVOIR_DRAINED      = 7;            // Reservoir is empty, needs to be filled. Use this flag if not using water level sensor.
#endif
//const uint8_t STATUS_
//const uint8_t STATUS_

// Settings data is stored in the lower part of the EEPROM.
// Address 0 is for all sensors that do not have specific settings. These can be given space later.
// All sensors have 8 bytes 
const uint16_t EC_SENSOR_EEPROM = 0;
const uint16_t WATERTEMPERATURE_SENSOR_EEPROM = 0;
const uint16_t BRIGHTNESS_SENSOR_EEPROM = 0;
const uint16_t TEMPERATURE_SENSOR_EEPROM = 0;
const uint16_t HUMIDITY_SENSOR_EEPROM = 0;
const uint16_t PH_SENSOR_EEPROM = 0;
const uint16_t DO_SENSOR_EEPROM = 0;
const uint16_t ORP_SENSOR_EEPROM = 0;
const uint16_t NETWORK_EEPROM = 0;
const uint16_t FLOW_SENSOR_EEPROM = 0;
const uint16_t ISOLATED_SENSOR_BOARD_EEPROM = 0;

// Byte 0 is simply skipped; starting at byte 1.
const uint16_t WATERLEVEL_SENSOR_EEPROM = 1;                // 4 bytes
const uint16_t PRESSURE_SENSOR_EEPROM = 21;                 // 2 bytes
const uint16_t GROWLIGHT_EEPROM = 39;                       // 9 bytes
const uint16_t FERTILISER_EEPROM = 64;                      // 8 bytes
const uint16_t PHMINUS_EEPROM = 96;                         // 4 bytes
const uint16_t RESERVOIR_EEPROM = 126;                      // 2 bytes
const uint16_t MYSQL_EEPROM = 144;                          // 164 bytes
const uint16_t OTA_PASSWORD_EEPROM = 408;                   // 32 bytes
const uint16_t GROWING_PARAMETERS_EEPROM = 456;             // 84 bytes
const uint16_t LOGGING_EEPROM = 556;                        // 317 bytes
const uint16_t DRAINAGE_EEPROM = 889;                       // 4 bytes
const uint16_t FREE_EEPROM = 909;                           // Above this address it's free to use.

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
const uint16_t EC_SENSOR_CALIBRATION_EEPROM = EEPROM_SIZE - 1 * sizeof(Datapoint) * DATAPOINTS; // Calibration data of EC sensor.
const uint16_t PH_SENSOR_CALIBRATION_EEPROM = EEPROM_SIZE - 2 * sizeof(Datapoint) * DATAPOINTS; // Calibration data of pH sensor.
const uint16_t DO_SENSOR_CALIBRATION_EEPROM = EEPROM_SIZE - 3 * sizeof(Datapoint) * DATAPOINTS;
const uint16_t ORP_SENSOR_CALIBRATION_EEPROM = EEPROM_SIZE - 4 * sizeof(Datapoint) * DATAPOINTS;
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
