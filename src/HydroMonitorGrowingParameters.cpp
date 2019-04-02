#include <HydroMonitorGrowingParameters.h>

/*
 * Constructor.
 */
HydroMonitorGrowingParameters::HydroMonitorGrowingParameters() {
}

/*
 * Configure the module.
 */
void HydroMonitorGrowingParameters::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l) {
  sensorData = sd;
  logging = l;
  if (GROWING_PARAMETERS_EEPROM > 0)
#ifdef USE_24LC256_EEPROM
    sensorData->EEPROM->get(GROWING_PARAMETERS_EEPROM, settings);
#else
    EEPROM.get(GROWING_PARAMETERS_EEPROM, settings);
#endif

  // Check whether any settings have been set, if not apply defaults.
  if (settings.fertiliserConcentration > 500) {
    logging->writeTrace(F("HydroMonitorGrowingParameters: applying default settings."));
    settings.fertiliserConcentration = 200;
    settings.solutionVolume = 100;
    settings.targetEC = 1;
    settings.pHMinusConcentration = 0.05;  // ml of pH-minus per litre of solution for a 1 pH point change.
    settings.targetpH = 7;
    strcpy(settings.systemName, "HydroMonitor");
    settings.timezone = 0;
#ifdef USE_24LC256_EEPROM
    sensorData->EEPROM->put(GROWING_PARAMETERS_EEPROM, settings);
#else
    EEPROM.put(GROWING_PARAMETERS_EEPROM, settings);
    EEPROM.commit();
#endif
  }
  updateSensorData();
  logging->writeInfo(F("HydroMonitorGrowingParameters: set up all the growing parameters."));
  return;
}

/*
 * The html code for the sensor specific settings.
 */
void HydroMonitorGrowingParameters::settingsHtml(ESP8266WebServer *server) {
  char buff[10];
  server->sendContent_P(PSTR("\
      <tr>\n\
        <th colspan=\"2\">General growing and system parameters.</th>\n\
      </tr><tr>\n\
        <td>Total volume of the system:</td>\n\
        <td><input type=\"number\" step=\"1\" name=\"parameter_solutionvolume\" value=\""));
  server->sendContent(itoa(settings.solutionVolume, buff, 10));
  server->sendContent_P(PSTR("\"> litres</td>\n\
      </tr><tr>\n\
        <td>Fertiliser concentration factor:</td>\n\
        <td>1 : <input type=\"number\" step=\"1\" name=\"parameter_fertiliser_concentration\" value=\""));
  server->sendContent(itoa(settings.fertiliserConcentration, buff, 10));
  server->sendContent_P(PSTR("\"></td>\n\
      </tr><tr>\n\
        <td>Target EC of the solution:</td>\n\
        <td><input type=\"number\" step=\"0.01\" name=\"parameter_targetec\" value=\""));
  sprintf_P(buff, PSTR("%.2f"), settings.targetEC);
  server->sendContent(buff);
  server->sendContent_P(PSTR("\"> mS/cm</td>\n\
      </tr>"));
#ifdef USE_PH_SENSOR
  server->sendContent_P(PSTR("<tr>\n\
        <td>pH minus concentration factor:</td>\n\
        <td><input type=\"number\" step=\"0.001\" name=\"parameter_phminus_concentration\" value=\""));
  sprintf_P(buff, PSTR("%.2f"), settings.pHMinusConcentration);
  server->sendContent(buff);
  server->sendContent_P(PSTR("\"> ml/litre for 1 pH point change</td>\n\
      </tr><tr>\n\
        <td>Target pH of the solution:</td>\n\
        <td><input type=\"number\" step=\"0.01\" name=\"parameter_targetph\" value=\""));
  sprintf_P(buff, PSTR("%.2f"), settings.targetpH);
  server->sendContent(buff);
  server->sendContent_P(PSTR("\"></td>\n\
      </tr>"));
#endif
  server->sendContent_P(PSTR("<tr>\n\
        <td>Name of this system:</td>\n\
        <td><input type=\"text\" name=\"parameter_systemname\" value=\""));
  if (strlen(sensorData->systemName) > 0) {
    server->sendContent(sensorData->systemName);
  }
  server->sendContent_P(PSTR("\"></td>\n\
      </tr><tr>\n\
        <td>Time zone:</td>\n\
        <td><input type=\"text\" name=\"parameter_timezone\" value=\""));
  sprintf_P(buff, PSTR("%.2f"), settings.timezone);
  server->sendContent(buff);
  server->sendContent_P(PSTR("\"></td>\n\
      </tr>"));
}

