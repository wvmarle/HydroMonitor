#include <HydroMonitorTemperatureSensor.h>
#include <Arduino.h>

#ifdef USE_TEMPERATURE_SENSOR
/*
   Measure the air temperature.
   Constructor.
*/
HydroMonitorTemperatureSensor::HydroMonitorTemperatureSensor () {
  lastWarned = millis() - WARNING_INTERVAL;
}

/*
   Configure the sensor as DHT22.
*/
#ifdef USE_DHT22
void HydroMonitorTemperatureSensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l, DHT22 *dht) {
  dht22 = dht;
  logging = l;
  sensorData = sd;
  logging->writeTrace(F("HydroMonitorTemperatureSensor: configured DHT22 sensor."));
  if (TEMPERATURE_SENSOR_EEPROM > 0) {
    EEPROM.get(TEMPERATURE_SENSOR_EEPROM, settings);
  }
}
#endif

/*
   Configure the sensor as BMP180.
*/
#ifdef USE_BMP180
void HydroMonitorTemperatureSensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l, BMP180 *bmp) {
  bmp180 = bmp;
  l->writeTrace(F("HydroMonitorTemperatureSensor: configured BMP180 sensor."));

  /*
     Configure the sensor as BMP280 or BME280.
  */
#elif defined(USE_BMP280) || defined(USE_BME280)
void HydroMonitorTemperatureSensor::begin(HydroMonitorCore::SensorData * sd, HydroMonitorLogging * l, BME280 * bmp) {
  bmp280 = bmp;
  l->writeTrace(F("HydroMonitorTemperatureSensor: configured BMP280 sensor."));
#endif

#if defined(USE_BMP180) || defined(USE_BMP280) || defined(USE_BME280)
  sensorData = sd;
  logging = l;
  if (TEMPERATURE_SENSOR_EEPROM > 0) {
    EEPROM.get(TEMPERATURE_SENSOR_EEPROM, settings);
  }
}
#endif

/*
   Take a measurement from the sensor.
*/
void HydroMonitorTemperatureSensor::readSensor(bool readNow) {
  static uint32_t lastReadSensor = -REFRESH_SENSORS;
  if (millis() - lastReadSensor > REFRESH_SENSORS ||
      readNow) {
#if defined(USE_BMP280) || defined(USE_BME280)
    sensorData->temperature = (float)bmp280->readTemperature();
#elif defined(USE_BMP180)
    sensorData->temperature = (float)bmp180->readTemperature();
#elif defined(USE_DHT22)
    sensorData->temperature = (float)dht22->readTemperature();
#endif
    if ((sensorData->temperature > 0 && sensorData->temperature < 10) ||
        sensorData->temperature > 60 &&
        millis() - lastWarned > WARNING_INTERVAL) {
      char buff[90];
      sprintf_P(buff, PSTR("HydroMonitorTemperatureSensor: unusual temperature of %2.2f°C measured."), sensorData->temperature);
      logging->writeWarning(buff);
      lastWarned = millis();
    }
  }
}

/*
   The sensor settings as html.
*/
void HydroMonitorTemperatureSensor::settingsHtml(ESP8266WebServer * server) {
}

bool HydroMonitorTemperatureSensor::settingsJSON(ESP8266WebServer * server) {
  return false;
}

/*
   The sensor data as html.
*/
void HydroMonitorTemperatureSensor::dataHtml(ESP8266WebServer * server) {
  server->sendContent_P(PSTR("\
      <tr>\n\
        <td>Air temperature</td>\n\
        <td>"));
  if (sensorData->temperature < 0) {
    server->sendContent_P(PSTR("Sensor not connected.</td>\n\
      </tr>"));
  }
  else {
    server->sendContent(String(sensorData->temperature));
    server->sendContent_P(PSTR(" &deg;C.</td>\n\
      </tr>"));
  }
}

/*
   Update the settings for this sensor, if any.
*/
void HydroMonitorTemperatureSensor::updateSettings(ESP8266WebServer * server) {
}
#endif

