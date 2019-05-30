/*
   HydroMonitorNetwork

*/

#ifndef HYDROMONITORNETWORK_H
#define HYDROMONITORNETWORK_H

#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <TimeLib.h>
#include <HydroMonitorCore.h>
#include <HydroMonitorLogging.h>

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
#define NTP_SERVER_NAME "pool.ntp.org"
#define NTP_PACKET_SIZE 48        // NTP time stamp is in the first 48 bytes of the message

class HydroMonitorNetwork
{
  public:

    struct Settings {
    };

    struct Request {
      String request;
      String keys[];
      String values[];
    };

    HydroMonitorNetwork(void);
    void begin(HydroMonitorCore::SensorData*, HydroMonitorLogging*, ESP8266WebServer*);

    void ntpUpdateInit();
    bool ntpCheck();
    void htmlPageHeader(ESP8266WebServer*, bool);
    void htmlPageFooter(ESP8266WebServer*);
    void htmlResponse(ESP8266WebServer*);
    void plainResponse(ESP8266WebServer*);
    void settingsHtml(ESP8266WebServer*);
    bool settingsJSON(ESP8266WebServer*);
    void updateSettings(ESP8266WebServer*);


  private:

    // For the internal time keeping and NTP connectivity.
    uint32_t sendNTPpacket(IPAddress&);
    void connectInit();
    bool doNtpUpdateCheck();
    uint8_t NTPtries;
    IPAddress timeServerIP;               // time.nist.gov NTP server address
    byte packetBuffer[NTP_PACKET_SIZE];   //buffer to hold incoming and outgoing packets
    WiFiUDP udp;                          // A UDP instance to let us send and receive packets over UDP
    uint32_t epoch;

    Settings settings;
    ESP8266WebServer *server;

    uint32_t startTime;
    uint32_t updateTime;
    HydroMonitorCore core;
    HydroMonitorCore::SensorData *sensorData;
    HydroMonitorLogging *logging;
};
#endif
