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

void HydroMonitorPressureSensor::readSensor(bool readNow) {
  static uint32_t lastReadSensor = -REFRESH_SENSORS;
  if (millis() - lastReadSensor > REFRESH_SENSORS ||
      readNow) {
#ifdef USE_BMP180
    float T = bmp180->readTemperature();
    sensorData->pressure = bmp180->readPressure(T);
#elif defined(USE_BMP280) || defined(USE_BME280)
    sensorData->pressure = bmp280->readPressure();
#endif
  }
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
  char buff[10];
  sprintf_P(buff, PSTR("%.1f"), settings.altitude);
  server->sendContent(buff);
  server->sendContent_P(PSTR("\"> meters above sealevel.</td>\n\
      </tr>\n"));
}

/*
   The settings as JSON.
*/
bool HydroMonitorPressureSensor::settingsJSON(ESP8266WebServer * server) {
  char buff[10];
  server->sendContent_P(PSTR("  \"pressure_sensor\": {\n"
                             "    \"pressure_altitude\":\""));
  sprintf_P(buff, PSTR("%6.1f"), settings.altitude);
  server->sendContent(buff);
  server->sendContent_P(PSTR("\"\n"
                             "  }"));
  return true;
}
  

/*
   The sensor data as HTML code.
*/
void HydroMonitorPressureSensor::dataHtml(ESP8266WebServer * server) {
  server->sendContent_P(PSTR("\n\
                             <tr>\n\
        <td>Atmospheric pressure</td>\n\
        <td>"));
  if (sensorData->pressure < 0) {
    server->sendContent_P(PSTR("Sensor not connected.</td>\n\
      </tr>"));
  }
  else {
    char buff[10];
    sprintf_P(buff, PSTR("%5.1f"), sensorData->pressure);
    server->sendContent(buff);
    server->sendContent_P(PSTR(" mbar.</td>\n\
      </tr>"));
  }
}

/*
   Update the settings.
*/
void HydroMonitorPressureSensor::updateSettings(ESP8266WebServer *server) {
  for (uint8_t i = 0; i < server->args(); i++) {
    if (server->argName(i) == "pressure_altitude") {
      if (core.isNumeric(server->arg(i))) {
        float val = server->arg(i).toFloat();
        if (val > -100 && val < 10000) {
          settings.altitude = val;
        }
      }
    }
  }
#ifdef USE_24LC256_EEPROM
  sensorData->EEPROM->put(WATERLEVEL_SENSOR_EEPROM, settings);
#else
  EEPROM.put(WATERLEVEL_SENSOR_EEPROM, settings);
  EEPROM.commit();
#endif
}
#endif

