#ifndef HYDROMONITOR_TEST_EVERYTHING_1_h
#define HYDROMONITOR_TEST_EVERYTHING_1_h

/*
 ***************************************************************************************
 */
// This one is for compilation testing only!
//
// Between the four of them they enable every single available hardware option, which is of course unrealistic but makes sure everything compiles properly.
#ifdef EVERYTHING_1

#define USE_WIFILED
#define WIFILED_PIN 0

#define USE_WATERLEVEL_SENSOR
#define USE_HCSR04
#define ECHO_PIN 0
#define TRIG_PIN 1

#define USE_EC_SENSOR
#define CAPPOS_PIN 0
#define CAPNEG_PIN 1
#define EC_PIN 2

#define USE_GROWLIGHT
#define GROWLIGHT_PIN 0

#define USE_WATERTEMPERATURE_SENSOR
#define USE_NTC
#define NTC_PIN 0
#define THERMISTORNOMINAL 10000
#define TEMPERATURENOMINAL 25
#define BCOEFFICIENT 3950
#define NTCSERIESRESISTOR 100000
#define ADCMAX 3388

#define USE_TEMPERATURE_SENSOR
#define USE_HUMIDITY_SENSOR
#define DHT22_PIN 0

#define USE_FERTILISER
#define FERTILISER_B_PIN 0
#define FERTILISER_A_PIN 1

#define USE_PHMINUS
#define PHMINUS_PIN 0

#define USE_PH_SENSOR
#define PH_SENSOR_PIN 0

#define USE_DO_SENSOR
#define DO_SENSOR_PIN 0

#define USE_ORP_SENSOR
#define ORP_SENSOR_PIN 0

#define USE_TEMPERATURE_SENSOR
#define USE_PRESSURE_SENSOR
#define USE_BMP180

#define USE_MICROSD

#define USE_BRIGHTNESS_SENSOR
#define USE_TSL2561

#define USE_I2C

#define USE_MICROSD

#define USE_FACTORY_RESET

#define USE_RESERVOIR
#define WATER_INLET_PIN

#endif
