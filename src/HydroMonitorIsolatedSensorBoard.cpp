#include <HydroMonitorIsolatedSensorBoard.h>

#ifdef USE_ISOLATED_SENSOR_BOARD

HydroMonitorIsolatedSensorBoard::HydroMonitorIsolatedSensorBoard () { 
}

/*
 * Set up the sensor.
 */
void HydroMonitorIsolatedSensorBoard::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l, SoftwareSerial *s) {
  logging = l;
  logging->writeTrace(F("HydroMonitorIsolatedSensorBoard: configured isolated sensor board."));
  sensorData = sd;
  sensorSerial = s;
  if (ISOLATED_SENSOR_BOARD_EEPROM > 0) {
#ifdef USE_24LC256_EEPROM
    sensorData->EEPROM->get(ISOLATED_SENSOR_BOARD_EEPROM, settings);
#else
    EEPROM.get(ISOLATED_SENSOR_BOARD_EEPROM, settings);
#endif
  }
  readingState = READING_IDLE;
  count = 0;
  return;
}

/*
 * Take a measurement from the sensor.
 * As this is a Serial input this one should be called frequently.
 */ 
void HydroMonitorIsolatedSensorBoard::readSensor() {
  if (sensorSerial->available()) {
    char c = sensorSerial->read();
    Serial.print(c);
    switch (readingState) {
      case READING_IDLE:                                    // Wait for the start tag for reading. Ignore any other characters at this point.
        if (c == '<') {                                     // Start tag - start reading the first value.
          readingState = READING_VALUE;
        }
        break;
      
      case READING_VALUE:                                   // Wait for the next character: this should tell which value we're going to get.
        count = 0;
        if (c == 'T') {                                     // T for temperature (in deg C *10).
          readingState = READING_TEMPERATURE;
        }
        if (c == 'E') {                                     // E for EC (as discharge cycles).
          readingState = READING_EC;
        }
        if (c == 'P') {                                     // P for pH (as raw analog reading).
          readingState = READING_PH;
        }
        break;
      
      case READING_TEMPERATURE:                             // We expect a number from 0 to 1000 (0 deg C to 100.0 deg C)
        if (c >= '0' && c <= '9') {
          buffer[count] = c;
          count++;
          if (count > 4) {                                  // Max number size should be 4 digits.
            readingState = READING_IDLE;
          }
        }
        else if (c == ',' || c == '>') {                    // Complete number received.
          buffer[count] = 0;                                // null termination for the string in the buffer.
#ifndef USE_DS18B20
          sensorData->waterTemp = atoi(buffer) / 10.0;
#endif
          if (c == ',') {                                   // Another value is coming.
            readingState = READING_VALUE;                   // Start reading.
          }
          else {                                            // It was an end tag.
            readingState = READING_IDLE;                    // Wait for the next communication.
          }
        }
        else {
          Serial.print(F("Invalid character reading temperature, value: "));
          Serial.println((uint8_t) c);
          readingState = READING_IDLE;                      // An invalid character was received; wait for the next communcication to start.
        }
        break;
      
      case READING_EC:
        if (c >= '0' && c <= '9') {
          buffer[count] = c;
          count++;
          if (count > 5) {                                  // Max number should be 5 digits.
            readingState = READING_IDLE;
          }
        }
        else if (c == ',' || c == '>') {                    // Complete number received.
          buffer[count] = 0;                                // null termination.
          sensorData->ecReading = atoi(buffer);
          if (c == ',') {                                   // Another value is coming.
            readingState = READING_VALUE;                   // Start reading.
          }
          else {                                            // It was an end tag.
            readingState = READING_IDLE;                    // Wait for the next communication.
          }
        }
        else {
          Serial.print(F("Invalid character reading EC, value: "));
          Serial.println((uint8_t) c);
          readingState = READING_IDLE;                      // An invalid character was received; wait for the next communcication to start.
        }
        break;
      
      case READING_PH:
        if (c >= '0' && c <= '9') {
          buffer[count] = c;
          count++;
          if (count > 5) {                                  // Max number should be 5 digits.
            readingState = READING_IDLE;
          }
        }
        else if (c == ',' || c == '>') {                    // Complete number received.
          buffer[count] = 0;                                // null termination.
          sensorData->phReading = atoi(buffer);
          if (c == ',') {                                   // Another value is coming.
            readingState = READING_VALUE;                   // Start reading.
          }
          else {                                            // It was an end tag.
            readingState = READING_IDLE;                    // Wait for the next communication.
          }
        }
        else {
          readingState = READING_IDLE;                      // An invalid character was received; wait for the next communcication to start.
          Serial.print(F("Invalid character reading pH, value: "));
          Serial.println((uint8_t) c);
        }
        break;
    }
  }  
}

/*
 * The sensor settings as html.
 */
void HydroMonitorIsolatedSensorBoard::settingsHtml(ESP8266WebServer *server) {
}

/*
 * The sensor data as JSON.
 */
bool HydroMonitorIsolatedSensorBoard::settingsJSON(ESP8266WebServer *server) {
  return false;                                             // none.
}

/*
 * The sensor data as html.
 */
void HydroMonitorIsolatedSensorBoard::dataHtml(ESP8266WebServer *server) {
}

/*
 * Update the settings for this sensor, if any.
 */
void HydroMonitorIsolatedSensorBoard::updateSettings(ESP8266WebServer* server) {
}
#endif
