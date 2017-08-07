#include <HydroMonitorGrowlight.h>
/**
 * Manages the growing light switch.
 *
 */
 
#include <Time.h>
#include <Arduino.h>

HydroMonitorGrowlight::HydroMonitorGrowlight (void) {
}

void HydroMonitorGrowlight::begin(Settings s, unsigned char pin) {

  // Set the parameters.
  growlightPin = pin;
  pinMode (growlightPin, OUTPUT);
  digitalWrite (growlightPin, LOW);
  setSettings(s);
  return;
}

void HydroMonitorGrowlight::setSettings(Settings s) {

  settings = s;
  return;
}


void HydroMonitorGrowlight::checkGrowlight(unsigned int brightness) {

  // Don't do anything to the growlight if in manual control mode.
  if (manualMode) {
    startup = false;
    return;
  }

  bool allowOn = false; // Will be set to true if at the current time the light is allowed on.

  // Check whether we're allowed to switch on the light.
  // If time is not set: ignore the on/off hours.
  // Otherwise, check whether we're in between the on and off hours.
  if (timeStatus() == timeNotSet) allowOn = true;
  else {
    unsigned int currentTime = hour() * 60 + minute();
    unsigned int onTime = settings.OnHour * 60 + settings.OnMinute;
    unsigned int offTime = settings.OffHour * 60 + settings.OffMinute;
    if (onTime < offTime) {
      if (currentTime > onTime && currentTime < offTime) allowOn = true;
      else allowOn = false;
    }
    else {
      if (currentTime > onTime && currentTime < offTime) allowOn = false;
      else allowOn = true;
    }
  }
  
  // Make sure that the light is off when it's not allowed on, and we're done.
  if (!allowOn) {
    digitalWrite(growlightPin, LOW);
    return;
  }

  // Check whether the brightness is higher or lower than the threshold for longer
  // than the set delay time, and switch the light off or on accordingly.
  if (brightness < settings.SwitchBrightness) {
  
    // If we're just starting up, pretend that it's been lowlux situation forever.
    if (startup) lowlux = 1;
    highlux = 0; // Low light situation.
    
    // If it's been lowlux long enough, switch it on. Otherwise set the starting
    // time of the lowlux situation.
    if (lowlux > 0 && now() > lowlux + settings.SwitchDelay) digitalWrite(growlightPin, HIGH); // switch on the growlight.
    else lowlux = now();
  }
  else {
    lowlux = 0; // High light situation.
    
    // If it's been highlux long enough, switch it off. Otherwise set the starting
    // time of the highlux situation.
    if (highlux > 0 && now() > highlux + settings.SwitchDelay) digitalWrite(growlightPin, LOW);
    else highlux = now();
  }
  startup = false;
  return;
}

/*
 * Switches the growlight on, regardless of lux level or time of day.
 */
void HydroMonitorGrowlight::on(void) {
  digitalWrite(growlightPin, HIGH);
  manualMode = true;
  return;
}

/*
 * Switches the growlight off, regardless of lux level or time of day.
 */
void HydroMonitorGrowlight::off(void) {
  digitalWrite(growlightPin, LOW);
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
 * Gets the current power on (true) or off (false) status of the growlight.
 */
bool HydroMonitorGrowlight::getStatus() {
  return digitalRead(growlightPin);
}

String HydroMonitorGrowlight::settingsHtml(void) {
  String html;
  html = F("<tr>\
          <th colspan=\"2\">Grow Light Settings.</th>\
        </tr><tr>\
          <td>\
            Brightness to switch on the growlight:\
          </td><td>\
            <input type=\"number\" name=\"growlight_switch_brightness\" min=\"0\" max=\"65535\" value=\"");
  html += settings.SwitchBrightness;
  html += F("\"> lux.\
          </td>\
        </tr><tr>\
          <td>\
            Time delay before the growlight is switched on/off:\
          </td><td>\
            <input type=\"number\" name=\"growlight_switch_delay\" min=\"0\" size=\"6\" value=\"");
  html += settings.SwitchDelay;
  html += F("\"> seconds.\
          </td>\
        </tr><tr>\
          <td>\
            Time after which the growlight may be swiched on:\
          </td><td>\
            <input type=\"number\" name=\"growlight_on_hour\" min=\"0\" max=\"23\" size=\"2\" value=\"");
  html += settings.OnHour;
  html += F("\"> :\
            <input type=\"number\" name=\"growlight_on_minute\" min=\"0\" max=\"59\" size=\"2\" value=\"");
  html += settings.OnMinute;
  html += F("\">\
          </td>\
        </tr><tr>\
          <td>\
            Time after which the growlight may be swiched on:\
          </td><td>\
            <input type=\"number\" name=\"growlight_off_hour\" min=\"0\" max=\"23\" size=\"2\" value=\"");
  html += settings.OffHour;
  html += F("\"> :\
            <input type=\"number\" name=\"growlight_off_minute\" min=\"0\" max=\"59\" size=\"2\" value=\"");
  html += settings.OffMinute;
  html += F("\">\
          </td>\
        </tr><tr>\
          <td>\
            Use automatic daytime sensing?\
          </td><td>");
  if (settings.DaylightAutomatic) {
    html += F("<input type=\"radio\" name=\"growlight_daylight_automatic\" value=\"1\" checked> Yes\
              <input type=\"radio\" name=\"growlight_daylight_automatic\" value=\"0\"> No");
  }
  else  {
    html += F("<input type=\"radio\" name=\"growlight_daylight_automatic\" value=\"1\"> Yes\
             <input type=\"radio\" name=\"growlight_daylight_automatic\" value=\"0\" checked> No");
  }
  html += F("</td>\
        </tr><tr>\
          <td>\
            Current growlight status: ");
  if (manualMode) html += F("manual, ");
  else html += F("automatic, ");
  if (getStatus()) html += F("on.");
  else html += F("off.");
  html += F("\
          </td><td>\
            <input type=\"submit\" formaction=\"/growlight_on\" formmethod=\"post\" name=\"growlight_on\" value=\"Switch On\">\
            &nbsp;&nbsp;<input type=\"submit\" formaction=\"/growlight_off\" formmethod=\"post\" name=\"growlight_off\" value=\"Switch Off\">");
  if (manualMode) html += F("&nbsp;&nbsp;<input type=\"submit\" formaction=\"/growlight_auto\" formmethod=\"post\" name=\"growlight_auto\" value=\"Automatic\">");
  html += F("</td>\
        </tr>");
  return html;
}



