#include <HydroMonitorMySQL.h>

/*
 * Take care of database connectivity (expects networking to be enabled).
 */

WiFiClient client;
MySQL_Connection conn((Client *)&client);
IPAddress server_ip;
MySQL_Cursor *cur_mem;
 
/*
 * The constructor.
 */
HydroMonitorMySQL::HydroMonitorMySQL() {
}

/*
 * Start the network services.
 */
void HydroMonitorMySQL::begin(HydroMonitorCore::SensorData *sd) {

  sensorData = sd;
  if (MYSQL_EEPROM > 0)
    EEPROM.get(MYSQL_EEPROM, settings);
  
  // Default settings, to be applied if the first byte is 255, which indicates the EEPROM
  // has never been written.
  if ((int)settings.mySQLHostname[0] == 255) {
    strlcpy(settings.mySQLHostname, MYSQL_HOSTNAME, 100);
    strlcpy(settings.mySQLUsername, MYSQL_USERNAME, 32);
    strlcpy(settings.mySQLPassword, MYSQL_PASSWORD, 32);
  }
  checkCredentials();
  writeTesting("HydroMonitorMySQL: configured MySQL connection.");
  if (loginValid) 
    writeTesting("HydroMonitorMySQL: stored login credentials valid.");
  else 
    writeTesting("HydroMonitorMySQL: stored login credentials invalid.");
  return;
}

/**
 * Send the latest sensor data to the database.
 */
void HydroMonitorMySQL::sendData() {

  if (loginValid == false)
    return;
    
//  WiFiClient client;
//  MySQL_Connection conn((Client *)&client);
//  IPAddress server_ip;
//  WiFi.hostByName(settings.mySQLHostname, server_ip);
//  if (conn.connect(server_ip, 3306, settings.mySQLUsername, settings.mySQLPassword) == false) 
//    return; // Don't try to continue if we can't connect to the server successfully.
    
  // Prepare the data string to be sent to the server.
  char query[290] = "";
  strcat(query, "INSERT INTO ch_");
  strcat(query, MYSQL_USERNAME);
  strcat(query, ".");
  strcat(query, MYSQL_DATA);
  strcat(query, " ");
  char fields[110] = "(";
  char values[90] = " VALUES (";
  char val[10];
  
#ifdef USE_EC_SENSOR
    strcat(fields, "EC, ");
    strcat(values, dtostrf(sensorData->EC, 4, 2, val));
    strcat(values, (", "));
#endif
#ifdef USE_BRIGHTNESS_SENSOR
    strcat(fields, "brightness, ");
    sprintf(val, "%d", sensorData->brightness);
    strcat(values, val);
    strcat(values, ", ");
#endif
#ifdef USE_WATERTEMPERATURE_SENSOR
    strcat(fields, "watertemp, ");
    strcat(values, dtostrf(sensorData->waterTemp, 4, 2, val));
    strcat(values, ", ");
#endif
#ifdef USE_WATERLEVEL_SENSOR
    strcat(fields, "waterlevel, ");
    strcat(values, dtostrf(sensorData->waterLevel, 4, 2, val));
    strcat(values, ", ");
#endif
#ifdef USE_PRESSURE_SENSOR
    strcat(fields, "pressure, ");
    strcat(values, dtostrf(sensorData->pressure, 4, 2, val));
    strcat(values, ", ");
#endif
#ifdef USE_TEMPERATURE_SENSOR
    strcat(fields, "airtemp, ");
    strcat(values, dtostrf(sensorData->temperature, 4, 2, val));
    strcat(values, ", ");
#endif
#ifdef USE_HUMIDITY_SENSOR
    strcat(fields, "humidity, ");
    strcat(values, dtostrf(sensorData->humidity, 4, 2, val));
    strcat(values, ", ");
#endif
#ifdef USE_PH_SENSOR
    strcat(fields, "ph, ");
    strcat(values, dtostrf(sensorData->pH, 4, 2, val));
    strcat(values, ", ");
#endif
#ifdef USE_DO_SENSOR
    strcat(fields, "DO, ");
    strcat(values, dtostrf(sensorData->DO, 4, 2, val));
    strcat(values, ", ");
#endif
#ifdef USE_ORP_SENSOR
    strcat(fields, "ORP, ");
    strcat(values, dtostrf(sensorData->ORP, 4, 2, val));
    strcat(values, ", ");
#endif
#ifdef USE_GROWLIGHT
    strcat(fields, "growlight, ");
    if (sensorData->growlight) strcat(values, "1, ");
    else strcat(values, "0, ");
#endif
  
  strncat(query, fields, strlen(fields)-2);
  strcat(query, ")");
  strncat(query, values, strlen(values)-2);
  strcat(query, ");");
 
  writeDebug("HydroMonitorMySQL: Sending out data.");
  writeDebug(query);
  doQuery(query);
  
//  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);  // This one works pefectly.
//  cur_mem->execute(query);
//  delete cur_mem; 

//  conn.close();
//  client.stop();
  return;
}

/*
 * Check a set of login credentials to a MySQL database.
 *
 * Returns true if valid; false if invalid.
 */ 
