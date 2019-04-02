#ifndef HYDROMONITOR_BOARD_131_h
#define HYDROMONITOR_BOARD_131_h

// Defaults for this individual board.
#define OTA_PASSWORD "esp"
#define MYSQL_USERNAME "board_131"             // MySQL user name. It's a string, so needs the quote marks.
#define MYSQL_PASSWORD "board_131_password"    // MySQL password. It's a string, so needs the quote marks.

#define LOGGING_USERNAME "board_131"             // MySQL user name. It's a string, so needs the quote marks.
#define LOGGING_PASSWORD "board_131_password"    // MySQL password. It's a string, so needs the quote marks.

// How to do the logging.
#define LOG_SERIAL  // Send log info to the Serial console.
#define LOG_MYSQL   // Send log info to the MySQL database.
#define USE_SERIAL

// Set the log level.
#define LOGLEVEL LOG_TRACE
//#define LOGLEVEL LOG_INFO
//#define LOGLEVEL LOG_WARNING
//#define LOGLEVEL LOG_ERROR
//#define LOGLEVEL LOG_OFF


#include <board_CH8.h>    // The defaults for the CH8 prototype.

// Special connection on pin 2 for this board.
#define USE_FLOW_SENSOR
#define FLOW_SENSOR_PIN 2

#endif
