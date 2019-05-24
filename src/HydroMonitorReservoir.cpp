#include <HydroMonitorReservoir.h>

#ifdef USE_RESERVOIR
HydroMonitorReservoir::HydroMonitorReservoir() {
  lastWarned = millis() - WARNING_INTERVAL;
  startAddWater = -WARNING_INTERVAL;
  oldBeep = false;
  beep = false;
}

//
// Note:
// The pin is set to INPUT in order to be able to check the float switch, if connected.
// The openValve() and closeValve() set the pinMode to output resp. input as well.
//
/*
   Set up the solenoid, connected to a MCP23017 port expander.
*/
#ifdef USE_WATERLEVEL_SENSOR
#ifdef RESERVOIR_MCP17_PIN
void HydroMonitorReservoir::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l, Adafruit_MCP23017* mcp23017, HydroMonitorWaterLevelSensor* sens) {
  mcp = mcp23017;
  mcp->pinMode(RESERVOIR_MCP17_PIN, INPUT);
  l->writeTrace(F("HydroMonitorReservoir: configured reservoir refill on MCP23017 port expander."));

/*
   Set up the solenoid, connected to a MCP23008 port expander.
*/
#elif defined(RESERVOIR_MCP_PIN)
void HydroMonitorReservoir::begin(HydroMonitorCore::SensorData * sd, HydroMonitorLogging * l, Adafruit_MCP23008 * mcp23008, HydroMonitorWaterLevelSensor * sens) {
  mcp = mcp23008;
  mcp->pinMode(RESERVOIR_MCP_PIN, INPUT);
  l->writeTrace(F("HydroMonitorReservoir: configured reservoir refill on MCP23008 port expander."));

  /*
     Set up the solenoid, connected to a PCF8574 port expander.
  */
#elif defined(RESERVOIR_PCF_PIN)
void HydroMonitorReservoir::begin(HydroMonitorCore::SensorData * sd, HydroMonitorLogging * l, PCF857x * pcf, HydroMonitorWaterLevelSensor * sens) {
  pcf8574 = pcf;
  pcf8574->pinMode(RESERVOIR_PCF_PIN, INPUT);
  l->writeTrace(F("HydroMonitorReservoir: configured reservoir refill on PCF8574 port expander."));

  /*
     Set up the solenoid, connected to a GPIO port.
  */
#elif defined(RESERVOIR_PIN)
void HydroMonitorReservoir::begin(HydroMonitorCore::SensorData * sd, HydroMonitorLogging * l, HydroMonitorWaterLevelSensor * sens) {
  pinMode(RESERVOIR_PIN, INPUT);
  l->writeTrace(F("HydroMonitorReservoir: configured reservoir refill."));
#endif
  waterLevelSensor = sens;
  reservoirEmptyTime = millis();
  initialFillingDone = false;
  lastGoodFill = millis();
#else                                                       // Not using a waterlevel sensor.
#ifdef RESERVOIR_MCP17_PIN
void HydroMonitorReservoir::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l, Adafruit_MCP23017* mcp23017) {
  mcp = mcp23017;
  mcp->pinMode(RESERVOIR_MCP17_PIN, INPUT);
  l->writeTrace(F("HydroMonitorReservoir: configured reservoir refill on MCP23017 port expander."));

/*
   Set up the solenoid, connected to a MCP23008 port expander.
*/
#elif defined(RESERVOIR_MCP_PIN)
void HydroMonitorReservoir::begin(HydroMonitorCore::SensorData * sd, HydroMonitorLogging * l, Adafruit_MCP23008 * mcp23008) {
  mcp = mcp23008;
  mcp->pinMode(RESERVOIR_MCP_PIN, INPUT);
  l->writeTrace(F("HydroMonitorReservoir: configured reservoir refill on MCP23008 port expander."));

  /*
     Set up the solenoid, connected to a PCF8574 port expander.
  */
#elif defined(RESERVOIR_PCF_PIN)
void HydroMonitorReservoir::begin(HydroMonitorCore::SensorData * sd, HydroMonitorLogging * l, PCF857x * pcf) {
  pcf8574 = pcf;
  pcf8574->pinMode(RESERVOIR_PCF_PIN, INPUT);
  l->writeTrace(F("HydroMonitorReservoir: configured reservoir refill on PCF8574 port expander."));

  /*
     Set up the solenoid, connected to a GPIO port.
  */
#elif defined(RESERVOIR_PIN)
void HydroMonitorReservoir::begin(HydroMonitorCore::SensorData * sd, HydroMonitorLogging * l) {
  pinMode(RESERVOIR_PIN, INPUT);
  l->writeTrace(F("HydroMonitorReservoir: configured reservoir refill."));
#endif
  bitSet(sd->systemStatus, STATUS_RESERVOIR_DRAINED); // We don't know the reservoir level: assume empty & start filling.
#endif
  sensorData = sd;
  logging = l;
  closeValve();  // Make sure it's closed.
  if (RESERVOIR_EEPROM > 0)
#ifdef USE_24LC256_EEPROM
    sensorData->EEPROM->get(RESERVOIR_EEPROM, settings);
#else
    EEPROM.get(RESERVOIR_EEPROM, settings);
#endif

  if (settings.maxFill > 100) {
    logging->writeTrace(F("HydroMonitorReservoir: applying default settings."));
    settings.maxFill = 90;
    settings.minFill = 70;
#ifdef USE_24LC256_EEPROM
    sensorData->EEPROM->put(RESERVOIR_EEPROM, settings);
#else
    EEPROM.put(RESERVOIR_EEPROM, settings);
    EEPROM.commit();
#endif
  }
}

/*
   doReservoir - manages the switching of the solenoid and adding the water to the reservoir.

   This has to be called as frequent as possible for proper timing and preventing overfilling.
*/
void HydroMonitorReservoir::doReservoir() {

  floatswitchTriggered = false;

#ifdef LEVEL_LIMIT_MCP17_PIN
  // Check for the float switch being triggered.
  if (bitRead(sensorData->systemStatus, STATUS_FILLING_RESERVOIR)) {
    if (mcp->digitalRead(LEVEL_LIMIT_MCP17_PIN) == LOW) {   // It should be pulled high while filling, unless triggered.
      floatswitchTriggered = true;
    }
  }
  else {
    if (mcp->digitalRead(RESERVOIR_MCP17_PIN) == HIGH) {    // It should be pulled low, unless triggered.
      floatswitchTriggered = true;
    }
  }
  
  if (floatswitchTriggered) {
#ifndef USE_WATERLEVEL_SENSOR
    bitSet(sensorData->systemStatus, STATUS_DRAINAGE_NEEDED); // Trigger drainage - if not using water level sensor, and retrigger until it's resolved.
#endif
    if (millis() - lastBeep > BEEP_FREQUENCY) {
      lastBeep += BEEP_FREQUENCY;
    }
    if (millis() - lastBeep < BEEP_DURATION) {
      beep = true;
    }
    else {
      beep = false;
    }
    if (millis() - lastWarned > WARNING_INTERVAL) {
      logging->writeError(F("Reservoir 10: float switch triggered, reservoir water level is critically high."));
      lastWarned = millis();
    }
  }
  else {
    beep = false;
  }
  if (beep != oldBeep) {
    mcp->digitalWrite(AUX1_MCP17_PIN, beep);
    oldBeep = beep;
  }
#endif

  if (bitRead(sensorData->systemStatus, STATUS_DRAINING_RESERVOIR) ||
      bitRead(sensorData->systemStatus, STATUS_WATERING) || // System is doing something that prevents us from filling the reservoir,
      bitRead(sensorData->systemStatus, STATUS_DOOR_OPEN) ||
      bitRead(sensorData->systemStatus, STATUS_MAINTENANCE) || 
      floatswitchTriggered) {                               // or it's too full already.
    if (bitRead(sensorData->systemStatus, STATUS_FILLING_RESERVOIR)) { // If we are currently adding water; stop this.
      closeValve();
    }
  }

#ifdef USE_WATERLEVEL_SENSOR                                // Water level based reservoir level management.
  else if (sensorData->waterLevel < 0                       // Water level sensor not connected, or out of range,
      && millis() - reservoirEmptyTime > 30 * 1000          // and we're empty for >half a minute,
      && initialFillingDone == false) {                     // and we didn't try adding some water yet:
    initialFillingDone = true;                              // Mark that we started trying.
    initialFillingInProgress = true;                        // It's in progress.
    openValve();
    startAddWater = millis();
    lastLevelCheck = millis();
    waterLevelSensor->readSensor(readNow = true);
    logging->writeTrace(F("HydroMonitorReservoir: No water level detected for half a minute, opening water inlet valve for 30 seconds to try and get the water level sensor to react."));
  }
  else if (initialFillingInProgress) {                      // We're trying to add some water to the reservoir.
    if (millis() - lastLevelCheck > 500) {                  // Check the sensor every 0.5 seconds.
      lastLevelCheck += 500;
      waterLevelSensor->readSensor(readNow = true);
    }
    if (millis() - startAddWater > 30 * 1000                // After 30 seconds, or:
        || sensorData->waterLevel > 0) {                    // if we actually have a reading, we can stop this.
      initialFillingInProgress = false;
      closeValve();
      logging->writeTrace(F("HydroMonitorReservoir: Initial filling done."));
    }
  }
  else {
    if (bitRead(sensorData->systemStatus, STATUS_FILLING_RESERVOIR)) { // Reservoir is being filled.
      if (millis() - lastLevelCheck > 500) {                // Measure the reservoir fill every 0.5 seconds.
        lastLevelCheck += 500;
        waterLevelSensor->readSensor(readNow = true);
      }
      if (sensorData->waterLevel > settings.maxFill) {      // If we have enough water in the reservoir, close the valve.
        closeValve();
        logging->writeTrace(F("HydroMonitorReservoir: water level high enough, closing the valve."));
      }
      if (millis() - startAddWater > 3 * 60 * 1000) {       // As extra safety measure: close the valve after 3 minutes, regardless of what the water level sensor says.
        closeValve();
        logging->writeWarning(F("HydroMonitorReservoir: added water for 3 minutes, high level not reached, timeout: closing the valve."));
        lastGoodFill = millis() + 60 * 60 * 1000;           // Call it a good fill, and set the time an hour in the future: no trying to fill before that time.
      }
    }
    else {
      if (sensorData->waterLevel > settings.minFill &&      
          bitRead(sensorData->systemStatus, STATUS_FILLING_RESERVOIR) == false) {

        // As long as the water level is above the set minimum and we're not adding water now,
        // there's nothing to do here other than keeping track of when this was.
        lastGoodFill = millis();
        initialFillingDone = true;                          // We're good.
      }
      else if (millis() - startAddWater < 30 * 60 * 1000) { // Less than 30 minutes after filling we're low already? That's odd!
        if (millis() - lastWarned > WARNING_INTERVAL) {
          lastWarned = millis();
          logging->writeError(F("Reservoir 11: shortly after the reservoir was topped up, water level dropped below the minimum already."));
        }
      }
      else if (millis() - lastGoodFill > 1 * 60 * 1000 &&   // If water too low for more than 1 minute,
               sensorData->waterLevel > 0) {                // and the water sensor actually gives a reading, start filling.
        logging->writeTrace(F("HydroMonitorReservoir: water level too low for 1 minute, opening the valve."));
        logging->writeInfo(F("HydroMonitorReservoir: adding water to the reservoir."));
        openValve();
        lastLevelCheck = millis();
        startAddWater = millis();
      }
    }
  }
  if (bitRead(sensorData->systemStatus, STATUS_DRAINING_RESERVOIR) &&
      sensorData->waterLevel < 0) {                         // Water sensor can't measure as a result of draining.
    initialFillingDone = false;                             // We will have to do the initial filling procedure
    reservoirEmptyTime = millis();                          // when the draining is completed.
  }
  if (sensorData->waterLevel > settings.maxFill + 5 &&
      millis() - startAddWater < 5 * 60 * 1000) {
    logging->writeError(F("Reservoir 12: water level >5% over maxFill within 5 minutes of starting to fill the reservoir. Suspected problem with the filling system."));
  }  
  if (sensorData->waterLevel > 100) {                       // Very full reservoir; should have been drained already.
    if (millis() - lastWarned > WARNING_INTERVAL) {
      lastWarned = millis();
      char buff[70];
      sprintf_P(buff, PSTR("HydroMonitorReservoir: water level too high: current level %3.1f%."), sensorData->waterLevel);
      logging->writeWarning(buff);
    }
  }
#else
  else if (bitRead(sensorData->systemStatus, STATUS_RESERVOIR_DRAINED)) { // Reservoir draining is completed, and no other flags blocking us from refilling.
    openValve();
    bitClear(sensorData->systemStatus, STATUS_RESERVOIR_DRAINED); // We're filling, so not drained any more. Clear the flag.
    startAddWater = millis();
    isWeeklyTopUp = false;
    logging->writeTrace(F("HydroMonitorReservoir: Reservoir empty after draining; filling with water."));
  }
  else if (millis() - startAddWater > 7 * 24 * 60 * 60 * 1000) { // Every 7 days: do a reservoir top-up.
    openValve();
    bitClear(sensorData->systemStatus, STATUS_RESERVOIR_DRAINED); // We're filling, so not drained any more. Clear the flag.
    startAddWater = millis();
    isWeeklyTopUp = true;
    logging->writeTrace(F("HydroMonitorReservoir: Doing weekly reservoir top-up."));
  }
  else if (bitRead(sensorData->systemStatus, STATUS_FILLING_RESERVOIR)) { // Reservoir is being filled.
    if ((isWeeklyTopUp && millis() - startAddWater > 3 * 60 * 1000) || // Weekly top-up for 3 minutes, or
        millis() - startAddWater > 20 * 60 * 1000) {        // 20 minutes for a complete fill.
    closeValve();
    bitClear(sensorData->systemStatus, STATUS_RESERVOIR_LEVEL_LOW); // It's for sure filled up now.
    logging->writeInfo(F("HydroMonitorReservoir: finished adding water, closing the valve."));
    }
  }
#endif
}

/*
   Functions to open (output power on) and close (output power off) the valve.
   Also set the pin to INPUT resp. OUTPUT mode: this to allow for reading the float switch, if used.
*/
void HydroMonitorReservoir::openValve() {
#ifdef RESERVOIR_PIN
  pinMode(RESERVOIR_PIN, OUTPUT);
  digitalWrite(RESERVOIR_PIN, HIGH);
#elif defined(RESERVOIR_PCF_PIN)
  pcf8574->pinMode(RESERVOIR_PCF_PIN, OUTPUT);
  pcf8574->write(RESERVOIR_PCF_PIN, LOW);
#elif defined(RESERVOIR_MCP_PIN)
  mcp->pinMode(RESERVOIR_MCP_PIN, OUTPUT);
  mcp->digitalWrite(RESERVOIR_MCP_PIN, HIGH);
#elif defined(RESERVOIR_MCP17_PIN)
  mcp->pinMode(RESERVOIR_MCP17_PIN, OUTPUT);
  mcp->digitalWrite(RESERVOIR_MCP17_PIN, HIGH);
#endif
  bitSet(sensorData->systemStatus, STATUS_FILLING_RESERVOIR);
  return;
}

void HydroMonitorReservoir::closeValve() {
#ifdef RESERVOIR_PIN
  digitalWrite(RESERVOIR_PIN, LOW);
  pinMode(RESERVOIR_PIN, INPUT);
#elif defined(RESERVOIR_PCF_PIN)
  pcf8574->write(RESERVOIR_PCF_PIN, HIGH);
  pcf8574->pinMode(RESERVOIR_PCF_PIN, INPUT);
#elif defined(RESERVOIR_MCP_PIN)
  mcp->digitalWrite(RESERVOIR_MCP_PIN, LOW);
  mcp->pinMode(RESERVOIR_MCP_PIN, INPUT);
#elif defined(RESERVOIR_MCP17_PIN)
  mcp->digitalWrite(RESERVOIR_MCP17_PIN, LOW);
  mcp->pinMode(RESERVOIR_MCP17_PIN, INPUT);
#endif
  bitClear(sensorData->systemStatus, STATUS_FILLING_RESERVOIR);
  return;
}

/*
   The settings as html.
*/
String HydroMonitorReservoir::settingsHtml() {
  String html = F("\
      <tr>\n\
        <th colspan=\"2\">Reservoir levels.</th>\n\
      </tr><tr>\n\
        <td>Minimum fill level:</td>\n\
        <td><input type=\"number\" step=\"1\" name=\"reservoir_minfill\" value=\"");
  html += String(settings.minFill);
  html += F("\"> %.</td>\n\
      </tr><tr>\n\
        <td>Maximum fill level:</td>\n\
        <td><input type=\"number\" step=\"1\" name=\"reservoir_maxfill\" value=\"");
  html += String(settings.maxFill);
  html += F("\"> %.</td>\n\
      </tr>\n");
  return html;
}

/*
   Process the settings from the key/value pairs.
*/
void HydroMonitorReservoir::updateSettings(String keys[], String values[], uint8_t nArgs) {
  for (uint8_t i = 0; i < nArgs; i++) {
    if (keys[i] == "reservoir_minfill") {
      if (core.isNumeric(values[i])) {
        uint8_t val = values[i].toInt();
        if (val <= 99) settings.minFill = val; // maxLevel must be larger, so 100% is not allowed.
      }
    }
  }
  for (uint8_t i = 0; i < nArgs; i++) {
    if (keys[i] == "reservoir_minfill") {
      if (core.isNumeric(values[i])) {
        uint8_t val = values[i].toInt();
        if (val > settings.minFill && val <= 100) settings.minFill = val;
      }
    }
  }
#ifdef USE_24LC256_EEPROM
  sensorData->EEPROM->put(RESERVOIR_EEPROM, settings);
#else
  EEPROM.put(RESERVOIR_EEPROM, settings);
  EEPROM.commit();
#endif
  return;
}
#endif