void HydroMonitorMySQL::checkCredentials() {
  checkCredentials(settings.mySQLHostname, settings.mySQLUsername, settings.mySQLPassword);
  return;
}

void HydroMonitorMySQL::checkCredentials(char* host, char* un, char* pw) {
  IPAddress server_ip;
  WiFi.hostByName(host, server_ip);
  WiFiClient client;
  MySQL_Connection conn((Client *)&client);
  loginValid = conn.connect(server_ip, 3306, un, pw);
  conn.close();
  client.stop();
  return;
}

/*
 * Write log message to the database.
 */
void HydroMonitorMySQL::writeLog(char *msg) {

#if defined(LOG_SERIAL) && defined(SERIAL)
  Serial.println(msg);
#endif

#ifdef LOG_MYSQL
  if (loginValid) {

    // Prepare the command string to be sent to the server.
    char query[40 + strlen(msg) + strlen(MYSQL_USERNAME) + strlen(MYSQL_LOG)];
    strcpy(query, "INSERT INTO ch_");
    strcat(query, MYSQL_USERNAME);
    strcat(query, ".");
    strcat(query, MYSQL_LOG);
    strcat(query, " (message) VALUES (\"");
    strncat(query, msg, strlen(msg));
    strcat(query, "\");");
    doQuery(query);
  }
#endif
  return;
}

void HydroMonitorMySQL::writeTrace(char *msg) {
  if (LOGLEVEL >= LOG_TRACE)
    writeLog(msg);
}

void HydroMonitorMySQL::writeDebug(char *msg) {
  if (LOGLEVEL >= LOG_DEBUG)
    writeLog(msg);
}

void HydroMonitorMySQL::writeTesting(char *msg) {
  if (LOGLEVEL >= LOG_TESTING)
    writeLog(msg);
}

void HydroMonitorMySQL::writeInfo(char *msg) {
  if (LOGLEVEL >= LOG_INFO)
    writeLog(msg);
}

void HydroMonitorMySQL::sendWarning(char *msg) {
  //TODO implement this: send e-mail to the user.

}

void HydroMonitorMySQL::doQuery(char *query) {
  
  if (conn.connected() == false) {
    delete cur_mem; 
    conn.close();
    
    WiFi.hostByName(settings.mySQLHostname, server_ip);
    if (conn.connect(server_ip, 3306, settings.mySQLUsername, settings.mySQLPassword) == false) 
      return; // Don't try to continue if we can't connect to the server successfully.

    cur_mem = new MySQL_Cursor(&conn);
    
  }
  cur_mem->execute(query);
}

/*
 * The settings as HTML.
 */
String HydroMonitorMySQL::settingsHtml() {
  String html;
  html = F("\
      <tr>\n\
        <th colspan=\"2\">Networking settings.</th>\n\
      </tr><tr>\n\
        <td>MySQL host name:</td>\n\
        <td><input type=\"text\" name=\"network_mysql_hostname\" value=\"");
  html += String(settings.mySQLHostname);
  html += F("\"></td>\n\
      </tr><tr>\n\
        <td>MySQL username:</td>\n\
        <td><input type=\"text\" name=\"network_mysql_username\" value=\"");
  html += String(settings.mySQLUsername);
  html += F("\"></td>\n\
      </tr><tr>\n\
        <td>MySQL password:</td>\n\
        <td><input type=\"text\" name=\"network_mysql_password\" value=\"");
  html += String(settings.mySQLPassword);
  html += F("\"></td>\n\
      </tr><tr>\n\
        <td></td>\n");
  if (loginValid) html += F("<td><span style=\"color:green\">Login valid.</span></td>");
  else html += F("<td><span style=\"color:red\">Login invalid.</span></td>");
  html += F("\
      </tr>");
  return html;
}

/*
 * Update the settings.
 */
void HydroMonitorMySQL::updateSettings(String keys[], String values[], uint8_t nArgs) {
  char hostname[100];
  char username[32];
  char password[32];
  for (uint8_t i=0; i<nArgs; i++) {
    if (keys[i] == "network_mysql_hostname") {
      if (keys[i].length() < 100) values[i].toCharArray(hostname, 100);
    }
    if (keys[i] == "network_mysql_username") {
      if (keys[i].length() < 32) values[i].toCharArray(username, 32);
    }
    if (keys[i] == "network_mysql_password") {
      if (keys[i].length() < 32) values[i].toCharArray(password, 32);
    }
  }

  // If nothing changed, just keep the original settings as is.  
  if (settings.mySQLHostname == hostname 
      && settings.mySQLUsername == username 
      && settings.mySQLPassword == password)
      return;

  // If any settings changed, check whether the login is valid. Only change the settings if the new
  // credentials are correct.
  checkCredentials(hostname, username, password);
  if (loginValid) {
    strlcpy(settings.mySQLHostname, hostname, 100);
    strlcpy(settings.mySQLUsername, username, 32); 
    strlcpy(settings.mySQLPassword, password, 32);
  }
  EEPROM.put(MYSQL_EEPROM, settings);
  EEPROM.commit();
  return;
}
