#include <HydroMonitorGrowlight.h>

#ifdef USE_GROWLIGHT
/**
 * Manages the growing light switch.
 */
HydroMonitorGrowlight::HydroMonitorGrowlight (void) {
}

/*
 * Set up the module - growing light connected to the PCF8574 port expander.
 */
#ifdef GROWLIGHT_PCF_PIN        
void HydroMonitorGrowlight::begin(HydroMonitorCore::SensorData *sd, HydroMonitorMySQL *l, PCF857x *pcf) {
  pcf8574 = pcf;
  l->writeTesting("HydroMonitorGrowlight: set up growing light on PCF port expander.");

/*
 * Set up the module - growing light connected to the MCP23008 port expander.
 */
#elif defined(GROWLIGHT_MCP_PIN)
void HydroMonitorGrowlight::begin(HydroMonitorCore::SensorData *sd, HydroMonitorMySQL *l, Adafruit_MCP23008 *mcp) {
  mcp23008 = mcp;
  mcp23008->pinMode(GROWLIGHT_MCP_PIN, OUTPUT);
  l->writeTesting("HydroMonitorGrowlight: set up growing light on MCP port expander.");

/*
 * Set up the module - growing light connected to a GPIO pin.
 */
#elif defined(GROWLIGHT_PIN)
void HydroMonitorGrowlight::begin(HydroMonitorCore::SensorData *sd, HydroMonitorMySQL *l) {
  pinMode(GROWLIGHT_PIN, OUTPUT);
  l->writeTesting("HydroMonitorGrowlight: set up growing light.");
#endif

  sensorData = sd;
  logging = l;
  switchLightOff();
  if (GROWLIGHT_EEPROM > 0)
    EEPROM.get(GROWLIGHT_EEPROM, settings);

  // Check whether we have a sensible value for switchBrightness; if not
  // set defaults for all settings.
  if (settings.switchBrightness == 0 || settings.switchBrightness == 65535) {
    logging->writeDebug("HydroMonitorGrowlight: applying default settings.");
    settings.switchBrightness = 1000;    // Brightness below which the light may be switched on.
    settings.switchDelay = 5 * 60;       // The delay in seconds.
    settings.onHour = 6;                 // Hour and 
    settings.onMinute = 00;              // minute of the day after which the growlight may be switched on.
    settings.offHour = 18;               // Hour and 
    settings.offMinute = 00;             // minute of the day after which the growlight must be switched off.
    settings.daylightAutomatic = false;  // Whether to attempt to follow daylight automatically.
  }

  lowlux = -settings.switchDelay; // Pretend it's been dark all along.
  highlux = 0;
  return;
}

/*
 * Handles the status of the growlight.
 * Check whether we may be on; if so check whether brightness is above or below the
 * theshold and if long enough switch the growlight off or on as needed.
 */
void HydroMonitorGrowlight::checkGrowlight() {

  // Don't do anything to the growlight if in manual control mode.
  if (manualMode) return;
  
  // Don't bother doing anything if the brightness sensor isn't working.
  if (sensorData->brightness < 1) return;

  bool allowOn = false; // Will be set to true if at the current time the light is allowed on.

  // Check whether we're allowed to switch on the light.
  // If time is not set: ignore the on/off hours.
  // Otherwise, check whether we're in between the on and off hours.
  if (timeStatus() == timeNotSet) allowOn = true;
  else {
    uint32_t currentTime = hour() * 60 + minute();
    uint32_t onTime = settings.onHour * 60 + settings.onMinute;
    uint32_t offTime = settings.offHour * 60 + settings.offMinute;
    if (onTime < offTime) {
      if (currentTime > onTime && currentTime < offTime) allowOn = true;
      else allowOn = false;
    }
    else {
      if (currentTime > onTime && currentTime < offTime) allowOn = false;
      else allowOn = true;
    }
  }

#ifdef USE_BRIGHTNESS_SENSOR
  // Make sure that the light is off when it's not allowed on, and we're done.
  if (!allowOn) {
    switchLightOff();
    return;
  }

  // Check whether the brightness is higher or lower than the threshold for longer
  // than the set delay time, and switch the light off or on accordingly.
  if (sensorData->brightness < settings.switchBrightness) {
    highlux = 0; // Low light situation.
    
    // If it's been lowlux long enough, switch it on. Otherwise set the starting
    // time of the lowlux situation.
    if (lowlux > 0) {
      if (now() - lowlux > settings.switchDelay) switchLightOn(); // switch on the growlight.
    }
    else lowlux = now();
  }
  else {
    lowlux = 0; // High light situation.
    
    // If it's been highlux long enough, switch it off. Otherwise set the starting
    // time of the highlux situation.
    if (highlux > 0) {
      if (now() - highlux > settings.switchDelay) switchLightOff();
    }
    else highlux = now();
  }
#else
  // If no brightness sensor connected, use the time to switch the light on and off.
  if (allowOn) {
    switchLightOn();
  }
  else {
    switchLightOff();
  }
#endif
  return;
}

/*
 * Switches the growlight on, regardless of lux level or time of day.
 */
void HydroMonitorGrowlight::on(void) {
  switchLightOn();
  manualMode = true;
  return;
}

/*
 * Switches the growlight off, regardless of lux level or time of day.
 */
void HydroMonitorGrowlight::off(void) {
  switchLightOff();
  manualMode = true;
  return;
}

/*
 * Switches the growlight off, regardless of lux level or time of day.
 */
void HydroMonitorGrowlight::automatic(void) {
  manualMode = false;
  return;
}

/*
 * Switch the light on or off.
 */
#ifdef GROWLIGHT_PIN
void HydroMonitorGrowlight::switchLightOn() {
  sensorData->growlight = true;
  digitalWrite(GROWLIGHT_PIN, HIGH);
  return;
}

void HydroMonitorGrowlight::switchLightOff() {
  sensorData->growlight = false;
  digitalWrite(GROWLIGHT_PIN, LOW);
  return;
}
#endif
#ifdef GROWLIGHT_PCF_PIN
void HydroMonitorGrowlight::switchLightOn() {
  sensorData->growlight = true;
  pcf8574->write(GROWLIGHT_PCF_PIN, LOW);
  return;
}

void HydroMonitorGrowlight::switchLightOff() {
  sensorData->growlight = false;
  pcf8574->write(GROWLIGHT_PCF_PIN, HIGH);
  return;
}
#endif
#ifdef GROWLIGHT_MCP_PIN
void HydroMonitorGrowlight::switchLightOn() {
  sensorData->growlight = true;
  mcp23008->digitalWrite(GROWLIGHT_MCP_PIN, HIGH);
  return;
}

void HydroMonitorGrowlight::switchLightOff() {
  sensorData->growlight = false;
  mcp23008->digitalWrite(GROWLIGHT_MCP_PIN, LOW);
  return;
}
#endif

/*
 * The sensor settings as html.
 */
String HydroMonitorGrowlight::settingsHtml(void) {
  String html;
  html = F("\
        <tr>\n\
          <th colspan=\"2\">Grow Light Settings.</th>\n\
        </tr><tr>\n\
          <td>\n\
            Brightness to switch on the growlight:\n\
          </td><td>\n\
            <input type=\"number\"  step=\"1\"name=\"growlight_switch_brightness\" min=\"0\" max=\"65535\" value=\"");
  html += settings.switchBrightness;
  html += F("\"> lux.\n\
          </td>\n\
        </tr><tr>\n\
          <td>\n\
            Time delay before the growlight is switched on/off:\n\
          </td><td>\n\
            <input type=\"number\" step=\"1\" name=\"growlight_switch_delay\" min=\"0\" size=\"6\" value=\"");
  html += settings.switchDelay;
  html += F("\"> seconds.\n\
          </td>\n\
        </tr><tr>\n\
          <td>\n\
            Time after which the growlight may be swiched on:\n\
          </td><td>\n\
            <input type=\"number\" step=\"1\" name=\"growlight_on_hour\" min=\"0\" max=\"23\" size=\"2\" value=\"");
  html += settings.onHour;
  html += F("\"> :\n\
            <input type=\"number\" step=\"1\" name=\"growlight_on_minute\" min=\"0\" max=\"59\" size=\"2\" value=\"");
  html += settings.onMinute;
  html += F("\">\n\
          </td>\n\
        </tr><tr>\n\
          <td>\n\
            Time after which the growlight must be swiched off:\n\
          </td><td>\n\
            <input type=\"number\" step=\"1\" name=\"growlight_off_hour\" min=\"0\" max=\"23\" size=\"2\" value=\"");
  html += settings.offHour;
  html += F("\"> :\n\
            <input type=\"number\" step=\"1\" name=\"growlight_off_minute\" min=\"0\" max=\"59\" size=\"2\" value=\"");
  html += settings.offMinute;
  html += F("\">\
          </td>\n\
        </tr><tr>\n\
          <td>\n\
            Use automatic daytime sensing?\n\
          </td><td>");
  if (settings.daylightAutomatic) {
    html += F("<input type=\"radio\" name=\"growlight_daylight_automatic\" value=\"1\" checked> Yes\n\
              <input type=\"radio\" name=\"growlight_daylight_automatic\" value=\"0\"> No");
  }
  else  {
    html += F("<input type=\"radio\" name=\"growlight_daylight_automatic\" value=\"1\"> Yes\n\
             <input type=\"radio\" name=\"growlight_daylight_automatic\" value=\"0\" checked> No");
  }
  html += F("</td>\n\
        </tr><tr>\n\
          <td>\n\
            Current growlight status: ");
  if (manualMode) html += F("manual, ");
  else html += F("automatic, ");
  if (sensorData->growlight) html += F("on.");
  else html += F("off.");
  html += F("\n\
          </td><td>\n\
            <input type=\"submit\" formaction=\"/growlight_on\" formmethod=\"post\" name=\"growlight_on\" value=\"Switch On\">\n\
            &nbsp;&nbsp;<input type=\"submit\" formaction=\"/growlight_off\" formmethod=\"post\" name=\"growlight_off\" value=\"Switch Off\">");
  if (manualMode) html += F("&nbsp;&nbsp;<input type=\"submit\" formaction=\"/growlight_auto\" formmethod=\"post\" name=\"growlight_auto\" value=\"Automatic\">");
  html += F("</td>\n\
        </tr>\n");
  return html;
}

/*
 * The sensor settings as html.
 */
String HydroMonitorGrowlight::dataHtml(void) {
  String html = F("<tr>\n\
    <td>Growing light</td>\n\
    <td>");
  if (sensorData->growlight) html += F("On.</td>\n\
  </tr>");
  else html += F("Off</td>\n\
  </tr>");
  return html;
}


/*
 * Update the settings for this sensor, if any.
 */
void HydroMonitorGrowlight::updateSettings(String keys[], String values[], uint8_t nArgs) {
  for (uint8_t i=0; i<nArgs; i++) {
    if (keys[i] == "growlight_switch_brightness") {
      if (core.isNumeric(values[i])) {
        uint32_t val = values[i].toInt();
        if (val < 65536) settings.switchBrightness = val;
      }
      continue;
    }
    if (keys[i] == "growlight_switch_delay") {
      if (core.isNumeric(values[i])) {
        uint16_t val = values[i].toInt();
        if (val > 0) settings.switchDelay = val;
      }
      continue;
    }
    if (keys[i] == "growlight_on_hour") {
      if (core.isNumeric(values[i])) {
        uint8_t val = values[i].toInt();
        if (val <= 23) settings.onHour = val;
      }
      continue;
    }
    if (keys[i] == "growlight_on_minute") {
      if (core.isNumeric(values[i])) {
        uint8_t val = values[i].toInt();
        if (val <= 59) settings.onMinute = val;
      }
      continue;
    }
    if (keys[i] == "growlight_off_hour") {
      if (core.isNumeric(values[i])) {
        uint8_t val = values[i].toInt();
        if (val <= 23) settings.offHour = val;
      }
      continue;
    }
    if (keys[i] == "growlight_off_minute") {
      if (core.isNumeric(values[i])) {
        uint8_t val = values[i].toInt();
        if (val >= 0 && val <= 59) settings.offMinute = val;
      }
      continue;
    }
    if (keys[i] == "growlight_daylight_automatic") {
      if (core.isNumeric(values[i])) {
        uint8_t val = values[i].toInt();
        if (val == 0) settings.daylightAutomatic = false;
        else settings.daylightAutomatic = true;
      }
      continue;
    }
  }
  EEPROM.put(GROWLIGHT_EEPROM, settings);
  EEPROM.commit();
  return;
}
#endif

