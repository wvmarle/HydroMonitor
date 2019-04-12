#ifndef HYDROMONITOR_TEST_EVERYTHING_2_h
#define HYDROMONITOR_TEST_EVERYTHING_2_h

/*
 ***************************************************************************************
 */
// This one is for testing only!
// All PCF & ADS connected things, the BMP280 and the TSL2591.
#ifdef EVERYTHING_2

#define USE_EC_SENSOR
#define CAPPOS_PIN 0
#define CAPNEG_PIN 1
#define EC_PIN 2

#define USE_WIFILED
#define WIFILED_PCF_PIN 0

#define USE_WATERLEVEL_SENSOR
#define USE_HCSR04
#define ECHO_PIN 0
#define TRIG_PCF_PIN 1

#define USE_GROWLIGHT
#define GROWLIGHT_PCF_PIN 0

#define USE_WATERTEMPERATURE_SENSOR
#define USE_NTC
#define NTC_ADS_PIN 1
#define THERMISTORNOMINAL 10000
#define TEMPERATURENOMINAL 25
#define BCOEFFICIENT 3950
#define NTCSERIESRESISTOR 100000
#define ADCMAX 3388

#define USE_FERTILISER
#define FERTILISER_B_PCF_PIN 0
#define FERTILISER_A_PCF_PIN 1

#define USE_PHMINUS
#define PHMINUS_PCF_PIN 0

#define USE_PH_SENSOR
#define PH_SENSOR_ADS_PIN 0

#define USE_DO_SENSOR
#define DO_SENSOR_ADS_PIN 0

#define USE_ORP_SENSOR
#define ORP_SENSOR_ADS_PIN 0

#define USE_PRESSURE_SENSOR
#define USE_TEMPERATURE_SENSOR
#define USE_BMP280

#define USE_BRIGHTNESS_SENSOR
#define USE_TSL2591

#define USE_I2C
#define USE_PCF8574
#define USE_ADS1115

#define USE_RESERVOIR
#define RESERVOIR_PCF_PIN

#endif

