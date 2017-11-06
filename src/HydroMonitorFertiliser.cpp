#include <HydroMonitorFertiliser.h>

#ifdef USE_FERTILISER

#ifndef USE_EC_SENSOR
#error Can't do fertiliser dosing without EC sensor!
#endif

/*
 * Add fertiliser to the system as needed, based on the current EC value.
 *
 * Constructor.
 */
HydroMonitorFertiliser::HydroMonitorFertiliser() {
  runBTime = 0;
  runATime = 0;
  fertiliserDelay = 30 * 60 * 1000; // Half an hour delay after adding fertiliser, to allow the system to mix properly.
  lastTimeAdded = -fertiliserDelay;
  addA = false;
  addB = false;
}

/*
 * Configure the module.
 */
#ifdef FERTILISER_A_MCP_PIN
void HydroMonitorFertiliser::begin(HydroMonitorCore::SensorData *sd, HydroMonitorMySQL *l, Adafruit_MCP23008 *mcp) {
  mcp23008 = mcp;
  pumpA = FERTILISER_A_MCP_PIN;
  pumpB = FERTILISER_B_MCP_PIN;
  mcp23008->pinMode(pumpA, OUTPUT);
  mcp23008->pinMode(pumpB, OUTPUT);
  l->writeTesting("HydroMonitorFertiliser: set up fertiliser pumps on MCP port expander.");

#elif FERTILISER_A_PCF_PIN
void HydroMonitorFertiliser::begin(HydroMonitorCore::SensorData *sd, HydroMonitorMySQL *l, PCF857x *pcf) {
  pcf8574 = pcf;
  pumpA = FERTILISER_A_PCF_PIN;
  pumpB = FERTILISER_B_PCF_PIN;
  l->writeTesting("HydroMonitorFertiliser: set up fertiliser pumps on PCF port expander.");

#elif FERTILISER_A_PIN
void HydroMonitorFertiliser::begin(HydroMonitorCore::SensorData *sd, HydroMonitorMySQL *l) {
  pumpA = FERTILISER_A_PIN;
  pumpB = FERTILISER_B_PIN;
  pinMode(pumpA, OUTPUT);
  pinMode(pumpB, OUTPUT);
  l->writeTesting("HydroMonitorFertiliser: set up fertiliser pumps with direct port connection.");
#endif

  sensorData = sd;
  logging = l;
  switchPumpOff(pumpA);
  switchPumpOff(pumpB);
  if (FERTILISER_EEPROM > 0)
    EEPROM.get(FERTILISER_EEPROM, settings);

/*
  // Check whether any settings have been set, if not apply defaults.
  if (settings.pumpASpeed < 1 || settings.pumpASpeed > 500) {
    logging->writeDebug("HydroMonitorFertiliser: applying default settings.");
    settings.pumpASpeed = 200;
    settings.pumpBSpeed = 200;
    EEPROM.put(FERTILISER_EEPROM, settings);
    EEPROM.commit();
  }
*/
  logging->writeInfo("HydroMonitorFertiliser: set up fertiliser pumps.");
  return;
}

/*
 * The fertiliser handling.
 * This has to be called frequently, to prevent delays in the switching.
 *
 * Step 1: check whether we've recently added fertiliser. If so, return instantly.
 * Step 2: check whether we have to start adding B,
 *    if not: check whether we are adding B and if so, whether it's enough,
 *    if not: check whether we have to start adding A,
 *    if not: check whether we are adding A and if so, whether it's enough,
 *    if not: check whether the current EC is higher than 5% below the target EC,
 *    if not: check whehter the last time we had high enough EC was more than 10 minutes ago,
 *            and if so, set the flags to start adding fertiliser, and calculate how much.
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
        logging->writeDebug("HydroMonitorFertiliser: measuring pump A finished.");
        switchPumpOff(pumpA);
        measuring = false;
        aRunning = false;
      }
    }
    else if (millis() - runBTime > 60 * 1000) {
      logging->writeDebug("HydroMonitorFertiliser: measuring pump B finished.");
      switchPumpOff(pumpB);
      measuring = false;
      bRunning = false;
    }
    
    // Don't do anything else while measuring!
    return;
  }
  
  // Don't start adding fertiliser if the sensor is not connected, or if calibration is not set (in which case
  // the value is nan, and any comparison results in false).
  if (sensorData->EC <= 0) return;

  // All parameters must have been set.
  // The user may set the targetpH to 0 to stop this process.
  if (sensorData->targetEC == 0 || sensorData->solutionVolume == 0 || sensorData->fertiliserConcentration == 0) return;

  // Add solution B, if needed.
  // It's best to start with B first, and do A second.
  if (addB) {
    logging->writeTesting("HydroMonitorFertiliser: start adding solution B, switching pump B on.");
    switchPumpOn(pumpB);    // Switch on the pump,
    bRunning = true;        // and flag it's running.
    addB = false;           // Reset this flag, as we're adding B now.
    startTime = millis();   // The time the addition started.
  }
  
  // Check whether B is running, and if so whether it's time to stop.
  else if (bRunning && millis() - startTime > runBTime) {
    logging->writeTesting("HydroMonitorFertiliser: finished adding solution B, switching pump B off.");
    switchPumpOff(pumpB);   // Switch off the pump.
    startATime = millis();  // Record when we did this, after a short delay A may start.
    bRunning = false;       // Reset the running flag.
  }
  
  // Start adding A if needed and B is not running and the 500 ms break is over.
  else if (addA && !bRunning && millis() - startATime > 500) {
    logging->writeTesting("HydroMonitorFertiliser: start adding solution A, switching pump A on.");
    switchPumpOn(pumpA);    // Switch on the pump,
    aRunning = true;        // and flag it's running.
    addA = false;           // Reset this flag, as we're adding A now.
    startTime = millis();   // The time the addition started.
  }
  
  // Check whether A is running, and if so whether it's time to stop.
  else if (aRunning && millis() - startTime > runATime) {
    logging->writeTesting("HydroMonitorFertiliser: finished adding solution A, switching pump A off.");
    switchPumpOff(pumpA);   // Switch off the pump.
    aRunning = false;       // Reset the running flag.
  }

  // Check whether the EC is not more than 0.05 mS/cm under the target value, and if so 
  // record the time when we found the EC to be good.
  if (sensorData->EC > sensorData->targetEC - 0.05) lastGoodEC = millis();

  // Only start adding after ten minutes of continuous too low EC, and at least fertiliserDelay
  // since we last added any fertiliser.
  else if (millis() - lastGoodEC > 10 * 60 * 1000 && millis() - lastTimeAdded > fertiliserDelay) {
  
    // Flag that we want to add fertilisers - both, of course.
    addA = true;
    addB = true;
    logging->writeInfo("HydroMonitorFertiliser: adding fertiliser solution.");
    
    // Add an amount of fertiliser that will add 0.1 mS/cm to the EC of the total solution, and calculate how
    // long the pumps have to run to add just that volume.
    // So if target EC = 1.00, it will start adding when the EC is <0.95 for >10 minutes, and top up the EC
    // to about 1.04 (not 1.05 as it was less than 0.95 to begin with).
    float addVolume = 0.1 * 1000 * sensorData->solutionVolume / sensorData->fertiliserConcentration; // The amount of fertiliser in ml to be added.
    runATime = addVolume / settings.pumpASpeed * 60 * 1000; // the time in milliseconds pump A has to run.
    runBTime = addVolume / settings.pumpBSpeed * 60 * 1000; // the time in milliseconds pump B has to run.
    logging->writeTesting("HydroMonitorFertiliser: 10 minutes of too low EC; have to start adding fertiliser.");
    /*
    String message = F("Measured EC: ");
    message += currentEC;
    message += F(", target EC: ");
    message += settings.targetEC;
    message += F(", adding ");
    message += addVolume;
    message += F(" ml of fertiliser.");
    logging->writeTesting(message);
    message = F("Required pump runtime: ");
    message += runATime;
    message += F("ms for pump A, ");
    message += runBTime;
    message += F("ms for pump B.");
    logging->writeTesting(message);
    */
    lastTimeAdded = millis();                               // Time we last added any fertiliser - which is what we're going to do now.
  }
  return;
}

