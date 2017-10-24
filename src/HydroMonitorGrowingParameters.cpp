#include <HydroMonitorGrowingParameters.h>

/*
 * Add fertiliser to the system as needed, based on the current EC value.
 *
 * Constructor.
 */
HydroMonitorGrowingParameters::HydroMonitorGrowingParameters() {
}

/*
 * Configure the module.
 */
void HydroMonitorGrowingParameters::begin(HydroMonitorCore::SensorData *sd, HydroMonitorMySQL *l) {
  sensorData = sd;
  logging = l;
  if (GROWING_PARAMETERS_EEPROM > 0)
    EEPROM.get(FERTILISER_EEPROM, settings);

  // Check whether any settings have been set, if not apply defaults.
  if (settings.fertiliserConcentration > 500) {
    logging->writeDebug("HydroMonitorGrowingParameters: applying default settings.");
    settings.fertiliserConcentration = 200;
    settings.solutionVolume = 100;
    settings.targetEC = 0;
    settings.pHMinusConcentration = 0.05;  // ml of pH-minus per litre of solution for a 1 pH point change.
    settings.targetpH = 0;
    strcpy(settings.systemName, "HydroMonitor");
    EEPROM.put(FERTILISER_EEPROM, settings);
    EEPROM.commit();
  }
  logging->writeInfo("HydroMonitorGrowingParameters: set up all the growing parameters.");
  return;
}

/*
 * The html code for the sensor specific settings.
 */
String HydroMonitorGrowingParameters::settingsHtml() {
  String html;
  html = F("\
      <tr>\n\
        <th colspan=\"2\">General growing and system parameters.</th>\n\
      </tr><tr>\n\
        <td>Total volume of the system:</td>\n\
        <td><input type=\"number\" step=\"1\" name=\"parameter_solutionvolume\" value=\"");
  html += String(settings.solutionVolume);
  html += F("\"> litres</td>\n\
      </tr><tr>\n\
        <td>Fertiliser concentration factor:</td>\n\
        <td>1 : <input type=\"number\" step=\"1\" name=\"parameter_fertiliser_concentration\" value=\"");
  html += String(settings.fertiliserConcentration);
  html += F("\"></td>\n\
      </tr><tr>\n\
        <td>Target EC of the solution:</td>\n\
        <td><input type=\"number\" step=\"0.1\" name=\"parameter_targetec\" value=\"");
  html += String(settings.targetEC);
  html += F("\"> mS/cm</td>\n\
      </tr><tr>\n\
        <td>pH minus concentration factor:</td>\n\
        <td><input type=\"number\" step=\"0.001\" name=\"parameter_phminus_concentration\" value=\"");
  html += String(settings.pHMinusConcentration);
  html += F("\"> ml/litre for 1 pH point change</td>\n\
      </tr><tr>\n\
        <td>Target pH of the solution:</td>\n\
        <td><input type=\"number\" step=\"0.1\" name=\"parameter_targetph\" value=\"");
  html += String(settings.targetpH);
  html += F("\"></td>\n\
      </tr><tr>\n\
        <td>Name of this system:</td>\n\
        <td><input type=\"text\" name=\"parameter_systemname\" value=\"");
  html += String(settings.systemName);
  html += F("\"></td>\n\
      </tr>\n");
  logging->writeDebug("HydroMonitorGrowingParameters: created settings html.");
  
  return html;
}

/*
 * Update the settings for this sensor, if any.
 */
void HydroMonitorGrowingParameters::updateSettings(String keys[], String values[], uint8_t nArgs) {
  for (uint8_t i=0; i<nArgs; i++) {
    if (keys[i] == F("parameter_solutionvolume")) {
      if (core.isNumeric(values[i])) {
        uint16_t val = values[i].toInt();
        if (val >= 0 && val < 2000) settings.solutionVolume = val;
      }
    }
    else if (keys[i] == F("parameter_fertiliser_concentration")) {
      if (core.isNumeric(values[i])) {
        uint16_t val = values[i].toInt();
        if (val >= 0 && val < 500) settings.fertiliserConcentration = val;
      }
    }
    else if (keys[i] == F("parameter_targetec")) {
      if (core.isNumeric(values[i])) {
        float val = values[i].toFloat();
        if (val >= 0 && val <= 5) settings.targetEC = val;
      }
    }
    else if (keys[i] == F("parameter_phminus_concentration")) {
      if (core.isNumeric(values[i])) {
        uint16_t val = values[i].toInt();
        if (val >= 0 && val <= 10) settings.pHMinusConcentration = val;
      }
    }
    else if (keys[i] == F("parameter_targetph")) {
      if (core.isNumeric(values[i])) {
        float val = values[i].toFloat();
        if (val >= 0 && val <= 10) settings.targetpH = val;
      }
    }
    else if (keys[i] == F("parameter_systemname")) {
      if (keys[i].length() < 64) {
        char buf[64];
        values[i].toCharArray(buf, 64);
        strcpy(settings.systemName, buf);
      }
    }
    
  }
  EEPROM.put(FERTILISER_EEPROM, settings);
  EEPROM.commit();
  logging->writeTrace("HydroMonitorGrowingParameters: updated settings.");
  return;
}

void HydroMonitorGrowingParameters::updateSensorData() {

#ifdef USE_EC_SENSOR
  sensorData->targetEC = settings.targetEC;
  sensorData->fertiliserConcentration = settings.fertiliserConcentration;
#endif
#ifdef USE_PH_SENSOR
  sensorData->targetpH = settings.targetpH;
  sensorData->pHMinusConcentration = settings.pHMinusConcentration;
#endif
#if defined(USE_EC_SENSOR) || defined(USE_PH_SENSOR)
  sensorData->solutionVolume = settings.solutionVolume;
#endif
  strcpy(settings.systemName, "HydroMonitor");
}

