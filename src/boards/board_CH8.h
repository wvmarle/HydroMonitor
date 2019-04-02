#ifndef HYDROMONITOR_BOARD_CH8_h
#define HYDROMONITOR_BOARD_CH8_h

/*
 ***************************************************************************************
 */
// The CH8 boards with "water sensors" board.
// This is a generic set of connections related to this PCB.
// Note: connection on pin 2 is to be defined separately in the individual board definition.
#define USE_GROWLIGHT
#define GROWLIGHT_PIN 15

#define USE_EC_SENSOR
#define CAPPOS_PIN 12      // CapPos/C+ for EC sensor.
#define CAPNEG_PIN 14      // CapNeg/C- for EC sensor.
#define EC_PIN 16          // ECpin/EC for EC sensor.

#define USE_BRIGHTNESS_SENSOR
#define USE_TSL2591

#define USE_TEMPERATURE_SENSOR
#define USE_PRESSURE_SENSOR
#define USE_BMP280

#define USE_WATERLEVEL_SENSOR
#define USE_WATERTEMPERATURE_SENSOR
#define USE_MS5837

#define USE_I2C

#define USE_WIFILED
#define WIFILED_PIN 13

#define USE_SERIAL

//#define USE_HUMIDITY_SENSOR
//#define USE_TEMPERATURE_SENSOR
//#define DHT22_PIN 2

#endif
