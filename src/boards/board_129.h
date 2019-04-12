#ifndef HYDROMONITOR_BOARD_129_h
#define HYDROMONITOR_BOARD_129_h

// First Hydromonitor Pro prototype - with SD card, DHT22, 4x 12V outputs, PCF8574 port extender.
// Reserverd IP address: 192.168.1.129.

// Defaults for this individual board.
#define OTA_PASSWORD "esp"
#define MYSQL_USERNAME "board_129"             // MySQL user name. It's a string, so needs the quote marks.
#define MYSQL_PASSWORD "board_129_password"    // MySQL password. It's a string, so needs the quote marks.

#define LOGGING_USERNAME "board_129"             // MySQL user name. It's a string, so needs the quote marks.
#define LOGGING_PASSWORD "board_129_password"    // MySQL password. It's a string, so needs the quote marks.

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

// Connecteded direct to the ESP8266.
#define USE_WATERLEVEL_SENSOR
#define USE_HCSR04
#define ECHO_PIN 16        // HC-SR04 range finder - echo pin.

//#define USE_EC_SENSOR
#define CAPPOS_PIN 0       // CapPos/C+ for EC sensor.
#define CAPNEG_PIN 1       // CapNeg/C- for EC sensor.
#define EC_PIN 3           // ECpin/EC for EC sensor.
//#define USE_FERTILISER

#define USE_TEMPERATURE_SENSOR
#define USE_HUMIDITY_SENSOR
#define DHT22_PIN 2           // DHT22 humidity and temperature sensor.

#define USE_BRIGHTNESS_SENSOR
#define USE_TSL2561

#define USE_PRESSURE_SENSOR
#define USE_BMP180

#define USE_I2C

// Connected to the PCF8574.
#define USE_PCF8574
/*
  P4 = connector 4
  P5 = connector 3
  P6 = connector 2
  P7 = connector 1
*/

#define TRIG_PCF_PIN 0         // HC-SR04 range finder - trig pin.
// P1 and P2 are not connected.
#define USE_WIFILED
#define WIFILED_PCF_PIN 3         // Indicator LED - on when WiFi is connected.
#define FERTILISER_B_PCF_PIN 5    // peristaltic pump for fertiliser.
#define FERTILISER_A_PCF_PIN 6    // peristaltic pump for fertiliser.
#define USE_PHMINUS
#define PHMINUS_PCF_PIN 4        // peristaltic pump for pH-minus.
//#define USE_GROWLIGHT
//#define GROWLIGHT_PCF_PIN 7       // Growing light.

// Connected to the ADS1115.
#define USE_ADS1115
// A0 is not connected.
#define USE_WATERTEMPERATURE_SENSOR
#define USE_NTC
#define NTC_ADS_PIN 1         // The thermistor for water temperature.
//#define USE_DO_SENSOR
//#define DO_SENSOR_ADS_PIN 2   // Dissolved oxygen sensor.

// pH sensor
#define USE_PH_SENSOR
#define PH_SENSOR_ADS_PIN 3   // pH sensor

// NTC parameters. All as floats to force floating point maths.
#define THERMISTORNOMINAL 10000.0// Nominal temperature value for the thermistor
#define TEMPERATURENOMINAL 25.0  // Nominal temperature depicted on the datasheet
#define BCOEFFICIENT 3950.0      // Beta value for our thermistor
#define NTCSERIESRESISTOR 10000.0// Value of the series resistor
#define ADCMAX 26400.0           // 32768*3.3/4.096

#endif
