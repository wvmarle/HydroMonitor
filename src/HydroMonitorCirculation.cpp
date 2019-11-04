#include <HydroMonitorCirculation.h>

#ifdef USE_CIRCULATION

/*
   The constructor.
*/
HydroMonitorCirculation::HydroMonitorCirculation() {
}

/*
   Set up the circulation pump control.
*/
#ifdef CIRCULATION_MCP17_PIN                                // Connected to MCP23017 port expander.
void HydroMonitorCirculation::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l, Adafruit_MCP23017* mcp23017) {
  mcp = mcp23017;
  mcp->pinMode(CIRCULATION_MCP17_PIN, OUTPUT);
  l->writeTrace(F("HydroMonitorCirculation: configured circulation pump on MCP23017 port expander."));

#elif defined(CIRCULATION_MCP_PIN)                          // Connected to MCP23008 port expander.
void HydroMonitorCirculation::begin(HydroMonitorCore::SensorData * sd, HydroMonitorLogging * l, Adafruit_MCP23008 * mcp23008) {
  mcp = mcp23008;
  mcp->pinMode(CIRCULATION_MCP_PIN, OUTPUT);
  l->writeTrace(F("HydroMonitorCirculation: configured circulation pump on MCP23008 port expander."));

#elif defined(CIRCULATION_PIN)                              // Connected to GPIO port.
void HydroMonitorCirculation::begin(HydroMonitorCore::SensorData * sd, HydroMonitorLogging * l) {
  pinMode(CIRCULATION_PIN, OUTPUT);
  l->writeTrace(F("HydroMonitorCirculation: configured circulation pump."));
#endif                                                      // endif pin definitions.
  sensorData = sd;
  logging = l;
  if (CIRCULATION_EEPROM > 0) {
#ifdef USE_24LC256_EEPROM
    sensorData->EEPROM->get(CIRCULATION_EEPROM, settings);
#else
    EEPROM.get(CIRCULATION_EEPROM, settings);
#endif
  }
  switchPumpOn();
}

/*
   Handle the circulation pump.
*/
void HydroMonitorCirculation::doCirculation() {

  // Switch off the pump while draining, in maintenance mode, or when the reservoir level is low.
  if (bitRead(sensorData->systemStatus, STATUS_DRAINING_RESERVOIR) ||
      bitRead(sensorData->systemStatus, STATUS_MAINTENANCE) ||
      bitRead(sensorData->systemStatus, STATUS_RESERVOIR_LEVEL_LOW)) {
    if (pumpRunning) {
      switchPumpOff();
    }
    lastOff = millis();                                     // Remember when we were last forced in off state.
  }
  else {
    if (pumpRunning == false &&
        bitRead(sensorData->systemStatus, STATUS_FILLING_RESERVOIR) == false) {  // If filling while we were off, wait for it to complete so there is enough water.
      if (lastOff - millis() > 200) {                       // A short delay, allowing to switch from draining to maintenance or filling.
        switchPumpOn();
      }
    }
  }
}

/*
   The sensor settings as html.
*/
void HydroMonitorCirculation::settingsHtml(ESP8266WebServer * server) {
}

/*
   The sensor settings as JSON.
*/
bool HydroMonitorCirculation::settingsJSON(ESP8266WebServer * server) {
  return false;
}

/*
   Update the settings.
*/
void HydroMonitorCirculation::updateSettings(ESP8266WebServer * server) {
}

/*
   Swich pump on.
*/
void HydroMonitorCirculation::switchPumpOn() {
  logging->writeTrace(F("HydroMonitorCirculation: switching on circulation pump."));
#ifdef CIRCULATION_PIN
  digitalWrite(CIRCULATION_PIN, HIGH);
#elif defined(CIRCULATION_MCP_PIN)
  mcp->digitalWrite(CIRCULATION_MCP_PIN, HIGH);
#elif defined(CIRCULATION_MCP17_PIN)
  mcp->digitalWrite(CIRCULATION_MCP17_PIN, HIGH);
#endif
  pumpRunning = true;
}

/*
   Swich pump off.
*/
void HydroMonitorCirculation::switchPumpOff() {
  logging->writeTrace(F("HydroMonitorCirculation: switching off circulation pump."));
#ifdef CIRCULATION_PIN
  digitalWrite(p, LOW);
#elif defined(CIRCULATION_MCP_PIN)
  mcp->digitalWrite(CIRCULATION_MCP_PIN, LOW);
#elif defined(CIRCULATION_MCP17_PIN)
  mcp->digitalWrite(CIRCULATION_MCP17_PIN, LOW);
#endif
  pumpRunning = false;
}

#endif

