#include <HydroMonitorLogging.h>

/*
 * Take care of database connectivity (expects networking to be enabled).
 */


/*
 * The constructor.
 */
HydroMonitorLogging::HydroMonitorLogging() {

}

/*
 * Start the network services.
 */
void HydroMonitorLogging::begin(HydroMonitorCore::SensorData *sd) {
  sensorData = sd;
  if (LOGGING_EEPROM > 0)
#ifdef USE_24LC256_EEPROM
    sensorData->EEPROM->get(LOGGING_EEPROM, settings);
#else
    EEPROM.get(LOGGING_EEPROM, settings);
#endif
  
  // Default settings, to be applied if the first byte is 255, which indicates the EEPROM
  // has never been written.
  if ((int)settings.hostname[0] == 255) {
    strlcpy(settings.hostname, LOGGING_HOSTNAME, 100);
    strlcpy(settings.hostpath, LOGGING_HOSTPATH, 150);
    strlcpy(settings.username, LOGGING_USERNAME, 32);
    strlcpy(settings.password, LOGGING_PASSWORD, 32);
#ifdef USE_24LC256_EEPROM
    sensorData->EEPROM->put(LOGGING_EEPROM, settings);
#else
    EEPROM.put(LOGGING_EEPROM, settings);
    EEPROM.commit();
#endif
  }
  checkCredentials();

  // Set up the local storage (SPIFFS - store in flash).
  SPIFFS.begin();                                           // Initialse the SPIFFS storage.
  initDataLogFile();
  initMessageLogFile();
}
  
//*******************************************************************************************************************
// Initialise the data logging.
// data file format:
// byte 0: transmission status.
// byte 1 ... sizeof(sensorData) + 1: the sensor data (binary).
void HydroMonitorLogging::initDataLogFile() {
  File f;
  if (SPIFFS.exists(dataLogFileName) == false) {            // If the data log does not exist,
    f = SPIFFS.open(dataLogFileName, "w");                  // create it.
    f.close();
    dataTransmitComplete = true;
    dataRecordToTransmit = 0;
  }
  else {                                                    // Data log exists already, figure out how far we were with transmission to the server->
    f = SPIFFS.open(dataLogFileName, "r");
    uint32_t nRecords = f.size() / fileRecordSize;          // Calculate number of records in the file.
    dataTransmitComplete = false;
    for (int32_t i = nRecords - 1; i >= 0; i--) {           // Start searching at the last record - usually less file to search through this way.
      f.seek(i * fileRecordSize, SeekSet);                  // Set file pointer to the beginning of the record.
      char recordStatus[1];
      f.readBytes((char *)recordStatus, 1);                 // Read the status byte of this record.
      if (recordStatus[0] == RECORD_TRANSMITTED) {          // This record has been transmitted already.
        dataRecordToTransmit = i + 1;                       // Store which one is next in line to be transmitted.
        if (dataRecordToTransmit == nRecords) {             // Last record has been transmitted already, so it's complete already.
          dataTransmitComplete = true;
        }
        break;                                              // We've found what we're looking for.
      }
    }
    connectionFailTime = -CONNECTION_RETRY_DELAY;           // We want to start transmitting right away!
  }
  f.close();
}

