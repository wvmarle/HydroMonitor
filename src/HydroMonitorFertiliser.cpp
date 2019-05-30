#include <HydroMonitorFertiliser.h>

#ifdef USE_FERTILISER

#ifndef USE_EC_SENSOR
#error Can't do fertiliser dosing without EC sensor!
#endif

/*
   Add fertiliser to the system as needed, based on the current EC value.

   Constructor.
*/
HydroMonitorFertiliser::HydroMonitorFertiliser() {
  runBTime = 0;
  runATime = 0;
  fertiliserDelay = 30 * 60 * 1000; // Half an hour delay after adding fertiliser, to allow the system to mix properly.
  lastTimeAdded = -fertiliserDelay;
  addA = false;
  addB = false;
  lastWarned = millis() - WARNING_INTERVAL;
}

/*
   Configure the module.
*/
#ifdef FERTILISER_A_MCP_PIN
void HydroMonitorFertiliser::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l, Adafruit_MCP23008 *mcp23008) {
  mcp = mcp23008;
  pumpA = FERTILISER_A_MCP_PIN;
  pumpB = FERTILISER_B_MCP_PIN;
  mcp->pinMode(pumpA, OUTPUT);
  mcp->pinMode(pumpB, OUTPUT);
  l->writeTrace(F("HydroMonitorFertiliser: set up fertiliser pumps on MCP23008 port expander."));

#elif defined(FERTILISER_A_MCP17_PIN)
void HydroMonitorFertiliser::begin(HydroMonitorCore::SensorData * sd, HydroMonitorLogging * l, Adafruit_MCP23017 * mcp23017) {
  mcp = mcp23017;
  pumpA = FERTILISER_A_MCP17_PIN;
  pumpB = FERTILISER_B_MCP17_PIN;
  mcp->pinMode(pumpA, OUTPUT);
  mcp->pinMode(pumpB, OUTPUT);
  l->writeTrace(F("HydroMonitorFertiliser: set up fertiliser pumps on MCP23017 port expander."));

#elif defined(FERTILISER_A_PCF_PIN)
void HydroMonitorFertiliser::begin(HydroMonitorCore::SensorData * sd, HydroMonitorLogging * l, PCF857x * pcf) {
  pcf8574 = pcf;
  pumpA = FERTILISER_A_PCF_PIN;
  pumpB = FERTILISER_B_PCF_PIN;
  pcf8574->pinMode(pumpA, OUTPUT);
  pcf8574->pinMode(pumpB, OUTPUT);
  l->writeTrace(F("HydroMonitorFertiliser: set up fertiliser pumps on PCF8574 port expander."));

#elif defined(FERTILISER_A_PIN)
void HydroMonitorFertiliser::begin(HydroMonitorCore::SensorData * sd, HydroMonitorLogging * l) {
  pumpA = FERTILISER_A_PIN;
  pumpB = FERTILISER_B_PIN;
  pinMode(pumpA, OUTPUT);
  pinMode(pumpB, OUTPUT);
  l->writeTrace(F("HydroMonitorFertiliser: set up fertiliser pumps with direct port connection."));
#endif

  sensorData = sd;
  logging = l;
  switchPumpOff(pumpA);
  switchPumpOff(pumpB);
  if (FERTILISER_EEPROM > 0)
#ifdef USE_24LC256_EEPROM
    sensorData->EEPROM->get(FERTILISER_EEPROM, settings);
#else
    EEPROM.get(FERTILISER_EEPROM, settings);
#endif

  // Check whether any settings have been set, if not apply defaults.
  if (settings.pumpASpeed < 1 || settings.pumpASpeed > 500) {
    logging->writeTrace(F("HydroMonitorFertiliser: applying default settings."));
    settings.pumpASpeed = 100;
    settings.pumpBSpeed = 100;
#ifdef USE_24LC256_EEPROM
    sensorData->EEPROM->put(FERTILISER_EEPROM, settings);
#else
    EEPROM.put(FERTILISER_EEPROM, settings);
    EEPROM.commit();
#endif
  }
}

/*
   The fertiliser handling.
   This has to be called frequently, to prevent delays in the switching.

   Step 1: check whether we've recently added fertiliser. If so, return instantly.
   Step 2: check whether we have to start adding B,
      if not: check whether we are adding B and if so, whether it's enough,
      if not: check whether we have to start adding A,
      if not: check whether we are adding A and if so, whether it's enough,
      if not: check whether the current EC is higher than 5% below the target EC,
      if not: check whether the last time we had high enough EC was more than 10 minutes ago,
              and if so, set the flags to start adding fertiliser, and calculate how much.
*/
void HydroMonitorFertiliser::doFertiliser() {

  // If we're measuring the pumps, let them run for 60 seconds, then switch off.
  // The pumps are started elsewhere; here just keep track of the time they've been running
  // and switch the running one off when it's due. This also stops the measuring
  // procedure.
  // The pumps are started by measurePumpA() and measurePumpB() respectively.
  if (measuring) {
    if (aRunning) {
      if (millis() - runATime > 60 * 1000) {
        logging->writeTrace(F("HydroMonitorFertiliser: measuring pump A finished."));
        switchPumpOff(pumpA);
        measuring = false;
        aRunning = false;
      }
    }
    else if (millis() - runBTime > 60 * 1000) {
      logging->writeTrace(F("HydroMonitorFertiliser: measuring pump B finished."));
      switchPumpOff(pumpB);
      measuring = false;
      bRunning = false;
    }
  }

  else if ((sensorData->EC <= 0) ||                         // Don't start adding fertiliser if the sensor is not connected, or if calibration is
           //                                                  not set (in which case the value is nan, and any comparison results in false).
           sensorData->targetEC == 0 ||                     // The user may set the targetEC to 0 to stop this process.
           sensorData->solutionVolume == 0 ||               // All parameters must have been set.
           sensorData->fertiliserConcentration == 0) {
    // Nothing to do in this case.
  }

  else if (bitRead(sensorData->systemStatus, STATUS_RESERVOIR_LEVEL_LOW) || // Don't start adding fertiliser if the reservoir level is low,
           bitRead(sensorData->systemStatus, STATUS_DRAINING_RESERVOIR) ||  // the reservoir is being drained,
           bitRead(sensorData->systemStatus, STATUS_MAINTENANCE) ||         // the system is in maintenance mode, or
           bitRead(sensorData->systemStatus, STATUS_DRAINAGE_NEEDED)) {     // drainage has been requested.
    if (aRunning) {                                         // If they're running, switch off the pumps.
      aRunning = false;
      switchPumpOff(pumpA);
    }
    if (bRunning) {
      bRunning = false;
      switchPumpOff(pumpA);
    }
    lastGoodEC = millis();                                  // Make sure we're locked out from adding EC for some time.
  }

  // It's best to start with B first, and do A second.
  else if (addB) {                                          // Add solution B, if needed.
    logging->writeTrace(F("HydroMonitorFertiliser: start adding solution B, switching pump B on."));
    switchPumpOn(pumpB);                                    // Switch on the pump,
    bRunning = true;                                        // and flag it's running.
    addB = false;                                           // Reset this flag, as we're adding B now.
    startTime = millis();                                   // The time the addition started.
  }

  // Check whether B is running, and if so whether it's time to stop.
  else if (bRunning && millis() - startTime > runBTime) {
    logging->writeTrace(F("HydroMonitorFertiliser: finished adding solution B, switching pump B off."));
    switchPumpOff(pumpB);                                   // Switch off the pump.
    startATime = millis();                                  // Record when we did this, after a short delay A may start.
    bRunning = false;                                       // Reset the running flag.
  }

  // Start adding A if needed and B is not running and the 500 ms break is over.
  else if (addA && !bRunning && millis() - startATime > 500) {
    logging->writeTrace(F("HydroMonitorFertiliser: start adding solution A, switching pump A on."));
    switchPumpOn(pumpA);                                    // Switch on the pump,
    aRunning = true;                                        // and flag it's running.
    addA = false;                                           // Reset this flag, as we're adding A now.
    startTime = millis();                                   // The time the addition started.
  }

  // Check whether A is running, and if so whether it's time to stop.
  else if (aRunning && millis() - startTime > runATime) {
    logging->writeTrace(F("HydroMonitorFertiliser: finished adding solution A, switching pump A off."));
    switchPumpOff(pumpA);                                   // Switch off the pump.
    aRunning = false;                                       // Reset the running flag.
    lastTimeAdded = millis();                               // Keep track of when we were done, in order to enforce the 30-minute delay.
  }

  // Check whether the EC is not more than 0.1 mS/cm under the target value
  // and less than 0.5 mS/cm above the target value, and if so
  // record the time when we found the EC to be good.
  else if (sensorData->EC > sensorData->targetEC - 0.1 &&
           sensorData->EC < sensorData->targetEC + 0.5) {
    lastGoodEC = millis();
  }

  // Start adding after ten minutes of continuous too low EC, and at least fertiliserDelay
  // since we last added any fertiliser.
  else if (sensorData->EC < sensorData->targetEC &&
           millis() - lastGoodEC > 10 * 60 * 1000 &&
           millis() - lastTimeAdded > fertiliserDelay) {

    // Flag that we want to add fertilisers - both, of course.
    addA = true;
    addB = true;
    float addVolume = 0.1 * 1000 * sensorData->solutionVolume / sensorData->fertiliserConcentration; // The amount of fertiliser in ml to be added of 0.1 mS/cm worth of EC.
    if (sensorData->EC > sensorData->targetEC - 0.1) {      // If the EC we measure is more than 0.1 mS/cm lower than the target volume, add a full shot of fertiliser.
      if (sensorData->EC > sensorData->targetEC - 1) {      // But don't add more than 1 mS/cm worth of fertiliser in one go.
        addVolume *= 1 * 10;
      }
      else {
        addVolume *= (sensorData->targetEC - sensorData->EC) * 10;
      }
    }
    logging->writeTrace(F("HydroMonitorFertiliser: 10 minutes of too low EC; have to start adding fertiliser."));
    char buff[70];
    sprintf_P(buff, PSTR("HydroMonitorFertiliser: adding %3.1f ml of fertiliser solution."), addVolume);
    logging->writeInfo(buff);
    runATime = addVolume / settings.pumpASpeed * 60 * 1000; // the time in milliseconds pump A has to run.
    runBTime = addVolume / settings.pumpBSpeed * 60 * 1000; // the time in milliseconds pump B has to run.
    lastTimeAdded = millis();                               // Time we last added any fertiliser - which is what we're going to do now.
    originalEC = sensorData->EC;                            // Try to detect whether the EC really came up as expected.
  }

#ifdef USE_DRAINAGE_PUMP
  // Start draining the reservoir after ten minutes of continuous too high EC.
  else if (sensorData->EC > sensorData->targetEC &&
           millis() - lastGoodEC > 10 * 60 * 1000) {
    logging->writeTrace(F("HydroMonitorFertiliser: 10 minutes of too high EC; completely refresh the reservoir."));
    logging->writeInfo(F("HydroMonitorFertiliser: refreshing reservoir to be able to reduce the EC value."));
    bitSet(sensorData->systemStatus, STATUS_DRAINAGE_NEEDED);
  }
#endif

  if (lastTimeAdded > 0 &&
      sensorData->EC > 0) {
    if (millis() - lastTimeAdded < 30 * 60 * 1000 &&          // Monitor EC for 30 minutes after last time added; it should be going up now.
        sensorData->EC > originalEC + 0.07) {
      originalEC = 0;
    }
    else if (originalEC > 0 && millis() - lastWarned > WARNING_INTERVAL) {
      logging->writeWarning(F("Fertiliser 01: fertiliser did not go up as expected after running the pumps. Fertiliser bottles may be empty."));
      lastWarned = millis();
    }
  }
}

/*
   Swich pump p on.
*/
void HydroMonitorFertiliser::switchPumpOn(uint8_t p) {
#ifdef FERTILISER_A_PIN
  digitalWrite(p, HIGH);
#elif defined(FERTILISER_A_PCF_PIN)
  pcf8574->write(p, LOW);
#elif defined(FERTILISER_A_MCP_PIN) || defined(FERTILISER_A_MCP17_PIN)
  mcp->digitalWrite(p, HIGH);
#endif
}

/*
   Swich pump p off.
*/
void HydroMonitorFertiliser::switchPumpOff(uint8_t p) {
#ifdef FERTILISER_A_PIN
  digitalWrite(p, LOW);
#elif defined(FERTILISER_A_PCF_PIN)
  pcf8574->write(p, HIGH);
#elif defined(FERTILISER_A_MCP_PIN) || defined(FERTILISER_A_MCP17_PIN)
  mcp->digitalWrite(p, LOW);
#endif
}


/*
   Start running pump A for 60 seconds to measure the flow.
*/
void HydroMonitorFertiliser::measurePumpA() {
  if (!(aRunning || bRunning)) {
    logging->writeTrace(F("HydroMonitorFertiliser: switching on pump A."));
    switchPumpOn(pumpA);
    runATime = millis();
    aRunning = true;
    measuring = true;
  }
}

/*
   Start running pump B for 60 seconds to measure the flow.
*/
void HydroMonitorFertiliser::measurePumpB() {
  if ( !(aRunning || bRunning)) {
    logging->writeTrace(F("HydroMonitorFertiliser: switching on pump B."));
    switchPumpOn(pumpB);
    runBTime = millis();
    bRunning = true;
    measuring = true;
  }
}

/*
   The html code for the sensor specific settings.
*/
void HydroMonitorFertiliser::settingsHtml(ESP8266WebServer * server) {
  char buff[10];
  server->sendContent_P(PSTR("\
      <tr>\n\
        <th colspan=\"2\">Fertiliser dosing settings.</th>\n\
      </tr><tr>\n\
        <td>Speed of pump A:</td>\n\
        <td><input type=\"number\" step=\"0.1\" name=\"fertiliser_pumpaspeed\" value=\""));
  sprintf(buff, "%.2f", settings.pumpASpeed);
  server->sendContent(buff);
  server->sendContent_P(PSTR("\"> ml/minute.&nbsp;&nbsp;<input type=\"submit\" formaction=\"/measure_pump_a_speed\" formmethod=\"post\" name=\"pumpa\" value=\"Measure now\"></td>\n\
      </tr><tr>\n\
        <td>Speed of pump B:</td>\n\
        <td><input type=\"number\" step=\"0.1\" name=\"fertiliser_pumpbspeed\" value=\""));
  sprintf(buff, "%.2f", settings.pumpBSpeed);
  server->sendContent(buff);
  server->sendContent_P(PSTR("\"> ml/minute.&nbsp;&nbsp;<input type=\"submit\" formaction=\"/measure_pump_b_speed\" formmethod=\"post\" name=\"pumpb\" value=\"Measure now\"></td>\n\
      </tr>\n"));
}

/*
   The JSON code for the sensor specific settings.
*/
bool HydroMonitorFertiliser::settingsJSON(ESP8266WebServer * server) {
  char buff[10];
  server->sendContent_P(PSTR("  \"fertiliser\": {\n"
                             "    \"pump_a_speed\":\""));
  sprintf(buff, "%.2f", settings.pumpASpeed);
  server->sendContent(buff);
  server->sendContent_P(PSTR("\",\n"
                             "    \"pump_b_speed\":\""));
  sprintf(buff, "%.2f", settings.pumpBSpeed);
  server->sendContent(buff);
  server->sendContent_P(PSTR("\"\n"
                             "  }"));
  return true;
}

/*
   Update the settings for this sensor, if any.
*/
void HydroMonitorFertiliser::updateSettings(ESP8266WebServer * server) {
  for (uint8_t i = 0; i < server->args(); i++) {
    if (server->argName(i) == F("fertiliser_pumpaspeed")) {
      if (core.isNumeric(server->arg(i))) {
        float val = server->arg(i).toFloat();
        if (val > 0 && val < 2001) settings.pumpASpeed = val;
      }
      else settings.pumpASpeed = 80;
    }
    if (server->argName(i) == F("fertiliser_pumpbspeed")) {
      if (core.isNumeric(server->arg(i))) {
        float val = server->arg(i).toFloat();
        if (val > 0 && val < 2001) settings.pumpBSpeed = val;
      }
      else settings.pumpBSpeed = 80;
    }
  }
#ifdef USE_24LC256_EEPROM
  sensorData->EEPROM->put(FERTILISER_EEPROM, settings);
#else
  EEPROM.put(FERTILISER_EEPROM, settings);
  EEPROM.commit();
#endif
  logging->writeTrace(F("HydroMonitorFertiliser: updated settings."));
}

#endif

