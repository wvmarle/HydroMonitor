#ifndef HYDROMONITOR_TEST_EVERYTHING_3_h
#define HYDROMONITOR_TEST_EVERYTHING_3_h

/*
 ***************************************************************************************
 */
// This one is for testing only!
// All the MCP connected stuff and the BME280
#ifdef EVERYTHING_3

#define USE_EC_SENSOR
#define CAPPOS_PIN 0
#define CAPNEG_PIN 1
#define EC_PIN 2

#define USE_PH_SENSOR
#define PH_SENSOR_ADS_PIN 0

#define USE_BRIGHTNESS_SENSOR
#define USE_TSL2591

#define USE_WIFILED
#define WIFILED_MCP_PIN 0

#define USE_WATERLEVEL_SENSOR
#define USE_HCSR04
#define ECHO_PIN 0
#define TRIG_MCP_PIN 1

#define USE_GROWLIGHT
#define GROWLIGHT_MCP_PIN 0

#define USE_FERTILISER
#define FERTILISER_B_MCP_PIN 0
#define FERTILISER_A_MCP_PIN 1

#define USE_PHMINUS
#define PHMINUS_MCP_PIN 0

#define USE_PRESSURE_SENSOR
#define USE_TEMPERATURE_SENSOR
#define USE_HUMIDITY_SENSOR
#define USE_BME280

#define USE_I2C
#define USE_MCP23008
#define USE_ADS1115

#define USE_WATERTEMPERATURE_SENSOR
#define USE_NTC
#define NTC_ADS_PIN 1
#define THERMISTORNOMINAL 10000
#define TEMPERATURENOMINAL 25
#define BCOEFFICIENT 3950
#define NTCSERIESRESISTOR 100000
#define ADCMAX 3388

#define USE_RESERVOIR
#define RESERVOIR_MCP_PIN

#endif