//*******************************************************************************************************************
// Initialise the message logging.
//
void HydroMonitorLogging::initMessageLogFile() {

  File f;
  uint32_t nMessages = 0;                                   // Total number of messages in the file.
  messageTransmitComplete = true;
  if (SPIFFS.exists(messageLogFileName) == false) {         // If the messages log does not exist,
    f = SPIFFS.open(messageLogFileName, "w");               // create it.
    f.close();
    messageToTransmit = 0;                                  // Seek index: start of first message due to be transmitted.
  }
  else {                                                    // Data log exists already, figure out how far we were with transmission to the server->
    f = SPIFFS.open(messageLogFileName, "r");
  
    // Search file for the latest sent message.
    uint32_t i = 0;                                         // The search index.
    if (f.size() > 0) {                                     // We have at least one message, otherwise the file size is zero.
      uint16_t nBytes;
      uint8_t status;
      while (i < f.size()) {                                // Search the file until the end.
        f.seek(i, SeekSet);
        status = f.read();                                  // Read the message status byte.
        f.seek(i + 6, SeekSet);                             // Set search pointer to start of the logged message - skipping the status bytes.
        nBytes = f.readBytesUntil('\0', msg, MAX_MESSAGE_SIZE - 1); // Get total size of the stored message.        
        if ((status == RECORD_STORED || status == RECORD_TRANSMITTED) &&
            nBytes <= MAX_MESSAGE_SIZE) {                   // Record appears sound.
          nMessages++;                                      // Keep track of total number of messages we have stored.
          addMessageToList(i);                              // Keep track of the starting points of the last 50 messages.
          i += nBytes + 7;                                  // Set index to start of the next message (which starts nBytes + 6 control + 1 null terminator further).
          if (status == RECORD_TRANSMITTED) {               // This message has been transmitted already,
            messageToTransmit = i;                          // but maybe the next not yet. Set start index to that point.
          }
        }
        else {
          f.close();
          SPIFFS.remove(messageLogFileName);
          nMessages = 0;
          messageToTransmit = 0;
          break;
        }
      }
    }
  }
  sprintf_P(msg, PSTR("HydroMonitorLogging: messages in this file: %d."), nMessages);
  writeTrace(msg);
  sprintf_P(msg, PSTR("HydroMonitorLogging: first unsent message starts at byte %d."), messageToTransmit);
  writeTrace(msg);
  if (messageToTransmit < f.size() &&
      f.size() > 0) {                                       // We have unsent messages.
    messageTransmitComplete = false;
  }
  f.close();  
  writeTrace(F("HydroMonitorLogging: configured message logging facility."));
}

void HydroMonitorLogging::addMessageToList(uint32_t msgIndex) {
  for (uint8_t i = 1; i < 50; i++) {
    latestMessageList[50 - i] = latestMessageList[49 - i];
  }
  latestMessageList[0] = msgIndex;
}

/********************************************************************************************************************
 * Take care of the actual logging - transmission and timing of new entries.
 */
void HydroMonitorLogging::logData() {

  // Every REFRESH_DATABASE milliseconds: log the sensor data, and try to transmit it to the database.
  if (millis() - lastLogSensorData > REFRESH_DATABASE) {
    lastLogSensorData += REFRESH_DATABASE;
    dataTransmitComplete = false;                           // We're adding a new record, so transmission is required.
    File f = SPIFFS.open(dataLogFileName, "a");             // Open the file, append mode.
    f.write(RECORD_STORED);                                 // First byte: status (it's merely stored at the moment).
    uint8_t buff[dataRecordSize];                           // Create a copy of the sensorData in different type
    memcpy(buff, sensorData, dataRecordSize);               // for easier writing to the file.
    for (uint8_t i = 0; i < dataRecordSize; i++) {          // TODO: can this be done through cast? 
      f.write(buff[i]);                                     // Something like: (uint8_t *)sensorData[i]
    }
    f.close();
    dataTransmitComplete = false;                           // We have a new record to transmit!

    // Check log file size, and if needed roll over into a new file.
    f = SPIFFS.open(dataLogFileName, "r");
    if (f.size() > MAX_FILE_SIZE) {
      f.close();
      if (SPIFFS.exists(dataLogFile1Name)) {                // Check on the rollover file; if it exists, remove it.
        SPIFFS.remove(dataLogFile1Name);
      }
      SPIFFS.rename(dataLogFileName, dataLogFile1Name);     // Rename the original log to the rollover file.
      f = SPIFFS.open(dataLogFileName, "w");                // Create a new logfile.
      uint32_t nBytes = fileRecordSize * 20;                // Copy the latest 20 entries from the old log file to the new one.
      File f1 = SPIFFS.open(dataLogFile1Name, "r");
      f1.seek(f1.size() - nBytes, SeekSet);
//      char buff[nBytes];
//      f1.readBytes(buff, nBytes);
      for (uint32_t i = 0; i < nBytes; i++) {
//        f.write(buff[i]);
        f.write(f1.read());
      }
      f1.close();
    }
    f.close();
  }
  transmitData();
  transmitMessages();
}

