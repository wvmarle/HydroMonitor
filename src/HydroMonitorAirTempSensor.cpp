#include <HydroMonitorAirTempSensor.h>

/*
 * Measure the air temperature.
 */

HydroMonitorAirTempSensor::HydroMonitorAirTempSensor () {
  airTempSensorPresent = false;
}

void HydroMonitorAirTempSensor::begin(Settings s, String t, uint8_t p) {

  setSettings(s);
  sensor = t;
  sensorPin = p;
  if (sensor == "DHT22") {
    DHT = HydroMonitorDHT();
    DHT.begin(sensorPin);
  }
  return;
}

void HydroMonitorAirTempSensor::setSettings(Settings s) {

  settings = s;
  return;
}

double HydroMonitorAirTempSensor::readSensor() {

  if (sensor = "DHT22") return (double)readDHT22();
//  if (sensor = "BMP180") return readBMP180;
//  if (sensor = "BMP280") return readBMP280;
//  if (sensor = "BME280") return readBME280;
  return -2;
}

float HydroMonitorAirTempSensor::readDHT22() {
  return DHT.readTemperature();
}

String HydroMonitorAirTempSensor::settingsHtml() {
  String html;
  /*
  html = F("<tr>\
        <th colspan=\"2\">EC Sensor settings.</th>\
      </tr><tr>\
        <td>Number of samples:</td>\
        <td><input type=\"number\" name=\"pressure_altitude\" value=\"");
  html += String(settings.Altitude);
  html += F("\"> meters above sealevel.</td>\
      </tr>");
  */
  return html;
}

