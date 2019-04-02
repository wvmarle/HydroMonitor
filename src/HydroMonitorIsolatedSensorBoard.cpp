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
    switch (readingState) {
      case READING_IDLE:
        if (c == '<') {
          readingState = READING_VALUE;
        }
      break;
      
      case READING_VALUE:
        count = 0;
        if (c == 'T') {
          readingState = READING_TEMPERATURE;
        }
        if (c == 'E') {
          readingState = READING_EC;
        }
        if (c == 'P') {
          readingState = READING_PH;
        }
      
      case READING_TEMPERATURE:
        if (c >= '0' && c <= '9') {
          buffer[count] = c;
          count++;
          if (count > 4) {  // Max number should be 4 digits.
            readingState = READING_IDLE;
          }
        }
        else if (c == ',' || c == '>') { // Complete number received.
          buffer[count] = 0; // null termination.
#ifndef USE_DS18B20
          sensorData->waterTemp = atoi(buffer) / 10.0;
#endif
          if (c == ',') {
            readingState = READING_VALUE;
          }
          else {
            readingState = READING_IDLE;
          }
        }
        break;
      
      case READING_EC:
        if (c >= '0' && c <= '9') {
          buffer[count] = c;
          count++;
          if (count > 5) {  // Max number should be 5 digits.
            readingState = READING_IDLE;
          }
        }
        else if (c == ',' || c == '>') { // Complete number received.
          buffer[count] = 0; // null termination.
          sensorData->ecReading = atoi(buffer);
          if (c == ',') {
            readingState = READING_VALUE;
          }
          else {
            readingState = READING_IDLE;
          }
        }
        break;      
      break;
      
      case READING_PH:
        if (c >= '0' && c <= '9') {
          buffer[count] = c;
          count++;
          if (count > 5) {  // Max number should be 5 digits.
            readingState = READING_IDLE;
          }
        }
        else if (c == ',' || c == '>') { // Complete number received.
          buffer[count] = 0; // null termination.
          sensorData->phReading = atoi(buffer);
          if (c == ',') {
            readingState = READING_VALUE;
          }
          else {
            readingState = READING_IDLE;
          }
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
  return false; // none.
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
  return;
}
#endif
