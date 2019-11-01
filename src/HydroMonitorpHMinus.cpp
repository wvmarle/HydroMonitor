#include <HydroMonitorpHMinus.h>

#ifdef USE_PHMINUS

#ifndef USE_PH_SENSOR
#error Can't do pH-minus dosing without pH sensor.
#endif


/*
   Handle the peristaltic pump that adds pH-minus solution to the system.
*/
HydroMonitorpHMinus::HydroMonitorpHMinus() {
  runTime = 0;
  pHDelay = 30 * 60 * 1000; // Half an hour delay after adding fertiliser, to allow the system to mix properly.
  lastTimeAdded = -pHDelay; // When starting up, don't apply the delay.
  addpH = false;
  lastWarned = millis() - WARNING_INTERVAL;
}

/*
   Setup the pH minus pump.
   This is used for when the pump is connected to the MCP23008/MCP23017 port expander.
*/
#ifdef PHMINUS_MCP_PIN
void HydroMonitorpHMinus::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l, Adafruit_MCP23008 *mcp23008) {
  mcp = mcp23008;
  mcp->pinMode(PHMINUS_MCP_PIN, OUTPUT);
  l->writeTrace(F("HydroMonitorpHMinus: configured pH-minus adjuster on MCP23008 port expander."));

#elif defined(PHMINUS_MCP17_PIN)
void HydroMonitorpHMinus::begin(HydroMonitorCore::SensorData * sd, HydroMonitorLogging * l, Adafruit_MCP23017 * mcp23017) {
  mcp = mcp23017;
  mcp->pinMode(PHMINUS_MCP17_PIN, OUTPUT);
  l->writeTrace(F("HydroMonitorpHMinus: configured pH-minus adjuster on MCP23017 port expander."));

  /*
     Setup the pH minus pump.
     This is used for when the pump is connected to the PCF8574 port expander.
  */
#elif defined(PHMINUS_PCF_PIN)
void HydroMonitorpHMinus::begin(HydroMonitorCore::SensorData * sd, HydroMonitorLogging * l, PCF857x * pcf) {
  pcf8574 = pcf;
  pcf8574->pinMode(PHMINUS_PCF_PIN, OUTPUT);
  l->writeTrace(F("HydroMonitorpHMinus: configured pH-minus adjuster on PCF8574 port expander."));

  /*
     Setup the pH minus pump - direct connection.
  */
#elif defined(PHMINUS_PIN)
void HydroMonitorpHMinus::begin(HydroMonitorCore::SensorData * sd, HydroMonitorLogging * l) {
  pinMode(PHMINUS_PIN, OUTPUT);
  l->writeTrace(F("HydroMonitorpHMinus: configured pH-minus adjuster."));
#endif

  sensorData = sd;
  logging = l;
  switchPumpOff();
  if (PHMINUS_EEPROM > 0)
#ifdef USE_24LC256_EEPROM
    sensorData->EEPROM->get(PHMINUS_EEPROM, settings);
#else
    EEPROM.get(PHMINUS_EEPROM, settings);
#endif

  // Check whether any settings have been set, if not apply defaults.
  if (settings.pumpSpeed < 0 || settings.pumpSpeed > 200) {
    l->writeTrace(F("HydroMonitorpHMinus: applying default settings."));
    settings.pumpSpeed = 20;      // ml per minute.
#ifdef USE_24LC256_EEPROM
    sensorData->EEPROM->put(PHMINUS_EEPROM, settings);
#else
    EEPROM.put(PHMINUS_EEPROM, settings);
    EEPROM.commit();
#endif
  }
}

/*
   The pH-minus handling.
   This has to be called very frequently.

   Step 0: check whether a flow measuring is in progress, and switch the pump off after 60 seconds.
   Step 1: check whether we've recently added pH adjuster. If so, return instantly.
   Step 2: check whether we are adding it and if so, whether it's enough,
      if not: check whether the current pH is less than 0.2 points above target pH,
      if not: check whether the last time we had low enough pH was more than 10 minutes ago,
              and if so, calculate how much pH adjuster has to be added, and start the pump.
*/

void HydroMonitorpHMinus::dopH() {

  // If we're measuring the pump speed, switch it off after 60 seconds.
  if (measuring) {
    if (millis() - startTime > 60 * 1000) {
      logging->writeTrace(F("HydroMonitorpHMinus: measuring pump finished."));
      switchPumpOff();
      measuring = false;
      running = false;
    }

    // Don't try to do anything else while measuring.
    return;
  }

  // All parameters must have been set.
  // The user may set the targetpH to 0 to stop this process.
  if (sensorData->targetpH == 0 ||
      sensorData->solutionVolume == 0 ||
      sensorData->pHMinusConcentration == 0) {
    return;
  }

  // Don't start adding pH minus solution if the reservoir level is low.
  if (bitRead(sensorData->systemStatus, STATUS_RESERVOIR_LEVEL_LOW) ||
      bitRead(sensorData->systemStatus, STATUS_DRAINING_RESERVOIR) ||
      bitRead(sensorData->systemStatus, STATUS_MAINTENANCE) ||
      bitRead(sensorData->systemStatus, STATUS_DRAINAGE_NEEDED)) {
    return;
  }

  // Check whether pH-minus is running, and if so whether it's time to stop.
  if (running) {
    if (millis() - startTime > runTime) {
      logging->writeTrace(F("HydroMonitorpHMinus: finished adding pH-minus; switching off the pump."));
      switchPumpOff();
      running = false;
      lastTimeAdded = millis();
      return;
    }
  }

  // Check whether we have to do anything at all, or that we're in the delay time.
  else if (millis() - lastTimeAdded < pHDelay) {
    return;
  }

  // pH way out of range - don't try to correct.
  else if (sensorData->pH > 9) {
    lastTimeAdded = millis() + pHDelay;
    return;
  }

  // Check whether we have a pH value that's less than 0.2 points above target value,
  // or whether the current EC itself is too low.
  // Keep the time we saw this good value.
  else if (sensorData->pH - sensorData->targetpH < 0.2 ||   // Measured pH is less than 0.2 points higher than the target.
           sensorData->EC - sensorData->targetEC < -0.3) {  // Measured EC is more than 0.3 points lower than the target.
    lastGoodpH = millis();
    return;
  }

  // If more than 10 minutes since lastGoodpH, add 0.2 pH points worth of pH-minus.
  else if (millis() - lastGoodpH > 10 * 60 * 1000) {
    float addVolume = 0.2 * sensorData->solutionVolume * sensorData->pHMinusConcentration; // The amount of fertiliser in ml to be added.
    runTime = (addVolume / settings.pumpSpeed) * 60 * 1000; // the time in milliseconds pump A has to run.
    logging->writeTrace(F("HydroMonitorpHMinus: 10 minutes of too high pH; start adding pH-minus."));
    char buff[90];
    sprintf_P(buff, PSTR("HydroMonitorpHMinus: running pump for %i ms to add %3.1f ml of pH- solution."), runTime, addVolume);
    logging->writeInfo(buff);
    switchPumpOn();                                         // Start the pump.
    running = true;                                         // Flag it's running.
    startTime = millis();                                   // Keep track of since when it's running.
    originalpH = sensorData->pH;
  }

  if (millis() - startTime  < 30 * 60 * 1000) {             // Monitor pH for 30 minutes after last time added; it should be going up now.
    if (sensorData->pH < originalpH - 0.1) {
      originalpH = 14;
    }
  }
  else if (originalpH < 14 && millis() - lastWarned > WARNING_INTERVAL) {
    logging->writeWarning(F("pHMinus 01: pH did not go down as expected after running the pump. pH- bottle may be empty."));
    lastWarned = millis();
  }
}

/*
   Functions to switch the pump on and off.
*/
void HydroMonitorpHMinus::switchPumpOn() {
#ifdef PHMINUS_PIN
  digitalWrite(PHMINUS_PIN, HIGH);
#elif defined(PHMINUS_PCF_PIN)
  pcf8574->write(PHMINUS_PCF_PIN, LOW);
#elif defined(PHMINUS_MCP_PIN)
  mcp->digitalWrite(PHMINUS_MCP_PIN, HIGH);
#elif defined(PHMINUS_MCP17_PIN)
  mcp->digitalWrite(PHMINUS_MCP17_PIN, HIGH);
#endif
}

void HydroMonitorpHMinus::switchPumpOff() {
#ifdef PHMINUS_PIN
  digitalWrite(PHMINUS_PIN, LOW);
#elif defined(PHMINUS_PCF_PIN)
  pcf8574->write(PHMINUS_PCF_PIN, HIGH);
#elif defined(PHMINUS_MCP_PIN)
  mcp->digitalWrite(PHMINUS_MCP_PIN, LOW);
#elif defined(PHMINUS_MCP17_PIN)
  mcp->digitalWrite(PHMINUS_MCP17_PIN, LOW);
#endif
}

// Measure the pump speed.
// This is called via the web interface or the app interface.
void HydroMonitorpHMinus::measurePump() {
  if (!running) { // Don't do anything if the pump is running already.
    logging->writeTrace(F("HydroMonitorpHMinus: switching on pH- pump."));
    switchPumpOn();
    startTime = millis();
    running = true;
    measuring = true;
  }
}

/*
   HTML code to set the various settings.
*/
void HydroMonitorpHMinus::settingsHtml(ESP8266WebServer * server) {
  server->sendContent_P(PSTR("\
      <tr>\n\
        <th colspan=\"2\">pH adjuster settings.</th>\n\
      </tr><tr>\n\
        <td>Speed of pump:</td>\n\
        <td><input type=\"number\" step=\"0.1\" name=\"ph_pumpspeed\" value=\""));
  char buff[10];
  sprintf(buff, "%.2f", settings.pumpSpeed);
  server->sendContent(buff);
  server->sendContent_P(PSTR("\"> ml/minute.&nbsp;&nbsp;<input type=\"submit\" formaction=\"/measure_pump_phminus_speed\" formmethod=\"post\" name=\"phpump\" value=\"Measure now\"></td>\n\
      </tr>\n"));
}

/*
   HTML code to set the various settings.
*/
bool HydroMonitorpHMinus::settingsJSON(ESP8266WebServer * server) {
  char buff[10];
  server->sendContent_P(PSTR("  \"ph_minus\": {\n"
                             "    \"pump_speed\":\""));
  sprintf(buff, "%.2f", settings.pumpSpeed);
  server->sendContent(buff);
  server->sendContent_P(PSTR("\"\n"
                             "  }"));
  return true;
}

/*
   Update the settings.
*/
void HydroMonitorpHMinus::updateSettings(ESP8266WebServer * server) {
  for (uint8_t i = 0; i < server->args(); i++) {
    if (server->argName(i) == "ph_pumpspeed") {
      if (core.isNumeric(server->arg(i))) {
        float val = server->arg(i).toFloat();
        if (val > 0) {
          settings.pumpSpeed = val;
        }
      }
      continue;
    }
  }
#ifdef USE_24LC256_EEPROM
  sensorData->EEPROM->put(PHMINUS_EEPROM, settings);
#else
  EEPROM.put(PHMINUS_EEPROM, settings);
  EEPROM.commit();
#endif
}
#endif