/*
 * Swich pump p on.
 */
#ifdef FERTILISER_A_PIN
void HydroMonitorFertiliser::switchPumpOn(uint8_t p) {
/*
  String message = F("HydroMonitorFertiliser: switching on pump on pin ");
  message += String(p);
  message += F("."));
  logging->writeTrace(message)
*/
  digitalWrite(p, HIGH);
  return;
}  
    
/*
 * Swich pump p off.
 */
void HydroMonitorFertiliser::switchPumpOff(uint8_t p) {
/*
  String message = F("HydroMonitorFertiliser: switching off pump on pin ");
  message += String(p);
  message += F("."));
  logging->writeTrace(message)
*/
  digitalWrite(p, LOW);
  return;
}  
#endif

#ifdef FERTILISER_A_PCF_PIN
void HydroMonitorFertiliser::switchPumpOn(uint8_t p) {
/*
  String message = F("HydroMonitorFertiliser: switching on pump on pcf port expander pin ");
  message += String(p);
  message += F(".");
  logging->writeTrace(message);
*/
  pcf8574->write(p, LOW);
  return;
}  
    
/*
 * Swich pump p off.
 */
void HydroMonitorFertiliser::switchPumpOff(uint8_t p) {
/*
  String message = F("HydroMonitorFertiliser: switching off pump on pcf port expander pin ");
  message += String(p);
  message += F(".");
  logging->writeTrace(message);
*/
  pcf8574->write(p, HIGH);
  return;
}  
#endif

