#include <HydroMonitorGrowlight.h>

#ifdef USE_GROWLIGHT
/**
   Manages the growing light switch.
*/
HydroMonitorGrowlight::HydroMonitorGrowlight (void) {
}

/*
   Set up the module - growing light connected to the PCF8574 port expander.
*/
#ifdef GROWLIGHT_PCF_PIN
void HydroMonitorGrowlight::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l, PCF857x *pcf) {
  pcf8574 = pcf;
  l->writeTrace(F("HydroMonitorGrowlight: set up growing light on PCF port expander."));

  /*
     Set up the module - growing light connected to the MCP23008 port expander.
  */
#elif defined(GROWLIGHT_MCP_PIN)
void HydroMonitorGrowlight::begin(HydroMonitorCore::SensorData * sd, HydroMonitorLogging * l, Adafruit_MCP23008 * mcp) {
  mcp23008 = mcp;
  mcp23008->pinMode(GROWLIGHT_MCP_PIN, OUTPUT);
  l->writeTrace(F("HydroMonitorGrowlight: set up growing light on MCP port expander."));

  /*
     Set up the module - growing light connected to the MCP23017 port expander.
  */
#elif defined(GROWLIGHT_MCP17_PIN)
void HydroMonitorGrowlight::begin(HydroMonitorCore::SensorData * sd, HydroMonitorLogging * l, Adafruit_MCP23017 * mcp) {
  mcp23017 = mcp;
  mcp23017->pinMode(GROWLIGHT_MCP17_PIN, OUTPUT);
  l->writeTrace(F("HydroMonitorGrowlight: set up growing light on MCP17 port expander."));

  /*
     Set up the module - growing light connected to a GPIO pin.
  */
#elif defined(GROWLIGHT_PIN)
void HydroMonitorGrowlight::begin(HydroMonitorCore::SensorData * sd, HydroMonitorLogging * l) {
  pinMode(GROWLIGHT_PIN, OUTPUT);
  l->writeTrace(F("HydroMonitorGrowlight: set up growing light."));
#endif

  sensorData = sd;
  logging = l;
  switchLightOff();
  if (GROWLIGHT_EEPROM > 0)
    EEPROM.get(GROWLIGHT_EEPROM, settings);

  // Check whether we have a sensible value for switchBrightness; if not
  // set defaults for all settings.
  if (settings.switchBrightness == 0 || settings.switchBrightness == 65535) {
    logging->writeTrace(F("HydroMonitorGrowlight: applying default settings."));
    settings.switchBrightness = 1000;                       // Brightness below which the light may be switched on.
    settings.switchDelay = 5 * 60;                          // The delay in seconds.
    settings.onHour = 6;                                    // Hour and
    settings.onMinute = 00;                                 // minute of the day after which the growlight may be switched on.
    settings.offHour = 18;                                  // Hour and
    settings.offMinute = 00;                                // minute of the day after which the growlight must be switched off.
    settings.daylightAutomatic = false;                     // Whether to attempt to follow daylight automatically.
    EEPROM.put(GROWLIGHT_EEPROM, settings);
    EEPROM.commit();
  }

  lowlux = -settings.switchDelay; // Pretend it's been dark all along.
  highlux = 0;
}

/*
   Handles the status of the growlight.
   Check whether we may be on; if so check whether brightness is above or below the
   theshold and if long enough switch the growlight off or on as needed.
*/
void HydroMonitorGrowlight::checkGrowlight() {

  if (manualMode ||   // Don't do anything to the growlight if in manual control mode.
      sensorData->brightness < 1) {  // Don't bother doing anything if the brightness sensor isn't working.
    return;
  }

  bool allowOn = false; // Will be set to true if at the current time the light is allowed on.

  // Check whether we're allowed to switch on the light.
  // If time is not set: ignore the on/off hours.
  // Otherwise, check whether we're in between the on and off hours.
  if (timeStatus() == timeNotSet) {
    allowOn = true;
  }
  else {
    uint32_t currentTime = hour() * 60 + minute();
    uint32_t onTime = settings.onHour * 60 + settings.onMinute;
    uint32_t offTime = settings.offHour * 60 + settings.offMinute;
    if (onTime < offTime) {
      allowOn = (currentTime > onTime && currentTime < offTime);
    }
    else {
      allowOn = ~(currentTime > onTime && currentTime < offTime);
    }
  }

#ifdef USE_BRIGHTNESS_SENSOR
  // Make sure that the light is off when it's not allowed on, and we're done.
  if (!allowOn) {
    switchLightOff();
  }

  // Check whether the brightness is higher or lower than the threshold for longer
  // than the set delay time, and switch the light off or on accordingly.
  else if (sensorData->brightness < settings.switchBrightness) {
    highlux = 0; // Low light situation.

    // If it's been lowlux long enough, switch it on. Otherwise set the starting
    // time of the lowlux situation.
    if (lowlux > 0) {
      if (now() - lowlux > settings.switchDelay) {
        switchLightOn();                                    // switch on the growlight.
      }
    }
    else {
      lowlux = now();
    }
  }
  else {
    lowlux = 0; // High light situation.

    // If it's been highlux long enough, switch it off. Otherwise set the starting
    // time of the highlux situation.
    if (highlux > 0) {
      if (now() - highlux > settings.switchDelay) {
        switchLightOff();
      }
    }
    else {
      highlux = now();
    }
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
}

/*
   Switches the growlight on, regardless of lux level or time of day.
*/
void HydroMonitorGrowlight::on(void) {
  switchLightOn();
  manualMode = true;
}

/*
   Switches the growlight off, regardless of lux level or time of day.
*/
void HydroMonitorGrowlight::off(void) {
  switchLightOff();
  manualMode = true;
}

/*
   Switches the growlight off, regardless of lux level or time of day.
*/
void HydroMonitorGrowlight::automatic(void) {
  manualMode = false;
}

/*
   Switch the light on or off.
*/
#ifdef GROWLIGHT_PIN
void HydroMonitorGrowlight::switchLightOn() {
  sensorData->growlight = true;
  digitalWrite(GROWLIGHT_PIN, HIGH);
}

void HydroMonitorGrowlight::switchLightOff() {
  sensorData->growlight = false;
  digitalWrite(GROWLIGHT_PIN, LOW);
}
#elif defined(GROWLIGHT_PCF_PIN)
void HydroMonitorGrowlight::switchLightOn() {
  sensorData->growlight = true;
  pcf8574->write(GROWLIGHT_PCF_PIN, LOW);
}

void HydroMonitorGrowlight::switchLightOff() {
  sensorData->growlight = false;
  pcf8574->write(GROWLIGHT_PCF_PIN, HIGH);
}
#elif defined(GROWLIGHT_MCP_PIN)
void HydroMonitorGrowlight::switchLightOn() {
  sensorData->growlight = true;
  mcp23008->digitalWrite(GROWLIGHT_MCP_PIN, HIGH);
}

void HydroMonitorGrowlight::switchLightOff() {
  sensorData->growlight = false;
  mcp23008->digitalWrite(GROWLIGHT_MCP_PIN, LOW);
}
#elif defined(GROWLIGHT_MCP17_PIN)
void HydroMonitorGrowlight::switchLightOn() {
  sensorData->growlight = true;
  mcp23017->digitalWrite(GROWLIGHT_MCP17_PIN, HIGH);
}

void HydroMonitorGrowlight::switchLightOff() {
  sensorData->growlight = false;
  mcp23017->digitalWrite(GROWLIGHT_MCP17_PIN, LOW);
}
#endif

/*
   The sensor settings as html.
*/
void HydroMonitorGrowlight::settingsHtml(ESP8266WebServer * server) {
  char buff[10];
  server->sendContent_P(PSTR("\
        <tr>\n\
          <th colspan=\"2\">Grow Light Settings.</th>\n\
        </tr><tr>\n\
          <td>\n\
            Brightness to switch on the growlight:\n\
          </td><td>\n\
            <input type=\"number\"  step=\"1\"name=\"growlight_switch_brightness\" min=\"0\" max=\"65535\" value=\""));
  server->sendContent(itoa(settings.switchBrightness, buff, 10));
  server->sendContent_P(PSTR("\> lux.\n\
          </td>\n\
        </tr><tr>\n\
          <td>\n\
            Time delay before the growlight is switched on/off:\n\
          </td><td>\n\
            <input type=\"number\" step=\"1\" name=\"growlight_switch_delay\" min=\"0\" size=\"6\" value=\""));
  server->sendContent(itoa(settings.switchDelay, buff, 10));
  server->sendContent_P(PSTR("\"> seconds.\n\
          </td>\n\
        </tr><tr>\n\
          <td>\n\
            Time after which the growlight may be swiched on:\n\
          </td><td>\n\
            <input type=\"number\" step=\"1\" name=\"growlight_on_hour\" min=\"0\" max=\"23\" size=\"2\" value=\""));
  server->sendContent(itoa(settings.onHour, buff, 10));
  server->sendContent_P(PSTR("\"> :\n\
            <input type=\"number\" step=\"1\" name=\"growlight_on_minute\" min=\"0\" max=\"59\" size=\"2\" value=\""));
  server->sendContent(itoa(settings.onMinute, buff, 10));
  server->sendContent_P(PSTR("\">\n\
          </td>\n\
        </tr><tr>\n\
          <td>\n\
            Time after which the growlight must be swiched off:\n\
          </td><td>\n\
            <input type=\"number\" step=\"1\" name=\"growlight_off_hour\" min=\"0\" max=\"23\" size=\"2\" value=\""));
  server->sendContent(itoa(settings.offHour, buff, 10));
  server->sendContent_P(PSTR("\"> :\n\
            <input type=\"number\" step=\"1\" name=\"growlight_off_minute\" min=\"0\" max=\"59\" size=\"2\" value=\""));
  server->sendContent(itoa(settings.offMinute, buff, 10));
  server->sendContent_P(PSTR("\">\
          </td>\n\
        </tr><tr>\n\
          <td>\n\
            Use automatic daytime sensing?\n\
          </td><td>"));
  if (settings.daylightAutomatic) {
    server->sendContent_P(PSTR("<input type=\"radio\" name=\"growlight_daylight_automatic\" value=\"1\" checked> Yes\n\
              <input type=\"radio\" name=\"growlight_daylight_automatic\" value=\"0\"> No"));
  }
  else  {
    server->sendContent_P(PSTR("<input type=\"radio\" name=\"growlight_daylight_automatic\" value=\"1\"> Yes\n\
             <input type=\"radio\" name=\"growlight_daylight_automatic\" value=\"0\" checked> No"));
  }
  server->sendContent_P(PSTR("</td>\n\
        </tr><tr>\n\
          <td>\n\
            Current growlight status: "));
  if (manualMode) {
    server->sendContent_P(PSTR("manual, "));
  }
  else {
    server->sendContent_P(PSTR("automatic, "));
  }
  if (sensorData->growlight) {
    server->sendContent_P(PSTR("on."));
  }
  else {
    server->sendContent_P(PSTR("off."));
  }
  server->sendContent_P(PSTR("\n\
          </td><td>\n\
            <input type=\"submit\" formaction=\"/growlight_on\" formmethod=\"post\" name=\"growlight_on\" value=\"Switch On\">\n\
            &nbsp;&nbsp;<input type=\"submit\" formaction=\"/growlight_off\" formmethod=\"post\" name=\"growlight_off\" value=\"Switch Off\">"));
  if (manualMode) {
    server->sendContent_P(PSTR("&nbsp;&nbsp;<input type=\"submit\" formaction=\"/growlight_auto\" formmethod=\"post\" name=\"growlight_auto\" value=\"Automatic\">"));
  }
  server->sendContent_P(PSTR("</td>\n\
        </tr>\n"));
}

/*
   The sensor settings as JSON.
*/
bool HydroMonitorGrowlight::settingsJSON(ESP8266WebServer * server) {
  char buff[10];
  server->sendContent_P(PSTR("  \"growlight\": {\n"
                             "    \"growlight_switch_brightness\":\""));
  server->sendContent(itoa(settings.switchBrightness, buff, 10));
  server->sendContent_P(PSTR("\",\n"
                             "    \"growlight_switch_delay\":\""));
  server->sendContent(itoa(settings.switchDelay, buff, 10));
  server->sendContent_P(PSTR("\",\n"
                             "    \"growlight_on_hour\":\""));
  server->sendContent(itoa(settings.onHour, buff, 10));
  server->sendContent_P(PSTR("\",\n"
                             "    \"growlight_on_minute\":\""));
  server->sendContent(itoa(settings.onMinute, buff, 10));
  server->sendContent_P(PSTR("\",\n"
                             "    \"growlight_off_hour\":\""));
  server->sendContent(itoa(settings.offHour, buff, 10));
  server->sendContent_P(PSTR("\",\n"
                             "    \"growlight_off_minute\":\""));
  server->sendContent(itoa(settings.offMinute, buff, 10));
  server->sendContent_P(PSTR("\"\n"
                             "  }"));
}

/*
   Update the settings for this sensor, if any.
*/
void HydroMonitorGrowlight::updateSettings(ESP8266WebServer *server) {
  for (uint8_t i = 0; i < server->args(); i++) {
    if (server->argName(i) == "growlight_switch_brightness") {
      if (core.isNumeric(server->arg(i))) {
        uint8_t val = server->arg(i).toInt();
        if (val < 65536) {
          settings.switchBrightness = val;
        }
      }
    }
    else if (server->argName(i) == "growlight_switch_delay") {
      if (core.isNumeric(server->arg(i))) {
        uint8_t val = server->arg(i).toInt();
        settings.switchDelay = val;
      }
    }    
    else if (server->argName(i) == "growlight_on_hour") {
      if (core.isNumeric(server->arg(i))) {
        uint8_t val = server->arg(i).toInt();
        if (val < 24) {
          settings.onHour = val;
        }
      }
    }
    else if (server->argName(i) == "growlight_on_minute") {
      if (core.isNumeric(server->arg(i))) {
        uint8_t val = server->arg(i).toInt();
        if (val < 60) {
          settings.onMinute = val;
        }
      }
    }
    else if (server->argName(i) == "growlight_off_hour") {
      if (core.isNumeric(server->arg(i))) {
        uint8_t val = server->arg(i).toInt();
        if (val < 24) {
          settings.offHour = val;
        }
      }
    }
    else if (server->argName(i) == "growlight_off_minute") {
      if (core.isNumeric(server->arg(i))) {
        uint8_t val = server->arg(i).toInt();
        if (val < 60) {
          settings.offMinute = val;
        }
      }
    }
    else if (server->argName(i) == "growlight_daylight_automatic") {
      if (core.isNumeric(server->arg(i))) {
        uint8_t val = server->arg(i).toInt();
        if (val == 0) {
          settings.daylightAutomatic = false;
        }
        else {
          settings.daylightAutomatic = true;
        }
      }
    }
  }
#ifdef USE_24LC256_EEPROM
  sensorData->EEPROM->put(RESERVOIR_EEPROM, settings);
#else
  EEPROM.put(RESERVOIR_EEPROM, settings);
  EEPROM.commit();
#endif
}
#endif

