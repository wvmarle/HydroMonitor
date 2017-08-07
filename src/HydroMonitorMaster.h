/*
 * HydroMonitorMaster
 *
 * This does most of the work for the HydroMonitor, and is designed to be completely hardware agnostic: it lets
 * the calling sketch set all the hardware parameters and so, and then does all the rest of the work.
 *
 */

#ifndef HYDROMONITORMASTER_H
#define HYDROMONITORMASTER_H

#include <WiFiManager.h>
#include <Time.h>
#include <Adafruit_ADS1015.h>
#include <ESP8266WebServer.h>
#include <pcf8574_esp.h>

#include <HydroMonitorECSensor.h>
#include <HydroMonitorWaterTempSensor.h>
#include <HydroMonitorWaterLevelSensor.h>
#include <HydroMonitorBrightnessSensor.h>
#include <HydroMonitorAirPressureSensor.h>
#include <HydroMonitorAirTempSensor.h>
#include <HydroMonitorHumiditySensor.h>
#include <HydroMonitorGrowlight.h>
#include <HydroMonitorNetwork.h>

#define EEPROM_ADDRESS 0
#define EEPROM_SIZE 1024

class HydroMonitorMaster 
{

  public:

    struct Settings {    
      HydroMonitorECSensor::Settings ECSensor;
      HydroMonitorECSensor::Settings ECSensor1;
      HydroMonitorECSensor::Settings ECSensor2;
      HydroMonitorECSensor::Settings ECSensor3;
      HydroMonitorWaterLevelSensor::Settings WaterLevelSensor;
      HydroMonitorAirPressureSensor::Settings AirPressureSensor;
      HydroMonitorGrowlight::Settings Growlight;
      HydroMonitorWaterTempSensor::Settings WaterTempSensor;
      HydroMonitorBrightnessSensor::Settings BrightnessSensor;
      HydroMonitorNetwork::Settings Network;
      HydroMonitorAirTempSensor::Settings AirTempSensor;
      HydroMonitorHumiditySensor::Settings HumiditySensor;
    };
    static const int nSensors = 10;
    
    HydroMonitorMaster (void);
    void begin();
    
    void enableECSensor1(unsigned char, unsigned char, unsigned char);
    void enableECSensor2(unsigned char, unsigned char, unsigned char);
    void enableECSensor3(unsigned char, unsigned char, unsigned char);
    void enableTSL2561();
    void enableGrowlight(unsigned char);
    void enableBMP180();
    void enableHCSR04(unsigned char, unsigned char, PCF857x*);
    void enableThermistor(unsigned char, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);
    void enableThermistor(unsigned char, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, Adafruit_ADS1115*);
    void enableDHT22(unsigned char);
    void enableNetwork();
    
    // The various recurring tasks, called by the TaskScheduler from the main sketch.
    void readSensors(void);
    void WiFiConnection(void);
    void WiFiBlink(void);
    void sendData(void);
    void ntpUpdateInit(void);
    
    // The main loop.
    void execute(void);
    
  
  private:

    // For the internal time keeping and NTP connectivity.
    String timestamp;
    int NTPtries;
    IPAddress timeServerIP;               // time.nist.gov NTP server address
    char* ntpServerName;
    byte packetBuffer[NTP_PACKET_SIZE];   // NTP time stamp is in the first 48 bytes of the message
    WiFiUDP udp;                          // A UDP instance to let us send and receive packets over UDP
    unsigned long epoch;
    void ntpCheck(void);
    unsigned long sendNTPpacket(IPAddress&);
    bool doNtpUpdateCheck(void);
    
    // The various variables containing the data from the sensors.
    double EC1, EC2, EC3;
    double waterlevel;
    unsigned int brightness;
    double watertemp;
    double pressure;
    
    // The various sensors.
    HydroMonitorBrightnessSensor brightnessSensor;
    bool useBrightnessSensor;
    HydroMonitorECSensor ECSensor1;
    bool useECSensor1;
    HydroMonitorECSensor ECSensor2;
    bool useECSensor2;
    HydroMonitorECSensor ECSensor3;
    bool useECSensor3;
    HydroMonitorWaterTempSensor waterTempSensor;
    bool useWaterTempSensor;
    HydroMonitorWaterLevelSensor waterLevelSensor;
    bool useWaterLevelSensor;
    HydroMonitorAirPressureSensor airPressureSensor;
    bool usePressureSensor;
    HydroMonitorAirTempSensor airTempSensor;
    bool useAirTempSensor;
    HydroMonitorHumiditySensor humiditySensor;
    bool useHumiditySensor;
    HydroMonitorGrowlight growlight;
    bool useGrowlight;
    
    // Networking
    HydroMonitorNetwork network;
    bool useNetwork;
    
    // Date and time.
    void currentTime(void);

    // Settings and other data storage.
    void readSettings(void);
    void writeSettings(void);
    String prependZeros(unsigned int, unsigned char);
    HydroMonitorNetwork::SensorData sensorData;
    void manageSettings(ESP8266WebServer);
    Settings settings;
    String sensorList[nSensors];
    
    // Calibration of the EC sensor(s).
    void calibrate_ec(ESP8266WebServer);
    void calibrate_ec1(ESP8266WebServer);
    void calibrate_ec2(ESP8266WebServer);
    void calibrate_ec3(ESP8266WebServer);    
    
    // The timing variables.
    unsigned long int lastReadSensor;
    unsigned long int lastSendData;
    
    // Web server handling.
    void handleAPI();
    void handleRoot();
    void handleNotFound();
    
};
#endif
