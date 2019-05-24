#include <HydroMonitorpHSensor.h>

#ifdef USE_PH_SENSOR
/*
 * The constructor.
 */
HydroMonitorpHSensor::HydroMonitorpHSensor() {
  calibratedSlope = 1;
  calibratedIntercept = 0;
  lastWarned = millis() - WARNING_INTERVAL;
}

/*
 * Setup the sensor.
 * This function is for when the connector is connected to an external ADS1115 ADC.
 */
#ifdef PH_SENSOR_ADS_PIN
#ifdef PH_POS_MCP_PIN
void HydroMonitorpHSensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l, Adafruit_ADS1115 *ads, Adafruit_MCP23008 *mcp) {
#else
void HydroMonitorpHSensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l, Adafruit_ADS1115 *ads) {
#endif
  ads1115 = ads;
  l->writeTrace(F("HydroMonitorpHSensor: configured pH sensor on ADS port expander."));

/*
 * Setup the sensor.
 * This function is for when the connector is connected to the internal ADC.
 */
#elif defined(PH_SENSOR_PIN)
#ifdef PH_POS_MCP_PIN
void HydroMonitorpHSensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l, Adafruit_MCP23008 *mcp) {
#else
void HydroMonitorpHSensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l) {
#endif
  l->writeTrace(F("HydroMonitorpHSensor: configured pH sensor."));
#endif
  sensorData = sd;
  logging = l;
  
#ifdef PH_POS_MCP_PIN
  mcp23008 = mcp;
  mcp23008->pinMode(PH_POS_MCP_PIN, OUTPUT);
  mcp23008->pinMode(PH_GND_MCP_PIN, OUTPUT);
  mcp23008->digitalWrite(PH_POS_MCP_PIN, HIGH);
  mcp23008->digitalWrite(PH_GND_MCP_PIN, LOW);
#elif defined (PH_POS_PIN)
  pinMode(PH_POS_PIN, OUTPUT);
  pinMode(PH_GND_PIN, OUTPUT);
  digitalWrite(PH_POS_PIN, HIGH);
  digitalWrite(PH_GND_PIN, LOW);
#endif
  if (PH_SENSOR_EEPROM > 0)
#ifdef USE_24LC256_EEPROM
    sensorData->EEPROM->get(PH_SENSOR_EEPROM, settings);
#else
    EEPROM.get(PH_SENSOR_EEPROM, settings);
#endif  
  // Read the calibration parameters.
  readCalibration();
  return;
}

/*
 * Calculate the calibration factors.
 */
void HydroMonitorpHSensor::readCalibration() {

  // Retrieve the calibration data, and calculate the slope and intercept.
#ifdef USE_24LC256_EEPROM
  sensorData->EEPROM->get(PH_SENSOR_CALIBRATION_EEPROM, calibrationData);
#else
  EEPROM.get(PH_SENSOR_CALIBRATION_EEPROM, calibrationData);
#endif
  
  // Make sure only enabled datapoints are considered when calculating slope and intercept.
  uint32_t usedReadings[DATAPOINTS];
  float enabledValue[DATAPOINTS];
  uint8_t nPoints = 0;
  for (int i=0; i<DATAPOINTS; i++) {
    if (calibrationData[i].enabled) {
      enabledValue[nPoints] = calibrationData[i].value;
      usedReadings[nPoints] = calibrationData[i].reading;
      nPoints++;
    }
  }
  core.leastSquares(enabledValue, usedReadings, nPoints, &calibratedSlope, &calibratedIntercept);
  return;
}

/*
 * Read the pH value.
 */
void HydroMonitorpHSensor::readSensor(bool readNow) {
  static uint32_t lastReadSensor = -REFRESH_SENSORS;
  if (millis() - lastReadSensor > REFRESH_SENSORS ||
      readNow) {
    lastReadSensor = millis();
    uint32_t reading = takeReading();
    sensorData->pH = ((float)reading - calibratedIntercept)/calibratedSlope;
    if (sensorData->pH > 15) {    // Impossible value! Sensor not connected or calibration not done.
      sensorData->pH = -1;
    }

    // Send warning if it's been long enough ago & pH is > 1 point above target.
    if (millis() - lastWarned > WARNING_INTERVAL) {
      if (sensorData->pH > 9 || sensorData->pH < 4) {
        lastWarned = millis();
        char message[110];
        sprintf_P(message, PSTR("pHSensor 01: unusual pH level measured: %2.2f. Check sensor."), sensorData->pH);
        logging->writeWarning(message);
      }
      else if (sensorData->pH > sensorData->targetpH + 1) {
        lastWarned = millis();
        char message[120];
        sprintf_P(message, PSTR("pHSensor 02: pH level is too high; correction with pH adjuster is urgently needed.\n"
                                "Target set: %2.2f, current pH: %2.2f."), sensorData->targetpH, sensorData->pH);
        logging->writeWarning(message);
      }
    }
  }
  return;
}

/*
 * Read the pH value.
 * This version returns the raw, temperature corrected reading as int.
 */
