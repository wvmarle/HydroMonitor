/*
 * HydroMonitorNetwork
 *
 */

#ifndef HYDROMONITORNETWORK_H
#define HYDROMONITORNETWORK_H

#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// WiFi server settings.
#define CONNECT_TIMEOUT   30      // Seconds
#define CONNECT_OK        0       // Status of successful connection to WiFi
#define CONNECT_FAILED    (-99)   // Status of failed connection to WiFi
#define HTTPSPORT         443

#define REFRESH 10                // Refresh rate (of the html page and the sensor readings) in seconds.

// Internal time keeping and NTP connectivity.
#define TIME_MSG_LEN 11           // time sync to PC is HEADER followed by Unix time_t as ten ASCII digits
#define TIME_HEADER 'T'           // Header tag for serial time sync message
#define TIME_REQUEST 7            // ASCII bell character requests a time sync message
#define NTPMAXTRIES 30
#define LOCAL_NTP_PORT 2390       // local port to listen for UDP packets
#define NTP_SERVER_NAME "time.nist.gov"
#define NTP_PACKET_SIZE 48        // NTP time stamp is in the first 48 bytes of the message


class HydroMonitorNetwork
{
  public:

    struct Settings {
      char MysqlHostname[100];
      char MysqlUrlBase[100];
      char MysqlUsername[32];
      char MysqlPassword[32];
      char Hostname[32];
      char Ssid[32];
      char WiFiPassword[64];
      char Timezone;
    };

    struct SensorData {
      bool useECSensor; double EC;
      bool useECSensor1; double EC1;
      bool useECSensor2; double EC2;
      bool useECSensor3; double EC3;
      bool useBrightnessSensor; unsigned int brightness;
      bool useWaterTempSensor; double waterTemp;
      bool useWaterLevelSensor; double waterLevel;
      bool usePressureSensor; double pressure;
      bool useGrowlight; bool growlight;
    };

    struct Request {
      String request;
      String keys[];
      String values[];
    };

    HydroMonitorNetwork(void);
    void begin(Settings);
    void setSettings(Settings);
    void WiFiBlink();
    void connectInit();
    void sendData(SensorData);
    void settingsPage(String);
    Request WiFiConnection(SensorData);
    void ntpUpdateInit();
    void ntpCheck();
    unsigned long sendNTPpacket(IPAddress&);
    bool doNtpUpdateCheck();
    void htmlResponse(ESP8266WebServer, String);
    void plainResponse(ESP8266WebServer, String);
    String createSensorHtml(SensorData);
    String createHtmlPage(String, bool);
    void handleNotFound(ESP8266WebServer);
    String handleAPI(ESP8266WebServer, String*, int, String*, String*);
    
  private:
    
    // For the internal time keeping and NTP connectivity.
    String timestamp;
    int NTPtries;
    IPAddress timeServerIP;               // time.nist.gov NTP server address
    byte packetBuffer[NTP_PACKET_SIZE];   //buffer to hold incoming and outgoing packets
    WiFiUDP udp;                          // A UDP instance to let us send and receive packets over UDP
    unsigned long epoch;
    WiFiClientSecure client;
    unsigned char WiFiLED;
    String createHtmlPage(SensorData);
    Settings settings;
    bool hasValue(String, String[], int);
    
    String validateDate(String);
    String validateNumber(String);
    
    void handleRoot(void);

};
#endif
