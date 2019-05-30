#include <HydroMonitorLogging.h>

/*
   Take care of database connectivity (expects networking to be enabled).
*/


/*
   The constructor.
*/
HydroMonitorLogging::HydroMonitorLogging() {

}

/*
   Start the network services.
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

  // Set up the local storage (SPIFFS - store in flash).
  SPIFFS.begin();                                           // Initialse the SPIFFS storage.
  initMessageLogFile();                                     // Do this first to be able to properly receive new messages.
  initDataLogFile();
}

//*******************************************************************************************************************
// Initialise the data logging.
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
    if (f.size() != nRecords * fileRecordSize) {            // Basic sanity check.
      Serial.println(F("Data record file corrupt; creating a new one."));
      f.close();
      SPIFFS.remove(dataLogFileName);
      f = SPIFFS.open(dataLogFileName, "w");                // create a new one.
      f.close();
      dataRecordToTransmit = 0;
    }
    else {
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
      sprintf_P(buff, PSTR("HydroMonitorLogging: data points in this file: %d."), nRecords);
      writeTrace(buff);
    }
  }
  sprintf_P(buff, PSTR("HydroMonitorLogging: first unsent data point: #%d."), dataRecordToTransmit);
  writeTrace(buff);
  Serial.print(F("Sensor data logging is "));
  Serial.println((dataTransmitComplete) ? F("completed") : F("not completed."));
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
  else {                                                    // Data log exists already, figure out how far we were with transmission to the server.
    f = SPIFFS.open(messageLogFileName, "r");

    // Search file for the latest sent message.
    uint32_t i = 0;                                         // The search index.
    if (f.size() > 0) {                                     // We have at least one message, otherwise the file size is zero.
      uint16_t nBytes;
      uint8_t status;
      while (i < f.size()) {                                // Search the file until the end.
        f.seek(i, SeekSet);
        status = f.read();                                  // Read the message status byte.
        f.seek(i + 16, SeekSet);                            // Set search pointer to start of the logged message - skipping the header bytes.
        nBytes = f.readBytesUntil('\0', buff, MAX_MESSAGE_SIZE); // Get total size of the stored message.
        if ((status == RECORD_STORED || status == RECORD_TRANSMITTED) &&
            nBytes <= MAX_MESSAGE_SIZE) {                   // Record appears sound.
          nMessages++;                                      // Keep track of total number of messages we have stored.
          addMessageToList(i);                              // Keep track of the starting points of the last 50 messages.
          i += nBytes + 17;                                 // Set index to start of the next message (which starts nBytes + 16 header + 1 null terminator further).
          if (status == RECORD_TRANSMITTED) {               // This message has been transmitted already,
            messageToTransmit = i;                          // but maybe the next not yet. Set start index to that point.
          }
        }
        else {
          f.close();
          SPIFFS.remove(messageLogFileName);
          f = SPIFFS.open(messageLogFileName, "w");         // create a new one.
          f.close();
          nMessages = 0;
          messageToTransmit = 0;
          writeTrace(F("Message log file corrupt; removed this."));
          break;
        }
      }
    }
  }
  sprintf_P(buff, PSTR("HydroMonitorLogging: messages in this file: %d."), nMessages);
  writeTrace(buff);
  sprintf_P(buff, PSTR("HydroMonitorLogging: first unsent message starts at byte %d."), messageToTransmit);
  writeTrace(buff);
  if (messageToTransmit < f.size() &&
      f.size() > 0) {                                       // We have unsent messages.
    messageTransmitComplete = false;
    Serial.println(F("We have messages to transmit."));
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
   Take care of the actual logging - transmission and timing of new entries.
*/
void HydroMonitorLogging::logData() {

  // Every REFRESH_DATABASE milliseconds: log the sensor data, and try to transmit it to the database.
  if (millis() - lastLogSensorData > REFRESH_DATABASE) {
    lastLogSensorData += REFRESH_DATABASE;
    dataTransmitComplete = false;                           // We're adding a new record, so transmission is required.
    File f = SPIFFS.open(dataLogFileName, "a");             // Open the file, append mode.
    f.write(RECORD_STORED);                                 // First byte: status (it's merely stored at the moment).
    uint32_t timestamp = now();
    f.write(timestamp & 0xFF);                              // Bytes 1-4: the time stamp.
    f.write(timestamp >> 8) & 0xFF;
    f.write(timestamp >> 16) & 0xFF;
    f.write(timestamp >> 24);
    for (uint8_t i = 0; i < 11; i++) {
      f.write('\0');                                        // Fill the rest of the header with zeros.
    }
    memcpy(buff, sensorData, dataRecordSize);               // Make a copy for easier writing to the file.
    for (uint8_t i = 0; i < dataRecordSize; i++) {
      f.write(buff[i]);
    }
    Serial.print(F("New sensor data point logged. New data file size: "));
    Serial.println(f.size());
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
      for (uint32_t i = 0; i < nBytes; i++) {
        f.write(f1.read());
      }
      f1.close();
      for (uint8_t i = 0; i < 50; i++) {
        latestMessageList[i] = 0;
      }
    }
    f.close();
  }

  // Transmit messages & data - if we can do this now.
  static bool credentialsChecked = false;                   // We have to check credentials after WiFi is up.
  if (bitRead(sensorData->systemStatus, STATUS_WATERING) == false && // Don't do this while watering.
      WiFi.status() == WL_CONNECTED &&                      // We're connected to WiFi.
      millis() - lastSent > 1000) {                         // Wait at least a second before sending another record for responsiveness.
    if (credentialsChecked == false) {                      // We didn't check credentials yet.
      checkCredentials();                                   // Do this now.
      credentialsChecked = true;
    }
    if (loginValid != VALID) {                              // We need a valid login to be set.
      if (millis() - lastWarned > WARNING_INTERVAL) {       // Produce warnings now and then if this is not set.
        checkCredentials();                                 // Check again for good measure. You never know.
        if (loginValid != VALID) {                          // Still not? Produce the warning for real.
          writeWarning(F("Logging 01: can not transmit messages or data: database login invalid."));
          lastWarned = millis();
        }
      }
    }
    else if (connectionFailed) {                            // If the connection failed,
      if (millis() - connectionFailTime > CONNECTION_RETRY_DELAY) { // wait some time before trying again.
        connectionFailed = false;
      }
      if (millis() - lastWarned > WARNING_INTERVAL) {       // Produce warning if it's been long enough.
        writeWarning(F("Logging 02: can not transmit messages or data: connection failed."));
        lastWarned = millis();
      }
    }
    else {                                                  // Everything checked out; we can try to send messages and data now.
      if (dataTransmitComplete == false) {                  // We have data points to transmit.
        Serial.print(F("Going to transmit sensor data point #"));
        Serial.println(dataRecordToTransmit);
        transmitData();
      }
      else if (messageTransmitComplete == false) {          // We have messages to transmit.
        Serial.println(F("Going to transmit message."));
        transmitMessages();
      }
    }
  }
}

