#ifndef HYDROMONITOR_BOARD_126_h
#define HYDROMONITOR_BOARD_126_h

// Second Hydromonitor Pro prototype - with SD card, water_sensors, 4x 12V outputs, 1x 5V output, MCP23008 port extender.
// Reserverd IP address: 192.168.1.126.

// Defaults for this individual board.
#define OTA_PASSWORD "esp"
#define MYSQL_USERNAME "board_126"             // MySQL user name. It's a string, so needs the quote marks.
#define MYSQL_PASSWORD "board_126_password"    // MySQL password. It's a string, so needs the quote marks.

#define LOGGING_USERNAME "board_126"             // MySQL user name. It's a string, so needs the quote marks.
#define LOGGING_PASSWORD "board_126_password"    // MySQL password. It's a string, so needs the quote marks.


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

// Connected directly to the ESP8266.
#define USE_EC_SENSOR
#define CAPPOS_PIN 0
#define CAPNEG_PIN 2
#define EC_PIN 16
#define USE_PH_SENSOR
#define PH_SENSOR_PIN A0
#define USE_I2C

// Connected to the MCP23008.
#define USE_MCP23008
#define OUT_12V_1_PIN 0
#define OUT_12V_2_PIN 1
#define OUT_12V_3_PIN 2
#define OUT_12V_4_PIN 3
#define OUT_5V_1_PIN 4
#define USE_WIFILED
#define WIFILED_MCP_PIN 5
#define PH_POS_MCP_PIN 6
#define PH_GND_MCP_PIN 7

// Other hardware.
#define USE_PRESSURE_SENSOR
#define USE_TEMPERATURE_SENSOR
#define USE_BMP180

#define USE_WATERLEVEL_SENSOR
#define USE_WATERTEMPERATURE_SENSOR
#define USE_MS5837

#define USE_MICROSD

#define USE_BRIGHTNESS_SENSOR
#define USE_TSL2561

#endif
