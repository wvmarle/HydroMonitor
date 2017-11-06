/*
 * HydroMonitorMySQL
 *
 */

#ifndef HYDROMONITORMYSQL_H
#define HYDROMONITORMYSQL_H

#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <Time.h>
#include <HydroMonitorCore.h>

#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>

class HydroMonitorMySQL
{
  public:

    struct Settings {
      char mySQLHostname[101];
      char mySQLUsername[33];
      char mySQLPassword[33];
    };

    HydroMonitorMySQL();
    void begin(HydroMonitorCore::SensorData*);

    void sendData(void);
    String settingsHtml(void);
    void updateSettings(String[], String[], uint8_t);
    void checkCredentials(void);
	  bool loginValid;
	  void writeTrace(char*);
	  void writeDebug(char*);
	  void writeTesting(char*);
	  void writeInfo(char*);
	  void sendWarning(char*);
	  
  private:
    char* username;
    char* password;
	  void checkCredentials (char*, char*, char*);
	  void writeLog(char*);
	  void doQuery(char*);
    
    Settings settings;
    HydroMonitorCore::SensorData *sensorData;
    HydroMonitorCore core;
};
#endif