uint32_t HydroMonitorpHSensor::takeReading() {
  uint32_t reading;
  //TODO detect whether a sensor is present based on reading.

#ifdef USE_ISOLATED_SENSOR_BOARD
  reading = sensorData->phReading;
#elif defined(PH_SENSOR_PIN)
  // The analogRead of the built-in ADC is multipled by 32 to end up with a greater range,
  // and to have a higher precision after the temperature correction.
  reading = 32 * analogRead(PH_SENSOR_PIN);
#elif defined(PH_SENSOR_ADS_PIN)
  reading = ads1115->readADC_SingleEnded(PH_SENSOR_ADS_PIN);
#endif

  //TODO temperature correction.
  return reading;
}

/*
 * The sensor settings as html.
 */
void HydroMonitorpHSensor::settingsHtml(ESP8266WebServer *server) {
  server->sendContent_P(PSTR("\
      <tr>\n\
        <th colspan=\"2\">pH Sensor settings.</th>\n\
      </tr><tr>\n\
        <td></td>\n\
        <td><input type=\"submit\" formaction=\"/calibrate_ph\" formmethod=\"post\" name=\"calibrate\" value=\"Calibrate now\"></td>\n\
      </tr>\n"));
}

/*
 * The sensor settings as JSON.
 */
bool HydroMonitorpHSensor::settingsJSON(ESP8266WebServer* server) {
  return false; // None.
}

/*
 * The sensor settings as html.
 */
void HydroMonitorpHSensor::dataHtml(ESP8266WebServer *server) {
  server->sendContent_P(PSTR("<tr>\n\
    <td>pH of the solution</td>\n\
    <td>"));
  if (sensorData->pH < 0) {
    server->sendContent_P(PSTR("Sensor not connected.</td>\n\
  </tr>"));
  }
  else {
    char buff[10];
    sprintf(buff, "%.2f", sensorData->pH);
    server->sendContent(buff);
    server->sendContent_P(PSTR(".</td>\n\
  </tr>"));
  }
}

/*
 * Get a list of past calibrations in html format.
 */
void HydroMonitorpHSensor::getCalibrationHtml(ESP8266WebServer *server) {
  core.calibrationHtml(server, "pH Sensor", "/calibrate_ph_action", calibrationData);
}

/*
 * Get a list of past calibrations in json format.
 */
void HydroMonitorpHSensor::getCalibration(ESP8266WebServer *server) {
  core.calibrationData(server, calibrationData);
}

void HydroMonitorpHSensor::doCalibrationAction(ESP8266WebServer *server) {
 if (server->hasArg(F("calibrate"))) {
   doCalibration(server);
 }
 else if (server->hasArg(F("delete"))) {
   deleteCalibration(server);
 }
 else {
   enableCalibration(server);
 }
}

/*
 * Handle the calibration of the sensor.
 */
void HydroMonitorpHSensor::doCalibration(ESP8266WebServer *server) {
  if (server->hasArg("value")) {
    String argVal = server->arg("value");
    if (argVal != "") { // if there's a value given, use this to create a calibration point.
      if (core.isNumeric(argVal)) {
        float val = argVal.toFloat();
        uint16_t res = takeReading();
        
        // Find the first available data point where the value can be stored.
        // This is any data point where the timestamp = 0, regardless of it being
        // enabled or not.
        for (uint8_t i=0; i<DATAPOINTS; i++) {
          if (calibrationData[i].timestamp == 0) {
            calibrationData[i].timestamp = now();
            calibrationData[i].value = val;
            calibrationData[i].reading = res;
            calibrationData[i].enabled = true;
            break;
          }
        }
        saveCalibrationData();
      }
    }
  }
}

/*
 * Enable/disable calibration points.
 */

void HydroMonitorpHSensor::enableCalibration(ESP8266WebServer *server) {
  for (uint8_t i=0; i<DATAPOINTS; i++) {
    String key = "enable";
    key += i;
    calibrationData[i].enabled = server->hasArg(key);
  }
  saveCalibrationData();
}

/*
 * Delete calibration points.
 */
void HydroMonitorpHSensor::deleteCalibration(ESP8266WebServer *server) {
  if (server->hasArg("delete")) {
    String argVal = server->arg("delete");
    if (core.isNumeric(argVal)) {
      uint8_t val = argVal.toInt();
      if (val < DATAPOINTS) {
        calibrationData[val].timestamp = 0;
        calibrationData[val].value = 0;
        calibrationData[val].reading = 0;
        calibrationData[val].enabled = false;
      }
      saveCalibrationData();
    }
  }
}  
  
void HydroMonitorpHSensor::saveCalibrationData() {

  // Store the calibration in EEPROM.
#ifdef USE_24LC256_EEPROM
  sensorData->EEPROM->put(PH_SENSOR_CALIBRATION_EEPROM, calibrationData);
#else
  EEPROM.put(PH_SENSOR_CALIBRATION_EEPROM, calibrationData);
#endif
  // Re-read the calibration values and update the EC probe parameters.
  readCalibration();
  return;
}

/*
 * Update the settings.
 */
void HydroMonitorpHSensor::updateSettings(ESP8266WebServer* server) {
  return;
}
#endif

