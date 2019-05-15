/*
 * HydroMonitorLogging
 *

   Message file format:
    - byte 0: records whether that record has been sent already.
    - byte 1: message type (log level).
    - byte 2-5: timestamp (seconds since epoch)
    - byte 6 - n+6: the message itself in ASCII format, null terminated.
    Minimum record size: 6; maximum record size: 506.
    
 */
 
#ifndef HYDROMONITORLOGGING_H
#define HYDROMONITORLOGGING_H

#include <HydroMonitorCore.h>
#include <FS.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266WiFi.h>

//#define RECORD_STATUS_SENT_BIT            0
//#define RECORD_STATUS_EMAILED_BIT         1

#define CONNECTION_RETRY_DELAY            60*60*1000

// Maximum file size to store messages and data.
#define MAX_LOGFILE_SIZE                  20000
#define MAX_FILE_SIZE                     20000

#define RECORD_STORED                     0x01
#define RECORD_TRANSMITTED                0x02

#define INVALID 0
#define UNCHECKED 1
#define VALID 2


class HydroMonitorLogging
{
  public:

    struct Settings {
      char hostname[101];
      char hostpath[151];
      char username[33];
      char password[33];
    };

    HydroMonitorLogging();
    void begin(HydroMonitorCore::SensorData*);

    void updateSettings(ESP8266WebServer*);
    void settingsHtml(ESP8266WebServer*);
    bool settingsJSON(ESP8266WebServer*);
    
    void messagesJSON(ESP8266WebServer*);

    void logData();
    void getLogData(uint8_t);
    void logMessage(char*);
    char* getLogMessage(uint8_t*, uint32_t*, char*);
    void resetLog();
    void transmitData();
    void transmitMessages();
    
    void checkCredentials(void);
    uint8_t hostValid;
    uint8_t pathValid;
	  uint8_t loginValid;

	  void writeTrace(const char*);
	  void writeTrace(const __FlashStringHelper*);
	  void writeInfo(const char*);
	  void writeInfo(const __FlashStringHelper*);
	  void writeWarning(const char*);
	  void writeWarning(const __FlashStringHelper*);
	  void writeError(const char*);
	  void writeError(const __FlashStringHelper*);
	  
  private:
	  void checkCredentials (char*, char*, char*, char*);
	  
	  void writeLog(uint8_t);
	  void bufferMsg_P(const char*);
	  void bufferMsg(const char*);
    char msg[MAX_MESSAGE_SIZE];                             // Buffer to store the message to log.

    uint16_t sendPostData(char*);
    void initDataLogFile();
    void initMessageLogFile();
    void addMessageToList(uint32_t);
    uint32_t latestMessageList[50];
    uint32_t lastSent;
	  
    Settings settings;
    Settings localSettings;
    HydroMonitorCore::SensorData *sensorData;
    HydroMonitorCore core;
    
    const uint16_t httpsPort = 443;
    char* username;
    char* password;
    
    uint32_t lastLogSensorData = -REFRESH_DATABASE;
    bool dataTransmitComplete;
    uint32_t dataRecordToTransmit;
    bool messageTransmitComplete;
    uint32_t messageToTransmit;
	  bool connectionFailed = false;
    uint32_t connectionFailTime = -CONNECTION_RETRY_DELAY;

    const char* dataLogFileName = "datalog";
    const char* dataLogFile1Name = "datalog1";

    const char* messageLogFileName = "messagelog";
    const char* messageLogFile1Name = "messagelog1";
    
    const uint8_t dataRecordSize = sizeof(HydroMonitorCore::SensorData);
    const uint8_t fileRecordSize = dataRecordSize + 1;
};
#endif
