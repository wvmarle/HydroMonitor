#include <HydroMonitorDrainage.h>

#ifdef USE_DRAINAGE

/*
   The constructor.
*/
HydroMonitorDrainage::HydroMonitorDrainage() {
}

/*
   Set up the drainage system.
*/
#ifdef USE_WATERLEVEL_SENSOR
#ifdef DRAINAGE_MCP17_PIN         // Connected to MCP23017 port expander.
void HydroMonitorDrainage::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l, Adafruit_MCP23017* mcp23017, HydroMonitorWaterLevelSensor* sens) {
  mcp = mcp23017;
  mcp->pinMode(DRAINAGE_MCP17_PIN, OUTPUT);
  l->writeTrace(F("HydroMonitorDrainage: configured drainage pump on MCP23017 port expander."));

#elif defined(DRAINAGE_MCP_PIN)   // Connected to MCP23008 port expander.
void HydroMonitorDrainage::begin(HydroMonitorCore::SensorData * sd, HydroMonitorLogging * l, Adafruit_MCP23008 * mcp23008, HydroMonitorWaterLevelSensor * sens) {
  mcp = mcp23008;
  mcp->pinMode(DRAINAGE_MCP_PIN, OUTPUT);
  l->writeTrace(F("HydroMonitorDrainage: configured drainage pump on MCP23008 port expander."));

#elif defined(DRAINAGE_PIN)         // Connected to GPIO port.
void HydroMonitorDrainage::begin(HydroMonitorCore::SensorData * sd, HydroMonitorLogging * l, HydroMonitorWaterLevelSensor * sens) {
  pinMode(DRAINAGE_PIN, OUTPUT);
  l->writeTrace(F("HydroMonitorDrainage: configured drainage pump."));
#endif
  waterLevelSensor = sens;
#else                                                       // Not using water level sensor.
#ifdef DRAINAGE_MCP17_PIN         // Connected to MCP23017 port expander.
void HydroMonitorDrainage::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l, Adafruit_MCP23017* mcp23017) {
  mcp = mcp23017;
  mcp->pinMode(DRAINAGE_MCP17_PIN, OUTPUT);
  l->writeTrace(F("HydroMonitorDrainage: configured drainage pump on MCP23017 port expander."));

#elif defined(DRAINAGE_MCP_PIN)   // Connected to MCP23008 port expander.
void HydroMonitorDrainage::begin(HydroMonitorCore::SensorData * sd, HydroMonitorLogging * l, Adafruit_MCP23008 * mcp23008) {
  mcp = mcp23008;
  mcp->pinMode(DRAINAGE_MCP_PIN, OUTPUT);
  l->writeTrace(F("HydroMonitorDrainage: configured drainage pump on MCP23008 port expander."));

#elif defined(DRAINAGE_PIN)         // Connected to GPIO port.
void HydroMonitorDrainage::begin(HydroMonitorCore::SensorData * sd, HydroMonitorLogging * l) {
  pinMode(DRAINAGE_PIN, OUTPUT);
  l->writeTrace(F("HydroMonitorDrainage: configured drainage pump."));
#endif                                                      // endif pin definitions.
#endif                                                      // endif USE_WATERLEVEL_SENSOR
  sensorData = sd;
  logging = l;
  if (DRAINAGE_EEPROM > 0) {
#ifdef USE_24LC256_EEPROM
    sensorData->EEPROM->get(DRAINAGE_EEPROM, settings);
#else
    EEPROM.get(DRAINAGE_EEPROM, settings);
#endif
  }
  switchPumpOff();

  if (settings.drainageInterval < 1 || settings.drainageInterval > 400) {
    logging->writeTrace(F("HydroMonitorDrainage: applying default settings."));
    settings.drainageInterval = 60;
    if (timeStatus() != timeNotSet && now() > 1546300800) { // midnight, 1 Jan 2019.
      settings.latestDrainage = now();
    }
    else {
      settings.latestDrainage = 1546300800;
    }
#ifdef USE_24LC256_EEPROM
    sensorData->EEPROM->put(DRAINAGE_EEPROM, settings);
#else
    EEPROM.put(DRAINAGE_EEPROM, settings);
    EEPROM.commit();
#endif
  }
}

/*
   Handle the drainage.
*/
void HydroMonitorDrainage::doDrainage() {
  if (bitRead(sensorData->systemStatus, STATUS_DRAINAGE_NEEDED)) { // Immediate drainage has been requested.
    bitClear(sensorData->systemStatus, STATUS_DRAINAGE_NEEDED);
    if (drainageState == DRAINAGE_IDLE) {
      drainageState = DRAINAGE_DRAIN_EXCESS;
      logging->writeInfo(F("HydroMonitorDrainage: immediate drainage requested."));
      settings.latestDrainage = now();
      drainageStart = millis();    
    }
  }
  if (now() > 1546300800) {                                 // If we have a sensible time already,
    if (settings.latestDrainage > now()) {                  // but latestDrainage is in the future,
      settings.latestDrainage = now();                      // set it to the current time.
#ifdef USE_24LC256_EEPROM
      sensorData->EEPROM->put(DRAINAGE_EEPROM, settings);
#else
      EEPROM.put(DRAINAGE_EEPROM, settings);
      EEPROM.commit();
#endif
    }
  }
#ifdef USE_WATERLEVEL_SENSOR
  if (millis() - lastLevelCheck > 500 &&
      (drainageState == DRAINAGE_AUTOMATIC_DRAINING_RUNNING ||
       drainageState == DRAINAGE_MANUAL_DRAINING_RUNNING ||
       drainageState == DRAINAGE_DRAIN_EXCESS_RUNNING)) {
    lastLevelCheck += 500;
    waterLevelSensor->readSensor(true);
  }

  if (sensorData->waterLevel < 95) {                        // If level >95% it's too high and we have to drain some water now.
    lastGoodFill = millis();
  }
#endif
  switch (drainageState) {
    case DRAINAGE_IDLE:
      if (now() > settings.latestDrainage + (uint32_t)settings.drainageInterval * 24 * 60 * 60) {
        drainageState = DRAINAGE_AUTOMATIC_DRAINING_START;
        logging->writeInfo(F("HydroMonitorDrainage: scheduled full drainage of the reservoir: solution maintenance."));
      }
#ifdef USE_WATERLEVEL_SENSOR
      else if (millis() - lastGoodFill > (uint32_t)2 * 60 * 1000) { // Drain some water if fill level is >95% for >2 minutes.
        drainageState = DRAINAGE_DRAIN_EXCESS;
        logging->writeWarning(F("Drainage 01: reservoir fill level too high for more than 2 minutes; draining the excess."));
      }
#endif
      break;

    // Some system states (currently watering, door open) stop automatic draining from commencing.
    // This does not affect manual draining, nor will it interrupt a sequence in progress.
    case DRAINAGE_AUTOMATIC_DRAINING_START:
      if ((bitRead(sensorData->systemStatus, STATUS_WATERING) ||
           bitRead(sensorData->systemStatus, STATUS_DOOR_OPEN)) == false) {
        logging->writeTrace(F("Start automatic draining sequence."));
        switchPumpOn();
        drainageState = DRAINAGE_AUTOMATIC_DRAINING_RUNNING;
        drainageStart = millis();
#ifndef USE_WATERLEVEL_SENSOR
        bitSet(sensorData->systemStatus, STATUS_RESERVOIR_LEVEL_LOW); // We're completely draining the reservoir now.
#endif
      }
      break;

    case DRAINAGE_AUTOMATIC_DRAINING_RUNNING:
#ifdef USE_WATERLEVEL_SENSOR
      if (sensorData->waterLevel < 10) {
#else
      if (millis() - drainageStart > (uint32_t)7 * 60 * 1000) {  // 7 mins here + 1 minute to complete for 8 minutes total.
#endif
        drainageCompletedTime = millis();
        drainageState = DRAINAGE_AUTOMATIC_DRAINING_COMPLETE;
        logging->writeTrace(F("Automatic draining sequence emptied reservoir; continue 60 seconds."));
      }
      if (millis() - drainageStart > (uint32_t)20 * 60 * 1000) {
        if (millis() - lastWarned > WARNING_INTERVAL) {
          logging->writeError(F("Drainage 02: Automatic draining sequence not completed in 20 minutes; possible pump malfunction."));
          lastWarned = millis();
        }
      }
      break;

    case DRAINAGE_AUTOMATIC_DRAINING_COMPLETE:
      if (millis() - drainageCompletedTime > (uint32_t)60 * 1000) { // Continue to pump for 60 seconds to make sure the reservoir is really empty.
        logging->writeTrace(F("Automatic draining sequence completed."));
        switchPumpOff();
        settings.latestDrainage = now();
        lastDrainageRun = millis();
#ifdef USE_24LC256_EEPROM
        sensorData->EEPROM->put(DRAINAGE_EEPROM, settings);
#else
        EEPROM.put(DRAINAGE_EEPROM, settings);
        EEPROM.commit();
#endif
        drainageState = DRAINAGE_IDLE;
        bitClear(sensorData->systemStatus, STATUS_MAINTENANCE);
#ifndef USE_WATERLEVEL_SENSOR
        bitSet(sensorData->systemStatus, STATUS_RESERVOIR_DRAINED);
#endif
      }
#ifdef USE_WATERLEVEL_SENSOR
      else if (sensorData->waterLevel > 12) {                  // It was a false reading.
        drainageState = DRAINAGE_AUTOMATIC_DRAINING_RUNNING;
      }
#endif
      break;

    case DRAINAGE_MANUAL_DRAINING_START:
      switchPumpOn();
      drainageState = DRAINAGE_MANUAL_DRAINING_RUNNING;
      bitSet(sensorData->systemStatus, STATUS_MAINTENANCE);
      drainageStart = millis();
#ifndef USE_WATERLEVEL_SENSOR
      bitSet(sensorData->systemStatus, STATUS_RESERVOIR_DRAINED); // Ensure refilling of the reservoir when we're done.
#endif
      break;

    case DRAINAGE_MANUAL_DRAINING_RUNNING:
#ifdef USE_WATERLEVEL_SENSOR
      if (sensorData->waterLevel < 10) {
#else
      if (millis() - drainageStart > (uint32_t)7 * 60 * 1000) { // 7 mins here + 1 minute to complete for 8 minutes total.
#endif
        drainageCompletedTime = millis();
        drainageState = DRAINAGE_MANUAL_DRAINING_HOLD_EMPTY;
        logging->writeTrace(F("Manual draining sequence emptied reservoir; continue 60 seconds."));
      }
      break;

    case DRAINAGE_MANUAL_DRAINING_HOLD_EMPTY:
      if (millis() - drainageCompletedTime > (uint32_t)60 * 1000) { // Continue to pump for 60 seconds to make sure the reservoir is really empty.
        switchPumpOff();
        logging->writeTrace(F("Manual draining sequence completed."));
        drainageState = DRAINAGE_MANUAL_DRAINING_COMPLETE;
        lastDrainageRun = millis();
        if (now() > 1546300800) {                               // Only store time if it makes sense to do so.
          settings.latestDrainage = now();
#ifdef USE_24LC256_EEPROM
          sensorData->EEPROM->put(DRAINAGE_EEPROM, settings);
#else
          EEPROM.put(DRAINAGE_EEPROM, settings);
          EEPROM.commit();
#endif
        }
      }
#ifdef USE_WATERLEVEL_SENSOR
      if (sensorData->waterLevel > 12) {                  // It was a false reading.
        drainageState = DRAINAGE_MANUAL_DRAINING_RUNNING;
      }
#endif
      break;

    case DRAINAGE_MANUAL_DRAINING_COMPLETE:
      break;

    case DRAINAGE_MAINTENANCE_RUN:
      switchPumpOn();
      lastDrainageRun = millis();
      drainageState = DRAINAGE_MAINTENANCE_RUNNING;
      break;

    case DRAINAGE_MAINTENANCE_RUNNING:
      if (millis() - lastDrainageRun > 3000) {           // Run the pump for 3 seconds, just to keep everything smooth.
        switchPumpOff();
        lastDrainageRun = millis();
        drainageState = DRAINAGE_IDLE;
        bitClear(sensorData->systemStatus, STATUS_MAINTENANCE);
      }
      break;

    case DRAINAGE_DRAIN_EXCESS:
      switchPumpOn();
      drainageStart = millis();
      drainageState = DRAINAGE_DRAIN_EXCESS_RUNNING;
      break;

    case DRAINAGE_DRAIN_EXCESS_RUNNING:
#ifdef USE_WATERLEVEL_SENSOR
      if (sensorData->waterLevel < 90) {
        lastDrainageRun = millis();
        drainageState = DRAINAGE_IDLE;
        bitClear(sensorData->systemStatus, STATUS_MAINTENANCE);
        switchPumpOff();
        logging->writeTrace(F("HydroMonitorDrainage: reservoir drained to <90% fill level."));
      }
      if (millis() - drainageStart > (uint32_t)20 * 60 * 1000) {
        if (millis() - lastWarned > WARNING_INTERVAL) {
          logging->writeError(F("Drainage 03: drainage of the excess not completed within 20 minutes; possible malfunction."));
          lastWarned = millis();
        }
      }
#else
      if (millis() - drainageStart > (uint32_t)8 * 60 * 1000) {
        lastDrainageRun = millis();
        drainageState = DRAINAGE_IDLE;
        bitClear(sensorData->systemStatus, STATUS_MAINTENANCE);
        switchPumpOff();
        logging->writeTrace(F("HydroMonitorDrainage: reservoir emergency drainage complete."));
        bitSet(sensorData->systemStatus, STATUS_RESERVOIR_DRAINED); // Allow refilling of the reservoir when we're done.
      }
#endif
      break;
  }
}

/*
   The sensor settings as html.
*/
void HydroMonitorDrainage::settingsHtml(ESP8266WebServer * server) {
  server->sendContent_P(PSTR("\
      <tr>\n\
        <th colspan=\"2\">Reservoir drainage settings.</th>\n\
      </tr><tr>\n\
        <td>Next scheduled reservoir refresh: </td>\n\
        <td>"));
  char timestamp[21];
  core.datetime(timestamp, (time_t)(settings.latestDrainage + (uint32_t)settings.drainageInterval * 24 * 60 * 60));
  server->sendContent(timestamp);
  server->sendContent_P(PSTR("</td>\n\
      </tr><tr>\n\
        <td>Drainage interval:</td>\n\
        <td><input type=\"number\" step=\"1\" name=\"drainage_interval\" value=\""));
  server->sendContent(itoa(settings.drainageInterval, timestamp, 10));
  server->sendContent_P(PSTR("\"> days.&nbsp;&nbsp;"));

  if (autoDrainageMode()) {
    server->sendContent_P(PSTR("<input type=\"submit\" formaction=\"/drain_start\" formmethod=\"post\" name=\"drainage\" value=\"Drain now\">"));
  }
  else {
    server->sendContent_P(PSTR("<input type=\"submit\" formaction=\"/drain_stop\" formmethod=\"post\" name=\"drainage\" value=\"Set automatic\">"));
  }
  server->sendContent_P(PSTR("</td>\n\
      </tr>\n"));
}

/*
   The sensor settings as JSON.
*/
bool HydroMonitorDrainage::settingsJSON(ESP8266WebServer * server) {
  server->sendContent_P(PSTR("  \"drainage\":\ {\n"
                             "    \"next_refresh\":\""));
  char buff[12];
  server->sendContent(itoa(settings.latestDrainage + (uint32_t)settings.drainageInterval * 24 * 60 * 60, buff, 10));
  server->sendContent_P(PSTR("\",\n"
                             "    \"drainage_interval\":\""));
  server->sendContent(itoa(settings.drainageInterval, buff, 10));
  server->sendContent_P(PSTR("\",\n"
                             "    \"drainage_state\":\""));
  if (autoDrainageMode()) {
    server->sendContent_P(PSTR("auto"));
  }
  else {
    server->sendContent_P(PSTR("manual"));
  }
  server->sendContent_P(PSTR("\"\n"
                             "  }"));
  return true;
}

bool HydroMonitorDrainage::autoDrainageMode() {
  return (drainageState != DRAINAGE_MANUAL_DRAINING_START &&
          drainageState != DRAINAGE_MANUAL_DRAINING_RUNNING &&
          drainageState != DRAINAGE_MANUAL_DRAINING_HOLD_EMPTY &&
          drainageState != DRAINAGE_MANUAL_DRAINING_COMPLETE);
}

void HydroMonitorDrainage::drainStart() {
  logging->writeTrace(F("HydroMonitorDrainage::drainStart(): Starting manual drainage sequence - into maintenance mode."));
  bitSet(sensorData->systemStatus, STATUS_MAINTENANCE);
  drainageState = DRAINAGE_MANUAL_DRAINING_START;
}

void HydroMonitorDrainage::drainStop() {
  logging->writeTrace(F("HydroMonitorDrainage::drainStop(): Stopping manual drainage sequence - back to normal."));
  drainageState = DRAINAGE_IDLE;
  bitClear(sensorData->systemStatus, STATUS_MAINTENANCE);
  switchPumpOff();
}

/*
   Update the settings.
*/
void HydroMonitorDrainage::updateSettings(ESP8266WebServer * server) {
  for (uint8_t i = 0; i < server->args(); i++) {
    if (server->argName(i) == F("drainage_interval")) {
      if (core.isNumeric(server->arg(i))) {
        uint32_t val = server->arg(i).toInt();
        if (val > 0 && val < 400) {
          settings.drainageInterval = val;
        }
      }
      else settings.drainageInterval = 60;
    }
  }

#ifdef USE_24LC256_EEPROM
  sensorData->EEPROM->put(DRAINAGE_EEPROM, settings);
#else
  EEPROM.put(DRAINAGE_EEPROM, settings);
  EEPROM.commit();
#endif
}

/*
   Swich pump on.
*/
void HydroMonitorDrainage::switchPumpOn() {
  logging->writeTrace(F("HydroMonitorDrainage: switching on drainage pump."));
#ifdef DRAINAGE_PIN
  digitalWrite(DRAINAGE_PIN, HIGH);
#elif defined(DRAINAGE_MCP_PIN)
  mcp->digitalWrite(DRAINAGE_MCP_PIN, HIGH);
#elif defined(DRAINAGE_MCP17_PIN)
  mcp->digitalWrite(DRAINAGE_MCP17_PIN, HIGH);
#endif
  bitSet(sensorData->systemStatus, STATUS_DRAINING_RESERVOIR);
}

/*
   Swich pump off.
*/
void HydroMonitorDrainage::switchPumpOff() {
  logging->writeTrace(F("HydroMonitorDrainage: switching off drainage pump."));
#ifdef DRAINAGE_PIN
  digitalWrite(p, LOW);
#elif defined(DRAINAGE_MCP_PIN)
  mcp->digitalWrite(DRAINAGE_MCP_PIN, LOW);
#elif defined(DRAINAGE_MCP17_PIN)
  mcp->digitalWrite(DRAINAGE_MCP17_PIN, LOW);
#endif
  bitClear(sensorData->systemStatus, STATUS_DRAINING_RESERVOIR);
}

#endif