/********************************************************************************************************************
   Transmit a sensor data record to the server.
*/
void HydroMonitorLogging::transmitData() {
  File f = SPIFFS.open(dataLogFileName, "r+");              // Open log file for read/write.

  // All data is stored already in the logfile; read back the data to transmit, attempt to transmit it, and if
  // successful mark the record as transmitted.
  HydroMonitorCore::SensorData dataEntry;
  Serial.print(F("Sensor data record start byte: "));
  Serial.print(dataRecordToTransmit * fileRecordSize);
  Serial.print(F(", end byte: "));
  Serial.print(dataRecordToTransmit * fileRecordSize + fileRecordSize);
  Serial.print(F(", file size: "));
  Serial.println(f.size());
  f.seek(dataRecordToTransmit * fileRecordSize, SeekSet);   // Start reading from the start of the next record we have to transmit.
  f.readBytes(buff, fileRecordSize);
  uint32_t timestamp;
  memcpy(&timestamp, buff + 1, 4);                          // Skip the record's byte 0, the status byte.
  memcpy(&dataEntry, buff + 16, dataRecordSize);            // Skip the record's 16-byte header.
  uint16_t size = 120 + strlen(settings.hostname) + strlen(settings.hostpath) + strlen(settings.username) + strlen(settings.password);
  char postData[size];
  Serial.print(F("Sensor data postData buffer size: "));
  Serial.println(size);
#ifdef USE_WATERLEVEL_SENSOR
  sprintf_P(postData, PSTR("http://%s%s?username=%s&password=%s&ec=%4.2f&watertemp=%4.2f&waterlevel=%4.2f&ph=%4.2f&timestamp=%u"),
            settings.hostname, settings.hostpath, settings.username, settings.password,
            dataEntry.EC, dataEntry.waterTemp, dataEntry.waterLevel, dataEntry.pH,
            timestamp);
#else
  sprintf_P(postData, PSTR("http://%s%s?username=%s&password=%s&ec=%4.2f&watertemp=%4.2f&waterlevel=%4.2f&ph=%4.2f&timestamp=%u"),
            settings.hostname, settings.hostpath, settings.username, settings.password,
            dataEntry.EC, dataEntry.waterTemp, 0, dataEntry.pH,
            timestamp);
#endif

  uint16_t httpCode = sendPostData(postData);
  if (httpCode == 200) {                                    // 200 = OK, transmissions successful.
    uint16_t seekPointer = dataRecordToTransmit * fileRecordSize;
    f.seek(dataRecordToTransmit * fileRecordSize, SeekSet);
    f.write(RECORD_TRANSMITTED);                            // Mark file entry as transmitted.
    dataRecordToTransmit++;                                 // Proceed to next entry.
    if (f.size() == dataRecordToTransmit * fileRecordSize) {
      dataTransmitComplete = true;
      Serial.println(F("Sensor data transmission completed."));
    }
    else if (f.size() > dataRecordToTransmit * fileRecordSize) { // This should never happen, yet it does...
      Serial.println(F("Data record file corrupt; creating a new one."));
      Serial.print(F("Data file size: "));
      Serial.print(f.size());
      Serial.print(F(", expected size: "));
      Serial.println(dataRecordToTransmit * fileRecordSize);
      f.close();
      SPIFFS.remove(dataLogFileName);
      f = SPIFFS.open(dataLogFileName, "w");                // create a new one.
      f.close();
      dataRecordToTransmit = 0;
    }
    else {
      dataTransmitComplete = false;
      Serial.print(F("File size: "));
      Serial.print(f.size());
      Serial.print(F(", next data point starts at: "));
      Serial.println(dataRecordToTransmit * fileRecordSize);
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
   Transmit a message record to the server.
*/
void HydroMonitorLogging::transmitMessages() {
  File f = SPIFFS.open(messageLogFileName, "r+");           // Open log file for read/write.
  f.seek(messageToTransmit, SeekSet);                       // Set seek pointer to start of the next message.
  f.readBytes(control, 16);                                 // The control bytes.
  uint16_t nBytes = f.readBytesUntil('\0', buff, MAX_MESSAGE_SIZE); // The actual message.
  buff[nBytes] = 0;                                         // Null terminator.
  uint32_t timestamp;
  memcpy(&timestamp, control + 2, 4);                       // The message's time stamp.
  uint8_t loglevel = control[1];                            // The log level.
  String encodedMessage = core.urlencode(buff);             // URLencode the message.
  uint16_t size = 80 + strlen(settings.hostname) + strlen(settings.hostpath) + strlen(settings.username) + strlen(settings.password) + encodedMessage.length();
  char postData[size];                                      // Create a buffer to hold the complete POST data.
  char aLoglevel[2];
  itoa(loglevel, aLoglevel, 10);
  char aTimestamp[12];
  itoa(timestamp, aTimestamp, 10);
  Serial.print(F("Message postData buffer size: "));
  Serial.println(size);
  sprintf_P(postData, PSTR("http://%s%s?username=%s&password=%s&loglevel=%s&message=%s&timestamp=%s"),
            settings.hostname, settings.hostpath, settings.username, settings.password, aLoglevel, encodedMessage.c_str(), aTimestamp);
  uint16_t httpCode = sendPostData(postData);               // Post the message to the database.
  if (httpCode == 200) {                                    // 200 = OK, transmissions successful.
    f.seek(messageToTransmit, SeekSet);                     // Set seek pointer back to the start of this message: the status byte.
    f.write(RECORD_TRANSMITTED);                            // Mark file entry as transmitted.
    messageToTransmit += nBytes + 17;                       // Proceed to next entry.
    if (f.size() == messageToTransmit) {                    // We reached the end of the file - transmission completed.
      messageTransmitComplete = true;
      Serial.println(F("Message transmission completed."));
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
   Check a set of login credentials.

   Returns true if valid; false if invalid.
*/
void HydroMonitorLogging::checkCredentials() {
  checkCredentials(settings.hostname, settings.hostpath, settings.username, settings.password);
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
  sprintf_P(postData, PSTR("http://%s%s?username=%s&password=%s&validate=1"),
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
   Write log message to the local file.
*/
void HydroMonitorLogging::writeLog(uint8_t loglevel) {

#if defined(LOG_SERIAL) && defined(SERIAL)
  Serial.println(buff);
#endif

  // Store message to the message log file.
  File f = SPIFFS.open(messageLogFileName, "a");            // Open log file for appending.
  addMessageToList(f.size());                               // New message starts at the end of the current file.
  f.write(RECORD_STORED);                                   // Byte 0: message status.
  f.write(loglevel);                                        // Byte 1: log level.
  uint16_t size = strlen(buff) + 1;                         // Message size should include the null terminator.
  uint32_t timestamp = now();

  f.write(timestamp & 0xFF);                                // Bytes 2-5: the time stamp.
  f.write(timestamp >> 8) & 0xFF;
  f.write(timestamp >> 16) & 0xFF;
  f.write(timestamp >> 24);

  for (uint8_t i = 0; i < 10; i++) {                        // Bytes 6-15: reserved.
    f.write(0xFF);
  }
  f.print(buff);                                            // Print the whole message to the file.
  f.write(0);                                               // Add the null terminator to complete the record.
  f.close();

  // Check log file size, and if needed roll over into a new file. Copy latest 50 messages to the new log file.
  File fOld = SPIFFS.open(messageLogFileName, "r");
  if (fOld.size() > MAX_FILE_SIZE) {
    Serial.println(fOld.size());
    fOld.close();
    if (SPIFFS.exists(messageLogFile1Name)) {               // Check on the rollover file; if it exists, remove it.
      SPIFFS.remove(messageLogFile1Name);
    }
    SPIFFS.rename(messageLogFileName, messageLogFile1Name); // Rename the original log to the rollover file.
    File fNew = SPIFFS.open(messageLogFileName, "w");       // Create a new logfile.
    fOld = SPIFFS.open(messageLogFile1Name, "r");           // Open the old one for reading back the latest 50 messages.
    uint32_t idx = latestMessageList[49];                   // Starting point of oldest message in the list is at index 49.
    for (uint8_t i = 0; i < 50; i++) {
      latestMessageList[i] -= idx;                          // Set the list to point to where the messages are going to be in the new file.
    }
    uint16_t bytesToRead = fOld.size() - idx;               // Calculate how many bytes we have to copy.
    fOld.seek(idx, SeekSet);                                // Start copying from here.
    while (idx < fOld.size() - MAX_MESSAGE_SIZE) {          // Copy MAX_MESSAGE_SIZE bytes at a time - that fits in buff - until less than that left in the file.
      fOld.readBytes(buff, MAX_MESSAGE_SIZE);
      fNew.write(buff, MAX_MESSAGE_SIZE);
      idx += MAX_MESSAGE_SIZE;
    }
    uint16_t lastBytes = fOld.size() - idx;                 // Calculate the remainder of bytes to copy.
    fOld.readBytes(buff, lastBytes);
    fNew.write(buff, lastBytes);
    fNew.close();
  }
  fOld.close();
  messageTransmitComplete = false;                          // Because we just added a new one!
}

/********************************************************************************************************************
   Trace level log messages.
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
   Info level log messages.
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
   Warning level log messages.
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
   Error level log messages.
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
   Functions to copy the message from PROGMEM to RAM for easier handling.
*/
void HydroMonitorLogging::bufferMsg_P(const char *str) {
  uint16_t size = strlen_P(str);
  if (size > MAX_MESSAGE_SIZE) {
    strncpy_P(buff, str, MAX_MESSAGE_SIZE - 3);
    strcat_P(buff, PSTR("..."));
  }
  else {
    strcpy_P(buff, str);
  }
}

void HydroMonitorLogging::bufferMsg(const char *str) {
  uint16_t size = strlen(str);
  if (size > MAX_MESSAGE_SIZE) {
    strncpy(buff, str, MAX_MESSAGE_SIZE - 3);
    strcat_P(buff, PSTR("..."));
  }
  else {
    strcpy(buff, str);
  }
}

/********************************************************************************************************************
   The settings as HTML.
*/
void HydroMonitorLogging::settingsHtml(ESP8266WebServer *server) {

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
   The settings as JSON.
*/
bool HydroMonitorLogging::settingsJSON(ESP8266WebServer* server) {

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
   Update the settings.
*/
void HydroMonitorLogging::updateSettings(ESP8266WebServer* server) {
  char hostname[101];
  char hostpath[151];
  char username[33];
  char password[33];
  for (uint8_t i = 0; i < server->args(); i++) {
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
   Send out a GET request to post data.
*/
uint16_t HydroMonitorLogging::sendPostData(char* postData) {

  //  return 500;                                               // Disable the transmission of logging data for now.


  //  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  //  client.setInsecure();                                    // Do not check for https fingerprints (insecure: allows MiM attacks).

  //  WiFiClientSecure client;
  //  client.setInsecure();                                     // Do not check for https fingerprints (insecure: allows MiM attacks).
  //  HTTPClient https;

  WiFiClient client;
  HTTPClient http;

  // Open a connection to the host.
  uint16_t responseCode;
  uint32_t startTransmission = millis();
  Serial.print(F("Starting transmission of "));
  Serial.print(strlen(postData));
  Serial.print(F(" bytes: "));
  Serial.println(postData);
  if (http.begin(client, postData)) {                       // HTTP connection.
    Serial.println(F("Connected."));
    responseCode = http.GET();                              // start connection and send HTTP header
    Serial.println(F("Got the GET request result."));
    if (responseCode > 0) {                                 // httpCode will be negative on error
      if (responseCode == HTTP_CODE_OK || responseCode == HTTP_CODE_MOVED_PERMANENTLY) { // File found at server
        String payload = http.getString();
      }
    }
  }
  http.end();
  client.stop();
  Serial.print(F("Transmission complete. Response code: "));
  Serial.print(responseCode);
  Serial.print(F(" Time taken: "));
  Serial.print(millis() - startTransmission);
  Serial.println(F(" ms."));
  return responseCode;
}

/********************************************************************************************************************
   When calling this function:
   status: contains the message number requested (0 = latest, 49 = oldest); returns the message type (log level).
   timestamp: returns the message's time stamp.
   message: returns the actual message.
   The function returns a reference to message for convenience.
*/
void HydroMonitorLogging::getLogMessage(uint8_t* status, uint32_t* timestamp) {
  if (*status > 49) {                                       // We don't have that many messages available...
    status = 0;
    timestamp = 0;
    strcpy(buff, "");
  }
  else {
    File f = SPIFFS.open(messageLogFileName, "r");          // Open log file for reading.
    f.seek(latestMessageList[*status] + 16, SeekSet);       // Set seek pointer to the start of the message.
    uint16_t nBytes = f.readBytesUntil('\0', buff, MAX_MESSAGE_SIZE); // Copy the stored message into global buffer buff,
    buff[nBytes] = 0;                                       // and top it off with a null terminator.
    f.seek(latestMessageList[*status] + 1, SeekSet);        // Start reading control bytes from the start of the requested record; skip byte 0 (transmission status).
    f.readBytes((char *)status, 1);                         // Byte 1: message type (log level).
    f.readBytes((char *)timestamp, 4);                      // Byte 2-6: timestamp.
    f.close();
  }
}


/********************************************************************************************************************
   Read the latest 50 messages and transmit them as JSON object.
*/
void HydroMonitorLogging::messagesJSON(ESP8266WebServer* server) {
  server->sendContent_P(PSTR("{\"messagelog\":\n"
                             "  {\n"));
  bool isFirst = true;
  uint8_t status;
  uint32_t timestamp;
  for (uint8_t i = 0; i < 50; i++) {
    status = i;
    getLogMessage(&status, &timestamp);                     // Places the message in the global buff.
    if (isFirst) {
      isFirst = false;
    }
    else {
      server->sendContent_P(PSTR(",\n"));
    }
    server->sendContent_P(PSTR("    \""));
    server->sendContent(itoa(i, control, 10));
    server->sendContent_P(PSTR("\":[\""));
    server->sendContent(itoa(status, control, 10));
    server->sendContent_P(PSTR("\",\""));
    server->sendContent(itoa(timestamp, control, 10));
    server->sendContent_P(PSTR("\",\""));
    if (strlen(buff) > 0) {
      server->sendContent(buff);
    }
    server->sendContent_P(PSTR("\"]"));
  }
  server->sendContent_P(PSTR("\n  }\n"
                             "}"));
}

