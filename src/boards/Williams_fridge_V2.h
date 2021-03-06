#ifndef HYDROMONITOR_WILLIAMS_FRIDGE_V2_H
#define HYDROMONITOR_WILLIAMS_FRIDGE_V2_H

// Defaults for this individual board.
#define OTA_PASSWORD "esp"
//#define MYSQL_USERNAME "sql_username"             // MySQL user name. It's a string, so needs the quote marks.
//#define MYSQL_PASSWORD "sql_password"             // MySQL password. It's a string, so needs the quote marks.

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

#define USE_EC_SENSOR                                       // Sensor #1
#define USE_PH_SENSOR                                       // #2
#define USE_WATERTEMPERATURE_SENSOR                         // #3
#define USE_DS18B20
#define USE_ISOLATED_SENSOR_BOARD                           // #4
//#define USE_WATERLEVEL_SENSOR                               // #5

#define N_SENSORS 4

#define USE_FERTILISER
#define USE_PHMINUS
#define USE_RESERVOIR
#define USE_DRAINAGE

#define LOG_MYSQL

#define AUX1_MCP17_PIN          0
#define LEVEL_LIMIT_MCP17_PIN   1
#define DRAINAGE_MCP17_PIN      2
#define WATER_INLET_MCP17_PIN     3
#define PHMINUS_MCP17_PIN       4
#define FERTILISER_B_MCP17_PIN  5
#define FERTILISER_A_MCP17_PIN  6
//#define AUX2_MCP17_PIN          7

// Direct pin connections.
//#define PH_SENSOR_PIN A0
//#define CAPNEG_PIN 0
//#define CAPPOS_PIN 13
//#define EC_PIN 12

//#define USE_HCSR04
//#define TRIG_PIN 15
//#define ECHO_PIN 16

#define USE_MPXV5004
#define MPXV5004_PIN A0

//#define USE_FLOATSWITCHES
//#define FLOATSWITCH_HIGH_MCP17_PIN    0   // AUX1 connector.
//#define FLOATSWITCH_MEDIUM_MCP17_PIN  7   // AUX2 connector.
//#define FLOATSWITCH_LOW_PIN           14  // DS18B20 connector.

#define USE_24LC256_EEPROM

#define IS_FRIDGE_CONTROL                                   // Some library functions/settings may be unique to the fridge control.

#endif
