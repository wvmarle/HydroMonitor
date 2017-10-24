#include <HydroMonitorpHMinus.h>

#ifdef USE_PHMINUS

#ifndef USE_PH_SENSOR
#error Can't do pH-minus dosing without pH sensor.
#endif


/*
 * Handle the peristaltic pump that adds pH-minus solution to the system.
 */
HydroMonitorpHMinus::HydroMonitorpHMinus() {
  runTime = 0;
  pHDelay = 30 * 60 * 1000; // Half an hour delay after adding fertiliser, to allow the system to mix properly.
  lastTimeAdded = -pHDelay; // When starting up, don't apply the delay.
  addpH = false;
}

/*
 * Setup the pH minus pump.
 * This is used for when the pump is connected to the MCP23008 port expander.
 */
#ifdef PHMINUS_MCP_PIN
void HydroMonitorpHMinus::begin(HydroMonitorCore::SensorData *sd, HydroMonitorMySQL *l, Adafruit_MCP23008 *mcp) {
  mcp23008 = mcp;
  mcp23008->pinMode(PHMINUS_MCP_PIN, OUTPUT);
  l->writeTesting("HydroMonitorpHMinus: configured pH-minus adjuster on MCP port expander.");

/*
 * Setup the pH minus pump.
 * This is used for when the pump is connected to the PCF8574 port expander.
 */
#elif defined(PHMINUS_PCF_PIN)
void HydroMonitorpHMinus::begin(HydroMonitorCore::SensorData *sd, HydroMonitorMySQL *l, PCF857x *pcf) {
  pcf8574 = pcf;
  l->writeTesting("HydroMonitorpHMinus: configured pH-minus adjuster on PCF port expander.");

/*
 * Setup the pH minus pump - connected to a GPIO pin.
 */
#elif defined(PHMINUS_PIN)
void HydroMonitorpHMinus::begin(HydroMonitorCore::SensorData *sd, HydroMonitorMySQL *l) {
  pinMode(PHMINUS_PIN, OUTPUT);
  l->writeTesting("HydroMonitorpHMinus: configured pH-minus adjuster.");
#endif

  sensorData = sd;
  logging = l;
  switchPumpOff();
  if (PHMINUS_EEPROM > 0)
    EEPROM.get(PHMINUS_EEPROM, settings);
  
  // Check whether any settings have been set, if not apply defaults.
  if (settings.pumpSpeed < 0 || settings.pumpSpeed > 500) {
      l->writeTesting("HydroMonitorpHMinus: applying default settings.");
      settings.pumpSpeed = 100;      // ml per minute.
  }
  return;
}

/*
 * The pH-minus handling.
 * This has to be called very frequently.
 *
 * Step 0: check whether a flow measuring is in progress, and switch the pump off after 60 seconds.
 * Step 1: check whether we've recently added pH adjuster. If so, return instantly.
 * Step 2: check whether we are adding it and if so, whether it's enough,
 *    if not: check whether the current pH is less than 0.2 points above target pH,
 *    if not: check whether the last time we had low enough pH was more than 10 minutes ago,
 *            and if so, calculate how much pH adjuster has to be added, and start the pump.
 */
void HydroMonitorpHMinus::dopH() {

  // If we're measuring the pump speed, switch it off after 60 seconds.
  if (measuring) {
    if (millis() - startTime > 60 * 1000) {
      switchPumpOff();
      measuring = false;
      running = false;
    }
    
    // Don't try to do anything else while measuring.
    return;
  }
  
  // All parameters must have been set.
  // The user may set the targetpH to 0 to stop this process.
  if (sensorData->targetpH == 0 || sensorData->solutionVolume == 0 || sensorData->pHMinusConcentration == 0) return;

  // Check whether pH-minus is running, and if so whether it's time to stop.
  if (running && millis() - startTime > runTime) {
    logging->writeTesting("HydroMonitorpHMinus: finished adding pH-minus; switching off the pump.");
    switchPumpOff();
    running = false;
    lastTimeAdded = millis();
    return;
  }

  // Check whether we have to do anything at all, or that we're in the delay time.
  else if (millis() - lastTimeAdded < pHDelay) return;
    
  // Check whether we have a pH value that's less than 0.2 points above target value.
  // Keep the time we saw this good value.
  else if (sensorData->pH - sensorData->targetpH < 0.2) {
    lastGoodpH = millis();
    return;
  }

  // If more than 10 minutes since lastGoodpH, add 0.2 pH points worth of pH-minus.
  else if (millis() - lastGoodpH > 10 * 60 * 1000) {
    float addVolume = 0.2 * sensorData->solutionVolume * sensorData->pHMinusConcentration; // The amount of fertiliser in ml to be added.
    runTime = addVolume / settings.pumpSpeed * 60 * 1000; // the time in milliseconds pump A has to run.
    logging->writeTesting("HydroMonitorpHMinus: 10 minutes of too low pH; start adding pH-minus.");
    /*
    String message = F("Measured pH: ");
    message += currentpH;
    message += F(", target pH: ");
    message += targetpH;
    message += F(", adding ");
    message += addVolume;
    message += F(" ml of pH-minus.");
    logging->writeTesting(message);
    message = F("Required pump runtime: ");
    message += runTime;
    message += F("ms.");
    logging->writeTesting(message);
    */
    switchPumpOn();       // Start the pump.
    running = true;       // Flag it's running.
    startTime = millis(); // Keep track of since when it's running.
  }
}

/*
 * Functions to switch the pump on and off.
 */
#ifdef PHMINUS_PIN
void HydroMonitorpHMinus::switchPumpOn() {
  digitalWrite(PHMINUS_PIN, HIGH);
  return;
}

// Switch the pump off.
void HydroMonitorpHMinus::switchPumpOff() {
  digitalWrite(PHMINUS_PIN, LOW);
  return;
}
#elif defined(PHMINUS_PCF_PIN)
void HydroMonitorpHMinus::switchPumpOn() {
  pcf8574->write(PHMINUS_PCF_PIN, LOW);
  return;
}

// Switch the pump off.
void HydroMonitorpHMinus::switchPumpOff() {
  pcf8574->write(PHMINUS_PCF_PIN, HIGH);
  return;
}
#elif defined(PHMINUS_MCP_PIN)
void HydroMonitorpHMinus::switchPumpOn() {
  mcp23008->digitalWrite(PHMINUS_MCP_PIN, HIGH);
  return;
}

// Switch the pump off.
void HydroMonitorpHMinus::switchPumpOff() {
  mcp23008->digitalWrite(PHMINUS_MCP_PIN, HIGH);
  return;
}
#endif

// Measure the pump speed.
// This is called via the web interface or the app interface.
void HydroMonitorpHMinus::measurePump() {
  if (!running) { // Don't do anything if the pump is running already.
    switchPumpOn();
    startTime = millis();
    running = true;
    measuring = true;
  }
}

/*
 * HTML code to set the various settings.
 */
String HydroMonitorpHMinus::settingsHtml() {
  String html;
  html = F("\
      <tr>\n\
        <th colspan=\"2\">pH adjuster settings.</th>\n\
      </tr><tr>\n\
        <td>Speed of pump:</td>\n\
        <td><input type=\"number\" step=\"0.1\" name=\"ph_pumpspeed\" value=\"");
  html += String(settings.pumpSpeed);
  html += F("\"> ml/minute.&nbsp;&nbsp;<input type=\"submit\" formaction=\"/measure_pump_phminus_speed\" formmethod=\"post\" name=\"phpump\" value=\"Measure now\"></td>\n\
      </tr>\n");
  
  return html;
}

/*
 * Update the settings.
 */
void HydroMonitorpHMinus::updateSettings(String keys[], String values[], uint8_t nArgs) {
  for (uint8_t i=0; i<nArgs; i++) {
    if (keys[i] == "ph_pumpspeed") {
      if (core.isNumeric(values[i])) {
        float val = values[i].toFloat();
        if (val > 0) settings.pumpSpeed = val;
      }
      continue;
    }
  }
  EEPROM.put(PHMINUS_EEPROM, settings);
  EEPROM.commit();
  return;
}
#endif

