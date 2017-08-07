#include <HydroMonitorHumiditySensor.h>
/**
 * Manages the relative humidity sensor.
 *
 */
 
//#include <Time.h>
//#include <Arduino.h>
//#include <HydroMonitorDHT.h>

HydroMonitorHumiditySensor::HydroMonitorHumiditySensor () {
  humiditySensorPresent = false;
}

void HydroMonitorHumiditySensor::begin(Settings s, String t, uint8_t p) {

  setSettings(s);
  sensor = t;
  sensorPin = p;
  if (sensor == "DHT22") {
    DHT = HydroMonitorDHT();
    DHT.begin(sensorPin);
  }
  humiditySensorPresent = true;
  return;
}

void HydroMonitorHumiditySensor::setSettings(Settings s) {

  settings = s;
  return;
}

double HydroMonitorHumiditySensor::readSensor() {

  if (sensor = "DHT22") return (double)readDHT22();
//  if (sensor = "BME280") return readBME280;
  return -2;
}
	
float HydroMonitorHumiditySensor::readDHT22() {
  return DHT.readHumidity();
}

String HydroMonitorHumiditySensor::settingsHtml(void) {
  String html;
  /*
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
  */
  return html;
}



