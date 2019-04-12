#ifndef HYDROMONITOR_WILLIAMS_FRIDGE_V1_H
#define HYDROMONITOR_WILLIAMS_FRIDGE_V1_H

// Defaults for this individual board.
#define OTA_PASSWORD "esp"
#define MYSQL_USERNAME "sql_username"             // MySQL user name. It's a string, so needs the quote marks.
#define MYSQL_PASSWORD "sql_password"             // MySQL password. It's a string, so needs the quote marks.

#define LOGGING_USERNAME "sql_username"           // MySQL user name. It's a string, so needs the quote marks.
#define LOGGING_PASSWORD "sql_password"           // MySQL password. It's a string, so needs the quote marks.

// How to do the logging.
#define LOG_SERIAL  // Send log info to the Serial console.
#define LOG_MYSQL   // Send log info to the MySQL database.

// Set the log level.
#define LOGLEVEL LOG_TRACE
//#define LOGLEVEL LOG_INFO
//#define LOGLEVEL LOG_WARNING
//#define LOGLEVEL LOG_ERROR
//#define LOGLEVEL LOG_OFF

#define USE_EC_SENSOR
#define USE_PH_SENSOR
#define USE_WATERLEVEL_SENSOR
//#define USE_DS1603L
#define USE_HCSR04
#define TRIG_PIN 15
#define ECHO_PIN 16
#define USE_WATERTEMPERATURE_SENSOR
#define USE_DS18B20
#define USE_FERTILISER
#define USE_PHMINUS
#define USE_RESERVOIR
#define USE_DRAINAGE_PUMP

#define LOG_MYSQL

#define FERTILISER_A_MCP17_PIN  1
#define FERTILISER_B_MCP17_PIN  0
#define PHMINUS_MCP17_PIN       2
#define DRAINAGE_MCP17_PIN      3
//#define DOOR_MCP17_PIN          4
//#define LEVEL_LIMIT_MCP17_PIN   5
//#define WIFILED_MCP17_PIN       6
#define RESERVOIR_MCP17_PIN     7

// Direct pin connections.
#define PH_SENSOR_PIN A0
#define CAPNEG_PIN 0
#define CAPPOS_PIN 13
#define EC_PIN 12

#endif
