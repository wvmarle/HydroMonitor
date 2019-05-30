#include <HydroMonitorPressureSensor.h>

#ifdef USE_PRESSURE_SENSOR
/*
   Measure the atmospheric pressure.
   The BMP180 sensor connects over I2C.
*/

HydroMonitorPressureSensor::HydroMonitorPressureSensor () {
}

#ifdef USE_BMP180
void HydroMonitorPressureSensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l, BMP180 *bmp) {
  bmp180 = bmp;
  l->writeTrace(F("HydroMonitorPressureSensor: configured a BMP180 sensor."));

#elif defined(USE_BMP280) || defined(USE_BME280)
void HydroMonitorPressureSensor::begin(HydroMonitorCore::SensorData * sd, HydroMonitorLogging * l, BME280 * bmp) {
  bmp280 = bmp;
  l->writeTrace(F("HydroMonitorPressureSensor: configured a BME280 sensor."));
#endif

  sensorData = sd;
  logging = l;
  if (PRESSURE_SENSOR_EEPROM > 0)
    EEPROM.get(PRESSURE_SENSOR_EEPROM, settings);

  if (settings.altitude < -200 || settings.altitude > 100000) {
    settings.altitude = 0;
    EEPROM.put(PRESSURE_SENSOR_EEPROM, settings);
    EEPROM.commit();
  }
}

void HydroMonitorPressureSensor::readSensor() {
#ifdef USE_BMP180
  float T = bmp180->readTemperature();
  sensorData->pressure = bmp180->readPressure(T);
#elif defined(USE_BMP280) || defined(USE_BME280)
  sensorData->pressure = bmp280->readPressure();
#endif
}

/*
   The settings as HTML code.
*/
void HydroMonitorPressureSensor::settingsHtml(ESP8266WebServer * server) {
  server->sendContent_P(PSTR("\
      <tr>\n\
        <th colspan=\"2\">Atmospheric pressure sensor settings.</th>\n\
      </tr><tr>\n\
        <td>Elevation:</td>\n\
        <td><input type=\"number\" step=\"0.1\" name=\"pressure_altitude\" value=\""));
  server->sendContent(String(settings.altitude));
  server->sendContent_P(PSTR("\"> meters above sealevel.</td>\n\
      </tr>\n");
}

/*
   The sensor data as HTML code.
*/
void HydroMonitorPressureSensor::dataHtml(ESP8266WebServer * server) {
  server->sendContent_P(PSTR("\n
                             <tr>\n\
        <td>Atmospheric pressure</td>\n\
        <td>"));
  if (sensorData->pressure < 0) {
    server->sendContent_P(PSTR("Sensor not connected.</td>\n\
      </tr>"));
  }
  else {
    server->sendContent(sensorData->pressure);
    server->sendContent_P(PSTR(" mbar.</td>\n\
      </tr>"));
  }
}

/*
   Update the settings.
*/
void HydroMonitorPressureSensor::updateSettings(String keys[], String values[], uint8_t nArgs) {
  for (uint8_t i = 0; i < nArgs; i++) {
    if (keys[i] == "pressure_altitude") {
      if (core.isNumeric(values[i])) {
        float val = values[i].toFloat();
        if (val > -100 && val < 10000) settings.altitude = val;
      }
    }
  }
  EEPROM.put(PRESSURE_SENSOR_EEPROM, settings);
  EEPROM.commit();
}
#endif

