/*
 * HydroMonitorCore
 *
 * This module does not do any real work but provides definitions to the other modules of HydroMonitor.
 *
 */

#ifndef HYDROMONITORCORE_H
#define HYDROMONITORCORE_H

#include <Arduino.h>
#include <ESP8266WiFi.h>

// The log functionality: various log levels.
#define TRACE 4     // everything - very noisy. 
#define DEBUG 3     // almost everything - noisy.
#define TESTING 2   // for testing the setup - includes actual readings
#define INFO 1      // normal info 
#define OFF 0       // no logging (production mode)

// WiFi server settings.
#define CONNECT_TIMEOUT   30      // Seconds
#define CONNECT_OK        0       // Status of successful connection to WiFi
#define CONNECT_FAILED    (-99)   // Status of failed connection to WiFi
#define HTTPSPORT         443

#define REFRESH 10                // Refresh rate (of the html page and the sensor readings) in seconds.

// Internal time keeping and NTP connectivity.
#define TIME_MSG_LEN 11           // time sync to PC is HEADER followed by Unix time_t as ten ASCII digits
#define TIME_HEADER 'T'           // Header tag for serial time sync message
#define TIME_REQUEST 7            // ASCII bell character requests a time sync message
#define NTPMAXTRIES 30
#define LOCAL_NTP_PORT 2390       // local port to listen for UDP packets
#define NTP_SERVER_NAME "time.nist.gov"
#define NTP_PACKET_SIZE 48        // NTP time stamp is in the first 48 bytes of the message

// The start address of the EEPROM.
#define EEPROM_ADDRESS 0

// The size allocation for EEPROM.
// Arduino: max 512 bytes, ESP8266: max 4096 bytes (one sector of flash memory).
// Must be equal or larger than the size the settings struct.
#define EEPROM_SIZE 1024


class HydroMonitorCore
{

  public:
    HydroMonitorCore(void);

    // The log functionality.
    void setLogging(void (*f)(String));
    void setLoglevel(int);
    int loglevel;
    typedef void (*LogFunction)(String);
    LogFunction writeLog;


  private:
    
};
#endif