/********************************************************************************************************************
 * Transmit a sensor data record to the server.
 */
void HydroMonitorLogging::transmitData() {
  if (bitRead(sensorData->systemStatus, STATUS_WATERING)) {  // Don't do this while watering.
    return;
  }

  if (loginValid != VALID) {                                // We need a valid login to be set.
    if (millis() - lastSent > WARNING_INTERVAL) {
      writeWarning(F("Logging 01: can not transmit messages or data: database login invalid."));
      lastSent = millis();
    }
    return;
  }
    
  if (connectionFailed &&                                   // If the connection failed, don't try again immediately.
      millis() - connectionFailTime < CONNECTION_RETRY_DELAY) {
    if (millis() - lastSent > CONNECTION_RETRY_DELAY) {
      writeWarning(F("Logging 02: can not transmit messages or data: connection failed."));
      lastSent = millis();
    }
    return;
  }
  
  if (dataTransmitComplete) {                               // Nothing to transmit.
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {                      // We're not connected to WiFi - can't transmit.
    return;
  }
  
  if (millis() - lastSent < 1000) {                         // Wait at least a second before sending another record for responsiveness.
    return;
  }
  
  connectionFailed = false;
  File f = SPIFFS.open(dataLogFileName, "r+");              // Open log file for read/write.

  // All data is stored already in the logfile; read back the data to transmit, attempt to transmit it, and if
  // successful mark the record as transmitted.
  HydroMonitorCore::SensorData dataEntry;
  char buff[fileRecordSize];
  f.seek(dataRecordToTransmit * fileRecordSize, SeekSet);   // Start reading from the start of the next record we have to transmit.
  f.readBytes(buff, fileRecordSize);
  memcpy(&dataEntry, buff + 1, dataRecordSize);             // Skip the record's byte 0, the status byte.
  
  uint16_t size = 90 + strlen(settings.hostname) + strlen(settings.hostpath) + strlen(settings.username) + strlen(settings.password);
  char postData[size];
  sprintf_P(postData, PSTR("https://%s%s?username=%s&password=%s&ec=%4.2f&watertemp=%4.2f&waterlevel=%4.2f&ph=%4.2f"),
                            settings.hostname, settings.hostpath, settings.username, settings.password, 
                            dataEntry.EC, dataEntry.waterTemp, dataEntry.waterLevel, dataEntry.pH);

  uint16_t httpCode = sendPostData(postData);
  if (httpCode == 200) {                                    // 200 = OK, transmissions successful.
    f.seek(dataRecordToTransmit * fileRecordSize, SeekSet);    
    f.write(RECORD_TRANSMITTED);                            // Mark file entry as transmitted.
    dataRecordToTransmit++;                                 // Proceed to next entry.
    if (f.size() == dataRecordToTransmit * fileRecordSize) {
      dataTransmitComplete = true;
    }
    else {
      dataTransmitComplete = false;
    }
  }
  else {                                                    // Connection failed: try again later.
    connectionFailed = true;
    connectionFailTime = millis();
  }
  f.close();
  lastSent = millis();
}

/********************************************************************************************************************
 * Transmit a message record to the server.
 */
void HydroMonitorLogging::transmitMessages() {
  if (bitRead(sensorData->systemStatus, STATUS_WATERING)) {  // Don't do this while watering.
    return;
  }

  if (messageTransmitComplete) {
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (millis() - lastSent < 1000) {                         // Wait at least a second before sending another record for responsiveness.
    return;
  }

  connectionFailed = false;
  File f = SPIFFS.open(messageLogFileName, "r+");           // Open log file for read/write.
  f.seek(messageToTransmit, SeekSet);                       // Set seek pointer to start of the next message.
  f.readBytes(msg, 6);                                      // The control bytes.
  uint16_t nBytes = f.readBytesUntil('\0', msg + 6, MAX_MESSAGE_SIZE - 7); // The actual message.
  msg[nBytes + 6] = 0;                                      // Null terminator.
  uint32_t timestamp;
  memcpy(&timestamp, msg + 2, 4);                           // The message's time stamp.
  uint8_t loglevel = msg[1];                                // The log level.
  String encodedMessage = core.urlencode(msg + 6);          // URLencode the message.
  uint16_t size = 80 + strlen(settings.hostname) + strlen(settings.hostpath) + strlen(settings.username) + strlen(settings.password) + encodedMessage.length();
  char postData[size];                                      // Create a buffer to hold the complete POST data.
  char aLoglevel[2];
  itoa(loglevel, aLoglevel, 10);
  char aTimestamp[12];
  itoa(timestamp, aTimestamp, 10);
  sprintf_P(postData, PSTR("https://%s%s?username=%s&password=%s&loglevel=%s&message=%s&timestamp=%s"),
            settings.hostname, settings.hostpath, settings.username, settings.password, aLoglevel, encodedMessage.c_str(), aTimestamp);
  uint16_t httpCode = sendPostData(postData);               // Post the message to the database.
  if (httpCode == 200) {                                    // 200 = OK, transmissions successful.
    f.seek(messageToTransmit, SeekSet);                     // Set seek pointer back to the start of this message: the status byte.
    f.write(RECORD_TRANSMITTED);                            // Mark file entry as transmitted.
    messageToTransmit += nBytes + 7;                        // Proceed to next entry.
    if (f.size() == messageToTransmit) {                    // We reached the end of the file - transmission completed.
      messageTransmitComplete = true;
    }
  }
  else {                                                    // Connection failed: try again later.
    connectionFailed = true;
    connectionFailTime = millis();
  }
  f.close();
  lastSent = millis();
}  

/********************************************************************************************************************
 * Check a set of login credentials.
 *
 * Returns true if valid; false if invalid.
 */ 
void HydroMonitorLogging::checkCredentials() {
  checkCredentials(settings.hostname, settings.hostpath, settings.username, settings.password);
  return;
}

void HydroMonitorLogging::checkCredentials(char* host, char* path, char* un, char* pw) {
  hostValid = UNCHECKED;
  pathValid = UNCHECKED;
  loginValid = UNCHECKED;
  
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  uint16_t size = 35 + strlen(host) + strlen(path) + strlen(un) + strlen(pw);
  char postData[size];
  sprintf_P(postData, PSTR("https://%s%s?username=%s&password=%s&validate=1"),
                           host, path, un, pw);
  
  uint16_t responseCode = sendPostData(postData);

  if (responseCode == 404) {
    hostValid = VALID;
    pathValid = INVALID;
  }  
  else if (responseCode == 403) { 
    hostValid = VALID;
    pathValid = VALID;
    loginValid = INVALID;
  }
  else if (responseCode == 200) {
    hostValid = VALID;
    pathValid = VALID;
    loginValid = VALID;
  }
}

/********************************************************************************************************************
 * Write log message to the local file.
 */
void HydroMonitorLogging::writeLog(uint8_t loglevel) {

#if defined(LOG_SERIAL) && defined(SERIAL)
  Serial.println(msg);
#endif

  // Store message to the message log file.
  File f = SPIFFS.open(messageLogFileName, "a");            // Open log file for appending.
  addMessageToList(f.size());                               // New message starts at the end of the current file.
  f.write(RECORD_STORED);                                   // Byte 0: message status.
  f.write(loglevel);                                        // Byte 1: log level.
  uint16_t size = strlen(msg) + 1;                          // Message size should include the null terminator.
  uint32_t timestamp = now();

  f.write(timestamp & 0xFF);                               // Bytes 2-5: the time stamp.
  f.write(timestamp >> 8) & 0xFF;
  f.write(timestamp >> 16) & 0xFF;
  f.write(timestamp >> 24);

  f.print(msg);                                             // Print the whole message to the file.
  f.write(0);                                               // Add the null terminator to complete the record.
  f.close();

  // Check log file size, and if needed roll over into a new file.
  // TODO: copy latest 50 messages to the new log file.
  f = SPIFFS.open(messageLogFileName, "r");
  if (f.size() > MAX_FILE_SIZE) {
    f.close();
    if (SPIFFS.exists(messageLogFile1Name)) {               // Check on the rollover file; if it exists, remove it.
      SPIFFS.remove(messageLogFile1Name);
    }
    SPIFFS.rename(messageLogFileName, messageLogFile1Name); // Rename the original log to the rollover file.
    f = SPIFFS.open(messageLogFileName, "w");               // Create a new logfile.
  }
  f.close();
  messageTransmitComplete = false;                          // Because we just added a new one!
}

/********************************************************************************************************************
 * Trace level log messages.
 */
void HydroMonitorLogging::writeTrace(const char *str) {
  if (LOGLEVEL >= LOG_TRACE) {
    bufferMsg(str);
    writeLog(LOG_TRACE);
  }
}

void HydroMonitorLogging::writeTrace(const __FlashStringHelper *str) {
  if (LOGLEVEL >= LOG_TRACE) {
    bufferMsg_P((PGM_P)str);
    writeLog(LOG_TRACE);
  }
}

/********************************************************************************************************************
 * Info level log messages.
 */
void HydroMonitorLogging::writeInfo(const char *str) {
  if (LOGLEVEL >= LOG_INFO) {
    bufferMsg(str);
    writeLog(LOG_INFO);
  }
}

void HydroMonitorLogging::writeInfo(const __FlashStringHelper *str) {
  if (LOGLEVEL >= LOG_INFO) {
    bufferMsg_P((PGM_P)str);
    writeLog(LOG_INFO);
  }
}

/********************************************************************************************************************
 * Warning level log messages.
 */
void HydroMonitorLogging::writeWarning(const char *str) {
  if (LOGLEVEL >= LOG_WARNING) {
    bufferMsg(str);
    writeLog(LOG_WARNING);
  }
}

void HydroMonitorLogging::writeWarning(const __FlashStringHelper *str) {
  if (LOGLEVEL >= LOG_WARNING) {
    bufferMsg_P((PGM_P)str);
    writeLog(LOG_WARNING);
  }
}

/********************************************************************************************************************
 * Error level log messages.
 */
void HydroMonitorLogging::writeError(const char *str) {
  if (LOGLEVEL >= LOG_ERROR) {
    bufferMsg(str);
    writeLog(LOG_ERROR);
  }
}

void HydroMonitorLogging::writeError(const __FlashStringHelper *str) {
  if (LOGLEVEL >= LOG_ERROR) {
    bufferMsg_P((PGM_P)str);
    writeLog(LOG_ERROR);
  }
}

/********************************************************************************************************************
 * Functions to copy the message from PROGMEM to RAM for easier handling.
 */
void HydroMonitorLogging::bufferMsg_P(const char *str) {
  uint16_t size = strlen_P(str);
  if (size > MAX_MESSAGE_SIZE) {
    strncpy_P(msg, str, MAX_MESSAGE_SIZE - 3);
    strcat_P(msg, PSTR("..."));
  }
  else {
    strcpy_P(msg, str);
  }
}

void HydroMonitorLogging::bufferMsg(const char *str) {
  uint16_t size = strlen(str);
  if (size > MAX_MESSAGE_SIZE) {
    strncpy(msg, str, MAX_MESSAGE_SIZE - 3);
    strcat_P(msg, PSTR("..."));
  }
  else {
    strcpy(msg, str);
  }
}

/********************************************************************************************************************
 * The settings as HTML.
 */
void HydroMonitorLogging::settingsHtml(ESP8266WebServer *server) {

  // Check the validity of our locally stored credentials - this may differ from the global set if invalid & not stored to EEPROM.
  checkCredentials(settings.hostname, settings.hostpath, settings.username, settings.password);
  server->sendContent_P(PSTR("\
      <tr>\n\
        <th colspan=\"2\">Remote logging settings.</th>\n\
      </tr><tr>\n\
        <td>Database server host name:</td>\n\
        <td><input type=\"text\" name=\"database_hostname\" "));
  if (hostValid == VALID) {
    server->sendContent_P(PSTR("style=\"color:green\" "));
  }
  else if (hostValid == INVALID) {
    server->sendContent_P(PSTR("style=\"color:red\" "));
  }
  server->sendContent_P(PSTR("value=\""));
  if (strlen(settings.hostname) > 0) {
    server->sendContent(settings.hostname);
  }
  server->sendContent_P(PSTR("\"></td>\n\
      </tr><tr>\n\
        <td>Database server path:</td>\n\
        <td><input type=\"text\" name=\"database_hostpath\" "));
  if (pathValid == VALID) {
    server->sendContent_P(PSTR("style=\"color:green\" "));
  }
  else if (pathValid == INVALID) {
    server->sendContent_P(PSTR("style=\"color:red\" "));
  }
  server->sendContent_P(PSTR("value=\""));
  if (strlen(settings.hostpath) > 0) {
    server->sendContent(settings.hostpath);
  }
  server->sendContent_P(PSTR("\"></td>\n\
      </tr><tr>\n\
        <td>Database username:</td>\n\
        <td><input type=\"text\" name=\"database_username\" "));
  if (loginValid == VALID) {
    server->sendContent_P(PSTR("style=\"color:green\" "));
  }
  else if (loginValid == INVALID) {
    server->sendContent_P(PSTR("style=\"color:red\" "));
  }
  server->sendContent_P(PSTR("value=\""));
  if (strlen(settings.username) > 0) {
    server->sendContent(settings.username);
  }
  server->sendContent_P(PSTR("\"></td>\n\
      </tr><tr>\n\
        <td>Database password:</td>\n\
        <td><input type=\"text\" name=\"database_password\" "));
  if (loginValid == VALID) {
    server->sendContent_P(PSTR("style=\"color:green\" "));
  }
  else if (loginValid == INVALID) {
    server->sendContent_P(PSTR("style=\"color:red\" "));
  }
  server->sendContent_P(PSTR("value=\""));
  if (strlen(settings.password) > 0) {
    server->sendContent(settings.password);
  }
  server->sendContent_P(PSTR("\"></td>\n\
      </tr><tr>\n\
        <td></td>\n"));
  server->sendContent_P(PSTR("\
      </tr>"));
}

/********************************************************************************************************************
 * The settings as JSON.
 */
bool HydroMonitorLogging::settingsJSON(ESP8266WebServer* server) {

  // Check the validity of our locally stored credentials - this may differ from the global set if invalid & not stored to EEPROM.
  checkCredentials(settings.hostname, settings.hostpath, settings.username, settings.password);
  server->sendContent_P(PSTR("  \"logging\": {\n"
                             "    \"hostname\":\""));
  if (strlen(settings.hostname) > 0) {
    server->sendContent(settings.hostname);
  }
  server->sendContent_P(PSTR("\",\n"
                             "    \"hostpath\":\""));
  if (strlen(settings.hostpath) > 0) {
    server->sendContent(settings.hostpath);
  }
  server->sendContent_P(PSTR("\",\n"
                             "    \"username\":\""));
  if (strlen(settings.username) > 0) {
    server->sendContent(settings.username);
  }
  server->sendContent_P(PSTR("\",\n"
                             "    \"password\":\""));
  if (strlen(settings.password) > 0) {
    server->sendContent(settings.password);
  }
  server->sendContent_P(PSTR("\"\n"
                             "  }"));
  return true;
}

/********************************************************************************************************************
 * Update the settings.
 */
void HydroMonitorLogging::updateSettings(ESP8266WebServer* server) {
  char hostname[101];
  char hostpath[151];
  char username[33];
  char password[33];
  for (uint8_t i=0; i<server->args(); i++) {
    if (server->argName(i) == "database_hostname") {
      if (server->arg(i).length() <= 100) {
        server->arg(i).toCharArray(hostname, 100);
        hostname[server->arg(i).length()] = '\0';
      }
    }
    if (server->argName(i) == "database_hostpath") {
      if (server->arg(i).length() <= 150) {
        server->arg(i).toCharArray(hostpath, 150);
        hostpath[server->arg(i).length()] = '\0';
      }
    }
    if (server->argName(i) == "database_username") {
      if (server->arg(i).length() <= 32) {
        server->arg(i).toCharArray(username, 32);
        username[server->arg(i).length()] = '\0';
      }
    }
    if (server->argName(i) == "database_password") {
      if (server->arg(i).length() <= 32) {
        server->arg(i).toCharArray(password, 32);
        password[server->arg(i).length()] = '\0';
      }
    }
  }

  // If nothing changed, just keep the original settings as is.  
  if (strcmp(settings.hostname, hostname) == 0
      && strcmp(settings.hostpath, hostpath) == 0
      && strcmp(settings.username, username) == 0 
      && strcmp(settings.password, password) == 0) {
    return;
  }
  
  // Store new login data in the settings.
  strcpy(settings.hostname, hostname);
  strcpy(settings.hostpath, hostpath);
  strcpy(settings.username, username);
  strcpy(settings.password, password);

  // If any settings changed, check whether the login is valid. Only store the settings
  // in EEPROM if the new credentials are correct.
  checkCredentials();
  if (loginValid == VALID) {
#ifdef USE_24LC256_EEPROM
    sensorData->EEPROM->put(LOGGING_EEPROM, settings);
#else
    EEPROM.put(LOGGING_EEPROM, settings);
    EEPROM.commit();
#endif    
  }
}

/********************************************************************************************************************
 * Send out a GET request to post data.
 */
uint16_t HydroMonitorLogging::sendPostData(char* postData) {

  // Create an https client, and set it to ignore the certificate.
  // This is insecure: it allows for a MITM attack.
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;
  
  // Open a connection to the host.
  uint16_t responseCode;
  if (https.begin(*client, postData)) {                     // HTTPS connection.
    responseCode = https.GET();                             // start connection and send HTTP header
    if (responseCode > 0) {                                 // httpCode will be negative on error
      if (responseCode == HTTP_CODE_OK || responseCode == HTTP_CODE_MOVED_PERMANENTLY) { // File found at server
        String payload = https.getString();
      }
    } 
    https.end();
  } 
  return responseCode;    
}  

/********************************************************************************************************************
 * When calling this function:
 * status: contains the message number requested (0 = latest, 49 = oldest); returns the message type (log level).
 * timestamp: returns the message's time stamp.
 * message: returns the actual message.
 * The function returns a reference to message for convenience.
 */
char* HydroMonitorLogging::getLogMessage(uint8_t* status, uint32_t* timestamp, char* message) {
  if (*status > 49) {                                       // We don't have that many messages available...
    status = 0;
    timestamp = 0;
    strcpy(message, "");
  }
  else {
    File f = SPIFFS.open(messageLogFileName, "r");          // Open log file for read/write.
    f.seek(latestMessageList[*status] + 1, SeekSet);        // Start reading from the start of the requested record; skip byte 0 (transmission status).
    f.readBytes((char *)status, 1);                         // byte 1: message type (log level).
    f.readBytes((char *)timestamp, 4);
    uint16_t nBytes = f.readBytesUntil('\0', message, MAX_MESSAGE_SIZE - 1); // Get total size of the stored message.        
    message[nBytes] = 0;
    f.close();
  }
  return message;
}


/********************************************************************************************************************
 * Read the latest 50 messages and transmit them as JSON object.
 */
void HydroMonitorLogging::messagesJSON(ESP8266WebServer* server) {
  server->sendContent_P(PSTR("{\"messagelog\":\n"
                             "  {\n"));
  bool isFirst = true;
  char buff[10];
  uint8_t status;
  uint32_t timestamp;
  char message[MAX_MESSAGE_SIZE];
  for (uint8_t i = 0; i < 50; i++) {
    status = i;
    getLogMessage(&status, &timestamp, message);
    if (isFirst) {
      isFirst = false;
    }
    else {
      server->sendContent_P(PSTR(",\n"));
    }
    server->sendContent_P(PSTR("    \""));
    server->sendContent(itoa(i, buff, 10));
    server->sendContent_P(PSTR("\":[\""));
    server->sendContent(itoa(status, buff, 10));
    server->sendContent_P(PSTR("\",\""));
    server->sendContent(itoa(timestamp, buff, 10));
    server->sendContent_P(PSTR("\",\""));
    if (strlen(message) > 0) {
      server->sendContent(message);
    }
    server->sendContent_P(PSTR("\"]"));
  }
  server->sendContent_P(PSTR("\n  }\n"
                             "}"));
}

