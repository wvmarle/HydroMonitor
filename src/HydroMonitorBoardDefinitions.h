/*
 * HydroMonitorBoardDefinitions.h
 *
 * (C) Wouter van Marle / City Hydroponics
 * www.cityhydroponics.hk
 *
 * A list of availalable boards - and prototypes - with hardware and pin configurations.
 *
 */
 
#ifndef HYDROMONITORBOARDDEFINITIONS_h
#define HYDROMONITORBOARDDEFINITIONS_h


// Port definitions for various prototypes.
// Uncomment one and only one of the below boards!

// The first HydroMonitorCH8 prototype.
// Reserverd IP address: 192.168.1.128.
//#define board_128

// First Hydromonitor Pro prototype - with SD card, DHT22, 4x 12V outputs, PCF8574 port extender.
// Reserverd IP address: 192.168.1.129.
//#define board_129

// Second Hydromonitor Pro prototype - with SD card, water_sensors, 4x 12V outputs, 1x 5V output, MCP23008 port extender.
// Reserverd IP address: 192.168.1.126.
//#define board_126

#define CH8


// Defaults for this individual board.
#define OTA_PASSWORD "esp"
#define MYSQL_USERNAME "board_126"             // MySQL user name. It's a string, so needs the quote marks.
#define MYSQL_PASSWORD "board_126_password"    // MySQL password. It's a string, so needs the quote marks.

// How to do the logging.
//#define LOG_SERIAL  // Send log info to the Serial console.
#define LOG_MYSQL   // Send log info to the MySQL database.

// Set the log level.
#define LOGLEVEL LOG_TESTING



// Hardware configuration the various prototypes and other boards.


/*
 ***************************************************************************************
 */
#ifdef board_126

#define USE_SERIAL

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
#define CARD_DETECT_MCP_PIN 7

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


/*
 ***************************************************************************************
 */
#ifdef board_128

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


/*
 ***************************************************************************************
 */
#ifdef board_129

//#define USE_SERIAL

// Connecteded direct to the ESP8266.
#define USE_WATERLEVEL_SENSOR
#define USE_HCSR04
#define ECHO_PIN 16        // HC-SR04 range finder - echo pin.

#define USE_EC_SENSOR
#define CAPPOS_PIN 0       // CapPos/C+ for EC sensor.
#define CAPNEG_PIN 1       // CapNeg/C- for EC sensor.
#define EC_PIN 3           // ECpin/EC for EC sensor.

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

#define TRIG_PCF_PIN 0         // HC-SR04 range finder - trig pin.
// P1 and P2 are not connected.
#define USE_WIFILED
#define WIFILED_PCF_PIN 3         // Indicator LED - on when WiFi is connected.
#define USE_FERTILISER
#define FERTILISER_B_PCF_PIN 4    // peristaltic pump for fertiliser.
#define FERTILISER_A_PCF_PIN 5    // peristaltic pump for fertiliser.
#define USE_PHMINUS
#define PHMINUS_PCF_PIN 6        // peristaltic pump for pH-minus.
#define USE_GROWLIGHT
#define GROWLIGHT_PCF_PIN 7       // Growing light.

// Connected to the ADS1115.
#define USE_ADS1115
// A0 is not connected.
#define USE_WATERTEMPERATURE_SENSOR
#define USE_NTC
#define NTC_ADS_PIN 1         // The thermistor for water temperature.
//#define USE_DO_SENSOR
//#define DO_SENSOR_ADS_PIN 2   // Dissolved oxygen sensor.

// pH sensor causes crash?!
#define USE_PH_SENSOR
#define PH_SENSOR_ADS_PIN 3   // pH sensor

// NTC parameters.
#define THERMISTORNOMINAL 10000  // Nominal temperature value for the thermistor
#define TEMPERATURENOMINAL 25    // Nominal temperature depicted on the datasheet
#define BCOEFFICIENT 3950        // Beta value for our thermistor
#define NTCSERIESRESISTOR 10000  // Value of the series resistor
#define ADCMAX 26400             // 32768*3.3/4.096
#endif


/*
 ***************************************************************************************
 */
// The CH8 boards with "water sensors" board.
#ifdef CH8

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

#define USE_HUMIDITY_SENSOR
#define USE_TEMPERATURE_SENSOR
#define DHT22_PIN 2
#endif


/*
 ***************************************************************************************
 */
#ifdef NODEMCU
#define USE_EC_SENSOR
#define CAPPOS_PIN D7      // CapPos/C+ for EC sensor.
#define CAPNEG_PIN D8      // CapNeg/C- for EC sensor.
#define EC_PIN D4          // ECpin/EC for EC sensor.
#endif


/*
 ***************************************************************************************
 */
// This one is for testing only!
// It enables every single hardware option, which is of course unrealistic but makes sure everything compiles properly.
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

//#define USE_DO_SENSOR
//#define DO_SENSOR_PIN 0

//#define USE_ORP_SENSOR
//#define ORP_SENSOR_PIN 0

#define USE_TEMPERATURE_SENSOR
#define USE_PRESSURE_SENSOR
#define USE_BMP180

#define USE_MICROSD

#define USE_BRIGHTNESS_SENSOR
#define USE_TSL2561

#define USE_I2C

#define USE_MICROSD

#define USE_FACTORY_RESET

#endif

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

//#define USE_DO_SENSOR
//#define DO_SENSOR_ADS_PIN 0

//#define USE_ORP_SENSOR
//#define ORP_SENSOR_ADS_PIN 0

#define USE_PRESSURE_SENSOR
#define USE_TEMPERATURE_SENSOR
#define USE_BMP280

#define USE_BRIGHTNESS_SENSOR
#define USE_TSL2591

#define USE_I2C
#define USE_PCF8574
#define USE_ADS1115

#endif

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


#endif

/*
 ***************************************************************************************
 */

// Whatever is left.
#ifdef EVERYTHING_4
#define USE_WATERLEVEL_SENSOR
#define USE_WATERTEMPERATURE_SENSOR
#define USE_MS5837

#define USE_PRESSURE_SENSOR
#define USE_TEMPERATURE_SENSOR
#define USE_HUMIDITY_SENSOR
#define USE_BME280

#endif



#endif
