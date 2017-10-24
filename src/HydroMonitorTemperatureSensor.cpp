#include <HydroMonitorTemperatureSensor.h>
#include <Arduino.h>

/*
 * Measure the air temperature.
 * Constructor.
 */
HydroMonitorTemperatureSensor::HydroMonitorTemperatureSensor () {
  temperature = -1;
}

#ifdef USE_TEMPERATURE_SENSOR
/*
 * Configure the sensor as DHT22.
 */
#ifdef DHT22_PIN
void HydroMonitorTemperatureSensor::begin(HydroMonitorMySQL *l, DHT22 *dht) {
  dht22 = dht;
  logging = l;
  logging->writeTesting("HydroMonitorTemperatureSensor: configured DHT22 sensor.");
  if (TEMPERATURE_SENSOR_EEPROM > 0)
    EEPROM.get(TEMPERATURE_SENSOR_EEPROM, settings);
    
  return;
}
#endif
  
/*
 * Configure the sensor as BMP180.
 */
#ifdef USE_BMP180
void HydroMonitorTemperatureSensor::begin(HydroMonitorMySQL *l, BMP180 *bmp) {
  bmp180 = bmp;
  l->writeTesting("HydroMonitorTemperatureSensor: configured BMP180 sensor.");
  
/*
 * Configure the sensor as BMP280 or BME280.
 */
#elif defined(USE_BMP280) || defined(USE_BME280)
void HydroMonitorTemperatureSensor::begin(HydroMonitorMySQL *l, BME280 *bmp) {
  bmp280 = bmp;
  l->writeTesting("HydroMonitorTemperatureSensor: configured BMP280 sensor.");
#endif

#if defined(USE_BMP180) || defined(USE_BMP280) || defined(USE_BME280)
  logging = l;
  if (TEMPERATURE_SENSOR_EEPROM > 0)
    EEPROM.get(TEMPERATURE_SENSOR_EEPROM, settings);
    
  return;
}
#endif
#endif
  
/*
 * Take a measurement from the sensor.
 */
float HydroMonitorTemperatureSensor::readSensor() {
#if defined(USE_BMP280) || defined(USE_BME280)
  temperature = (float)bmp280->readTemperature();
#elif defined(USE_BMP180)
  temperature = (float)bmp180->readTemperature();
#elif defined(USE_DHT22)
  temperature = (float)dht22->readTemperature();
#endif
  return temperature;
}

/*
 * The sensor settings as html.
 */
String HydroMonitorTemperatureSensor::settingsHtml() {
  return "";
}

/*
 * The sensor data as html.
 */
String HydroMonitorTemperatureSensor::dataHtml() {
  String html = F("<tr>\n\
    <td>Air temperature</td>\n\
    <td>");
  if (temperature < 0) html += F("Sensor not connected.</td>\n\
  </tr>");
  else {
    html += String(temperature);
    html += F(" &deg;C.</td>\n\
  </tr>");
  }
  return html;
}

/*
 * Update the settings for this sensor, if any.
 */
void HydroMonitorTemperatureSensor::updateSettings(String keys[], String values[], uint8_t nArgs) {
  return;
}
