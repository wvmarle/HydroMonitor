#ifndef HYDROMONITOR_BOARD_128_h
#define HYDROMONITOR_BOARD_128_h

// The first HydroMonitorCH8 prototype.
// Reserverd IP address: 192.168.1.128.

// Defaults for this individual board.
#define OTA_PASSWORD "esp"
#define MYSQL_USERNAME "board_128"             // MySQL user name. It's a string, so needs the quote marks.
#define MYSQL_PASSWORD "board_128_password"    // MySQL password. It's a string, so needs the quote marks.

#define LOGGING_USERNAME "board_128"             // MySQL user name. It's a string, so needs the quote marks.
#define LOGGING_PASSWORD "board_128_password"    // MySQL password. It's a string, so needs the quote marks.

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
#define USE_WIFILED
#define WIFILED_PIN 16                // Indicator LED - on when WiFi is connected.

#define USE_WATERLEVEL_SENSOR
#define USE_HCSR04                // Use the HC-SR04 range finder.
#define ECHO_PIN 3                // HC-SR04 range finder - echo pin. RX when Serial is enabled.
#define TRIG_PIN 1                // HC-SR04 range finder - trig pin. TX when Serial is enabled.

#define USE_EC_SENSOR
#define CAPPOS_PIN 14             // CapPos/C+ for EC sensor.
#define CAPNEG_PIN 12             // CapNeg/C- for EC sensor.
#define EC_PIN 13                 // ECpin/EC for EC sensor.

#define USE_GROWLIGHT
#define GROWLIGHT_PIN 15          // 5V power switch for the growlight.

#define USE_WATERTEMPERATURE_SENSOR
#define USE_NTC
#define NTC_PIN 0                 // The thermistor - analog 0.

// NTC parameters.
#define THERMISTORNOMINAL 10000   // Nominal temperature value for the thermistor
#define TEMPERATURENOMINAL 25     // Nominal temperature depicted on the datasheet
#define BCOEFFICIENT 3950         // Beta value for our thermistor
#define NTCSERIESRESISTOR 100000  // Value of the series resistor
#define ADCMAX 3380               // 1024*3.3/1.0 (the ADC measures only 0-1V, this is the range as if it's doing 0-3.3V).

#endif

