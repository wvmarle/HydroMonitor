#ifndef HYDROMONITOR_3A_h
#define HYDROMONITOR_3A_h

#define LOGGING_USERNAME "sql_username"           // MySQL user name. It's a string, so needs the quote marks.
#define LOGGING_PASSWORD "sql_password"           // MySQL password. It's a string, so needs the quote marks.

// How to do the logging.
#define LOG_SERIAL  // Send log info to the Serial console.
#define LOG_MYSQL   // Send log info to the MySQL database.

// Set the log level.
//#define LOGLEVEL LOG_TRACE
#define LOGLEVEL LOG_INFO
//#define LOGLEVEL LOG_WARNING
//#define LOGLEVEL LOG_ERROR
//#define LOGLEVEL LOG_OFF


//FIXME: the beeper.
#define AUX1_MCP17_PIN 10


/*
 ***************************************************************************************
 Hydromonitor v.3a
 
 Available connections:
 - fertiliser & pH- dosage
 - growlight
 - ventilation
 - reservoir drainage
 - reservoir water inlet
 - float switch
 - air bubbler (for use with pressure tube water level sensor)
 - EC, pH sensor (isolated board - +5, GND, Vccio, Tx)
 - WiFi LED
 - LCD display (I2C / 5V)
 - UV sensor (I2C / 3.3V)
 - Brightness sensor (I2C / 3.3V) 
 - Temp/humidity sensor (I2C / 3.3V)
 - Water level sensor (pressure tube)
 - OUT1, OUT2, OUT4 (spare switched 12V outputs)
 - AUX1, AUX2, AUX3, AUX4 (spare 5V I/O connections)
 - I2C 5V spare.
 - I2C 3.3V spare.
 */

#define USE_MCP23017
#define MCP_23017_ENABLE_PIN 14

#define USE_FERTILISER
#define FERTILISER_B_MCP17_PIN 7
#define FERTILISER_A_MCP17_PIN 6

#define USE_PHMINUS
#define PHMINUS_MCP17_PIN 5

#define USE_GROWLIGHT
#define GROWLIGHT_MCP17_PIN 4

#define USE_VENTILATOR
#define VENTILATOR_MCP17_PIN 3

#define USE_DRAINAGE
#define DRAINAGE_MCP17_PIN 2

#define USE_RESERVOIR
#define WATER_INLET_MCP17_PIN 0
#define LEVEL_LIMIT_MCP17_PIN 1

/*
#define USE_WATERLEVEL_SENSOR
#define USE_MPXV5004
#define MPXV5004_PIN A0
#define USE_BUBBLER
#define BUBBLER_MCP17_PIN 12
*/

#define USE_EC_SENSOR
#define USE_PH_SENSOR
#define USE_WATERTEMPERATURE_SENSOR
#define USE_ISOLATED_SENSOR_BOARD

#define N_SENSORS 8

#define ISOLATED_SENSOR_BOARD_RX_PIN 12

#define USE_WIFILED
#define WIFILED_PIN 13

#define USE_BRIGHTNESS_SENSOR
#define USE_TSL2591

#define USE_TEMPERATURE_SENSOR
#define USE_PRESSURE_SENSOR
#define USE_HUMIDITY_SENSOR
#define USE_BME280

#define USE_I2C

#define USE_SERIAL

#endif