/*
 * The JSON code for the sensor specific settings.
 */
void HydroMonitorGrowingParameters::settingsJSON(ESP8266WebServer *server) {
  char buff[10];
  server->sendContent_P(PSTR("  \"parameters\": {\n"
                             "    \"volume\":\""));
  server->sendContent(itoa(settings.solutionVolume, buff, 10));
  server->sendContent_P(PSTR("\",\n"
                             "    \"fertiliser_concentration\":\""));
  server->sendContent(itoa(settings.fertiliserConcentration, buff, 10));
  server->sendContent_P(PSTR("\",\n"
                             "    \"target_ec\":\""));
  sprintf_P(buff, PSTR("%.2f"), settings.targetEC);
  server->sendContent(buff);

#ifdef USE_PH_SENSOR
  server->sendContent_P(PSTR("\",\n"
                             "    \"ph_concentration\":\""));
  sprintf_P(buff, PSTR("%.2f"), settings.pHMinusConcentration);
  server->sendContent(buff);
  server->sendContent_P(PSTR("\",\n"
                             "    \"target_ph\":\""));
  sprintf_P(buff, PSTR("%.2f"), settings.targetpH);
  server->sendContent(buff);
#endif
  server->sendContent_P(PSTR("\",\n"
                             "    \"system_name\":\""));
                             
  Serial.print(F("systemName: \""));
  Serial.print(settings.systemName);
  Serial.println(F("\""));
  if (strlen(settings.systemName) > 0) {
    server->sendContent(settings.systemName);
  }
  server->sendContent_P(PSTR("\",\n"
                             "    \"timezone\":\""));
  sprintf_P(buff, PSTR("%.2f"), settings.timezone);
  server->sendContent(buff);
  server->sendContent_P(PSTR("\"\n"
                             "  }"));
}

/*
 * Update the settings for this sensor, if any.
 */
void HydroMonitorGrowingParameters::updateSettings(ESP8266WebServer* server) {
  for (uint8_t i=0; i<server->args(); i++) {
    if (server->argName(i) == F("parameter_solutionvolume")) {
      if (core.isNumeric(server->arg(i))) {
        uint16_t val = server->arg(i).toInt();
        if (val >= 0 && val < 2000) settings.solutionVolume = val;
      }
    }
    else if (server->argName(i) == F("parameter_fertiliser_concentration")) {
      if (core.isNumeric(server->arg(i))) {
        uint16_t val = server->arg(i).toInt();
        if (val >= 0 && val <= 500) settings.fertiliserConcentration = val;
      }
    }
    else if (server->argName(i) == F("parameter_targetec")) {
      if (core.isNumeric(server->arg(i))) {
        float val = server->arg(i).toFloat();
        if (val >= 0 && val <= 5) settings.targetEC = val;
      }
    }
    else if (server->argName(i) == F("parameter_phminus_concentration")) {
      if (core.isNumeric(server->arg(i))) {
        float val = server->arg(i).toFloat();
        if (val >= 0 && val <= 10) settings.pHMinusConcentration = val;
      }
    }
    else if (server->argName(i) == F("parameter_targetph")) {
      if (core.isNumeric(server->arg(i))) {
        float val = server->arg(i).toFloat();
        if (val >= 0 && val <= 10) settings.targetpH = val;
      }
    }
    else if (server->argName(i) == F("parameter_systemname")) {
      if (server->argName(i).length() < 65) {
        char buf[65];
        server->arg(i).toCharArray(buf, 64);
        strcpy(settings.systemName, buf);
      }
    }
    else if (server->argName(i) == F("parameter_timezone")) {
      if (core.isNumeric(server->arg(i))) {
        float val = server->arg(i).toFloat();
        if (val >= 0 && val <= 25) settings.timezone = val;
      }
    }
  }
#ifdef USE_24LC256_EEPROM
  sensorData->EEPROM->put(GROWING_PARAMETERS_EEPROM, settings);
#else
  EEPROM.put(GROWING_PARAMETERS_EEPROM, settings);
  EEPROM.commit();
#endif
  updateSensorData();
  logging->writeTrace(F("HydroMonitorGrowingParameters: updated settings."));
  return;
}

/*
  Copy all the parameters into the sensorData struct, so it can be shared by other modules.
 */
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
  strcpy(sensorData->systemName, settings.systemName);
  sensorData->timezone = settings.timezone;
}

