#include <HydroMonitorpHSensor.h>

/*
 * The constructor.
 */
HydroMonitorpHSensor::HydroMonitorpHSensor() {
  calibratedSlope = 1;
  calibratedIntercept = 0;
  pH = -1;
  lastWarned = millis() - WARNING_INTERVAL;
}

#ifdef USE_PH_SENSOR
/*
 * Setup the sensor.
 * This function is for when the connector is connected to an external ADS1115 ADC.
 */
#ifdef PH_SENSOR_ADS_PIN
void HydroMonitorpHSensor::begin(HydroMonitorMySQL *l, Adafruit_ADS1115 *ads) {
  ads1115 = ads;
  l->writeTesting("HydroMonitorpHSensor: configured pH sensor on ADS port expander.");

/*
 * Setup the sensor.
 * This function is for when the connector is connected to the internal ADC.
 */
#elif defined(PH_SENSOR_PIN)
void HydroMonitorpHSensor::begin(HydroMonitorMySQL *l) {
  l->writeTesting("HydroMonitorpHSensor: configured pH sensor.");
#endif
  logging = l;
  if (PH_SENSOR_EEPROM > 0)
    EEPROM.get(PH_SENSOR_EEPROM, settings);
  
  // Read the calibration parameters.
  readCalibration();
  return;
}
#endif

/*
 * Calculate the calibration factors.
 */
void HydroMonitorpHSensor::readCalibration() {

  // Read the calibration parameters from EEPROM.
  core.readCalibration(PH_SENSOR_CALIBRATION_EEPROM, timestamp, value, reading, enabled);
  
  // Make sure only enabled datapoints are considered when calculating slope and intercept.
  uint32_t enabledReading[DATAPOINTS];
  for (int i=0; i<DATAPOINTS; i++) {
    if (enabled[i]) enabledReading[i] = reading[i];
    else enabledReading[i] = 0;
  }
  core.leastSquares(value, enabledReading, DATAPOINTS, &calibratedSlope, &calibratedIntercept);
  return;
}

/*
 * Read the pH value.
 */
float HydroMonitorpHSensor::readSensor(float waterTemp) {
  uint32_t reading = takeReading(waterTemp);
  pH = ((float)reading - calibratedIntercept)/calibratedSlope;

  // Send warning if it's been long enough ago & pH is > 1 point above target.
  if (millis() - lastWarned > WARNING_INTERVAL && pH > targetpH + 1) {
    lastWarned = millis();
    char message[110] = "pH level is too high; correction with pH adjuster is urgently needed.\nTarget set: ";
    char buf[5];
    snprintf(buf, 5, "%f", targetpH);
    strcat(message, buf);
    strcat(message, ", current pH: ");
    snprintf(buf, 5, "%f", pH);
    strcat(message, buf);
    strcat(message, ".");
    logging->sendWarning(message);
  }
  return pH;
}

/*
 * Read the pH value.
 * This version returns the raw, temperature corrected reading as int.
 */
uint32_t HydroMonitorpHSensor::takeReading(float waterTemp) {
  uint32_t reading;
  //TODO detect whether a sensor is present based on reading.
  
#ifdef PH_SENSOR_PIN
  
  // The analogRead of the built-in ADC is multipled by 32 to end up with a greater range,
  // and to have a higher precision after the temperature correction.
  reading = 32 * analogRead(PH_SENSOR_PIN);

#elif defined(PH_SENSOR_ADS_PIN)
  reading = ads1115->readADC_SingleEnded(PH_SENSOR_ADS_PIN);
#endif

  //TODO temperature correction.
  return reading;
}

void HydroMonitorpHSensor::setTargetpH(float p) {
  targetpH = p;
}

/*
 * The sensor settings as html.
 */
String HydroMonitorpHSensor::settingsHtml() {
  String html;
  html = F("\
      <tr>\n\
        <th colspan=\"2\">pH Sensor settings.</th>\n\
      </tr><tr>\n\
        <td></td>\n\
        <td><input type=\"submit\" formaction=\"/calibrate_ph\" formmethod=\"post\" name=\"calibrate\" value=\"Calibrate now\"></td>\n\
      </tr>\n");
  return html;
}


/*
 * The sensor settings as html.
 */
String HydroMonitorpHSensor::dataHtml() {
  String html = F("<tr>\n\
    <td>pH of the solution</td>\n\
    <td>");
  if (pH < 0) html += F("Sensor not connected.</td>\n\
  </tr>");
  else {
    html += String(pH);
    html += F(".</td>\n\
  </tr>");
  }
  return html;
}

/*
 * Get a list of past calibrations in html format.
 */
 //TODO implement this.
String HydroMonitorpHSensor::getCalibrationHtml() {
  return core.calibrationHtml("pH Sensor", "/calibrate_ph_action", timestamp, value, reading, enabled);
}

/*
 * Get a list of past calibrations in json format.
 */
 //TODO implement this.
String HydroMonitorpHSensor::getCalibrationData() {
  return core.calibrationData(timestamp, value, reading, enabled);
}

/*
 * Handle an html request for calibration, or a related function.
 */
void HydroMonitorpHSensor::doCalibration(ESP8266WebServer *server, float waterTemp) {

  // See whether it's a request to delete one of the calibrated values.
  if (server->hasArg("delete")) {
    String argVal = server->arg("delete");
    if (core.isNumeric(argVal)) {
      uint8_t val = argVal.toInt();
      if (val < DATAPOINTS) {
        timestamp[val] = 0;
        value[val] = 0;
        reading[val] = 0;
        enabled[val] = false;
      }
    }
  }
  
  // See whether it's a request to do a calibration.
  else if (server->hasArg("value")) {
    String argVal = server->arg("value");
    if (argVal != "") { // if there's a value given, use this to create a calibration point.
      if (core.isNumeric(argVal)) {
        float val = argVal.toFloat();
        uint16_t res = takeReading(val);
        
        // Find the first available data point where the value can be stored.
        // This is any data point where the timestamp = 0, regardless of it being
        // enabled or not.
        for (uint8_t i=0; i<DATAPOINTS; i++) {
          if (timestamp[i] == 0) {
            timestamp[i] = now();
            value[i] = val;
            reading[i] = res;
            enabled[i] = true;
            break;
          }
        }
      }
    }

    // Key "value" is present without parameter, and no other commands, so one of the checkboxes
    // was toggled. Set the enabled list accordingly.
    else {
      for (uint8_t i=0; i<DATAPOINTS; i++) {
        String key = "enable";
        key += i;
        if (server->hasArg(key)) enabled[i] = true;
        else enabled[i] = false;
      }
    }
  }
  // No more arguments to test for. Store the calibration.
  core.writeCalibration(PH_SENSOR_CALIBRATION_EEPROM, timestamp, value, reading, enabled);

  // Re-read the calibration values and update the EC probe parameters.
  readCalibration();
  return;
}
/*
 * Update the settings.
 */
void HydroMonitorpHSensor::updateSettings(String keys[], String values[], uint8_t nArgs) {
  return;
}