#ifdef FERTILISER_A_MCP_PIN
void HydroMonitorFertiliser::switchPumpOn(uint8_t p) {
/*
  String message = F("HydroMonitorFertiliser: switching on pump on mcp port expander pin ");
  message += String(p);
  message += F("."));
  logging->writeTrace(message)
*/
  mcp23008->digitalWrite(p, HIGH);
  return;
}  
    
/*
 * Swich pump p off.
 */
void HydroMonitorFertiliser::switchPumpOff(uint8_t p) {
/*
  String message = F("HydroMonitorFertiliser: switching on pump on mcp port expander pin ");
  message += String(p);
  message += F("."));
  logging->writeTrace(message)
*/
  mcp23008->digitalWrite(p, LOW);
  return;
}  
#endif

/*
 * Start running pump A for 60 seconds to measure the flow.
 */
void HydroMonitorFertiliser::measurePumpA() {
  if (!(aRunning || bRunning)) {
    logging->writeDebug("HydroMonitorFertiliser: switching on pump A.");
    switchPumpOn(pumpA);
    runATime = millis();
    aRunning = true;
    measuring = true;
  }
}
/*
 * Start running pump B for 60 seconds to measure the flow.
 */
void HydroMonitorFertiliser::measurePumpB() {
  if ( !(aRunning || bRunning)) {
    logging->writeDebug("HydroMonitorFertiliser: switching on pump B.");
    switchPumpOn(pumpB);
    runBTime = millis();
    bRunning = true;
    measuring = true;
  }
}

/*
 * The html code for the sensor specific settings.
 */
String HydroMonitorFertiliser::settingsHtml() {
  String html;
  html = F("\
      <tr>\n\
        <th colspan=\"2\">Fertiliser dosing settings.</th>\n\
      </tr><tr>\n\
        <td>Speed of pump A:</td>\n\
        <td><input type=\"number\" step=\"0.1\" name=\"fertiliser_pumpaspeed\" value=\"");
  html += String(settings.pumpASpeed);
  html += F("\"> ml/minute.&nbsp;&nbsp;<input type=\"submit\" formaction=\"/measure_pump_a_speed\" formmethod=\"post\" name=\"pumpa\" value=\"Measure now\"></td>\n\
      </tr><tr>\n\
        <td>Speed of pump B:</td>\n\
        <td><input type=\"number\" step=\"0.1\" name=\"fertiliser_pumpbspeed\" value=\"");
  html += String(settings.pumpBSpeed);
  html += F("\"> ml/minute.&nbsp;&nbsp;<input type=\"submit\" formaction=\"/measure_pump_b_speed\" formmethod=\"post\" name=\"pumpb\" value=\"Measure now\"></td>\n\
      </tr>\n");
  logging->writeDebug("HydroMonitorFertiliser: created settings html.");
  
  //logging->writeTrace(html);
  
  return html;
}

/*
 * Update the settings for this sensor, if any.
 */
void HydroMonitorFertiliser::updateSettings(String keys[], String values[], uint8_t nArgs) {
  for (uint8_t i=0; i<nArgs; i++) {
    if (keys[i] == F("fertiliser_pumpaspeed")) {
      if (core.isNumeric(values[i])) {
        float val = values[i].toFloat();
        if (val > 0 && val < 2001) settings.pumpASpeed = val;
      }
      else settings.pumpASpeed = 11;
    }
    if (keys[i] == F("fertiliser_pumpbspeed")) {
      if (core.isNumeric(values[i])) {
        float val = values[i].toFloat();
        if (val > 0 && val < 2001) settings.pumpBSpeed = val;
      }
      else settings.pumpBSpeed = 22;
    }
  }
  EEPROM.put(FERTILISER_EEPROM, settings);
  EEPROM.commit();
  logging->writeTrace("HydroMonitorFertiliser: updated settings.");
  return;
}

#endif

