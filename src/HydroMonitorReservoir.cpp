#include <HydroMonitorReservoir.h>

#ifdef USE_RESERVOIR
HydroMonitorReservoir::HydroMonitorReservoir() {
}

#ifndef USE_WATERLEVEL_SENSOR
#error Can't handle the reservoir filler without water level sensor.
#endif

/*
 * Set up the solenoid, connected to a MCP23008 port expander.
 */
#ifdef RESERVOIR_MCP_PIN
void HydroMonitorReservoir::begin(HydroMonitorCore::SensorData *sd, HydroMonitorMySQL *l, Adafruit_MCP23008* mcp, HydroMonitorWaterLevelSensor* sens) {
  mcp23008 = mcp;
  mcp23008->pinMode(RESERVOIR_MCP_PIN, OUTPUT);
  l->writeTesting("HydroMonitorReservoir: configured reservoir refill on MCP port expander.");

/*
 * Set up the solenoid, connected to a PCF8574 port expander.
 */
#elif defined(RESERVOIR_PCF_PIN)
void HydroMonitorReservoir::begin(HydroMonitorCore::SensorData *sd, HydroMonitorMySQL *l, PCF857x* pcf, HydroMonitorWaterLevelSensor* sens) {
  pcf8574 = pcf;
  l->writeTesting("HydroMonitorReservoir: configured reservoir refill on PCF port expander.");

/*
 * Set up the solenoid, connected to a GPIO port.
 */
#elif defined(RESERVOIR_PIN)
void HydroMonitorReservoir::begin(HydroMonitorCore::SensorData *sd, HydroMonitorMySQL *l, HydroMonitorWaterLevelSensor* sens) {
  pinMode(RESERVOIR_PIN, OUTPUT);
  l->writeTesting("HydroMonitorReservoir: configured reservoir refill.");
#endif
  sensorData = sd;
  logging = l;
  waterLevelSensor = sens;
  if (RESERVOIR_EEPROM > 0)
    EEPROM.get(RESEROVOIR_EEPROM, settings);

  if (settings.maxFill > 100) {
    logging->writeTesting("HydroMonitorReservoir: applying default settings.");
    settings.maxFill = 90;
    settings.minFill = 70;
  }
  return;
}

/*
 * doReservoir - manages the switching of the solenoid and adding the water to the reservoir.
 *
 * This has to be called as frequent as possible for proper timing and preventing overfilling.
 */
void HydroMonitorReservoir::doReservoir() {

  // As long as the water level is above the set minimum and we're not in adding water now, 
  // there's nothing to do here other than keeping track of when this was.
  if (fill > settings.minFill && !addWater) {
    lastGoodFill = millis();
    return;
  }
  
  // Wait for the fill level to be too low for 10 minutes before taking action.
  if (millis() - lastGoodFill > 10 * 60 * 1000) {
    core.writeTesting(F("HydroMonitorReservoir: water level too low for 10 minutes, opening the valve."));
    core.writeInfo(F("HydroMonitorReservoir: adding water to the reservoir."));
    openValve();
    addWater = true;
    lastLevel = millis();
    startAddWater = millis();
  }
  
  // Measure the reservoir fill every 0.5 seconds.
  if (millis() - lastLevel > 500) {
    lastLevel += 500;
    waterLevelSensor->readSensor();
  }

  // Check whether we have enough water, if so close the valve.
  if (sensorData.fill > settings.maxFill) {
    closeValve();
    core.writeTesting(F("HydroMonitorReservoir: water level high enough, closing the valve."));
    addWater = false;
  }
  
  // As extra safety measure: close the valve after 2 minutes, regardless of what the fill sensor says.
  if (millis() - startAddWater > 2 * 60 * 1000) {
    closeValve();
    core.writeTesting(F("HydroMonitorReservoir: added water for 2 minutes, high level not reached, timeout: closing the valve."));
    addWater = false;
  }
}

/*
 * Functions to open (output power on) and close (output power off) the valve .
 */
#ifdef RESERVOIR_PIN
void HydroMonitorReservoir::openValve() {
  digitalWrite(RESERVOIR_PIN, HIGH);
  return;
}  

void HydroMonitorReservoir::closeValve() {
  digitalWrite(RESERVOIR_PIN, LOW);
  return;
}

#elif defined(RESERVOIR_PCF_PIN)
void HydroMonitorReservoir::openValve() {
  pcf8574->write(RESERVOIR_PCF_PIN, LOW);
  return;
}  
    
void HydroMonitorReservoir::closeValve() {
  pcf8574->write(RESERVOIR_PCF_PIN, HIGH);
  return;
}

#elif defined(RESERVOIR_MCP_PIN)
void HydroMonitorReservoir::openValve() {
  mcp23008->digitalWrite(RESERVOIR_MCP_PIN, HIGH);
  return;
}  

void HydroMonitorReservoir::closeValve() {
  mcp23008->digitalWrite(RESERVOIR_MCP_PIN, LOW);
  return;
}
#endif

/*
 * The settings as html.
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
 * Process the settings from the key/value pairs.
 */
void HydroMonitorReservoir::updateSettings(String keys[], String values[], uint8_t nArgs) {
  for (uint8_t i=0; i<nArgs; i++) {
    if (keys[i] == "reservoir_minfill") {
      if (core.isNumeric(values[i])) {
        uint8_t val = values[i].toInt();
        if (val <= 99) settings.minFill = val; // maxLevel must be larger, so 100% is not allowed.
      }
    }
  }
  for (uint8_t i=0; i<nArgs; i++) {
    if (keys[i] == "reservoir_minfill") {
      if (core.isNumeric(values[i])) {
        uint8_t val = values[i].toInt();
        if (val > settings.minFill && val <= 100) settings.minFill = val;
      }
    }
  }
  EEPROM.put(RESERVOIR_EEPROM, settings);
  EEPROM.commit();
  return;
}
#endif

