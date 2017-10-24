#include <HydroMonitorHumiditySensor.h>

/**
 * Manages the relative humidity sensor.
 * Constructor.
 *
 */
HydroMonitorHumiditySensor::HydroMonitorHumiditySensor () {
  humidity = -1;
}

/*
 * Configure the sensor as DHT22.
 */
#ifdef USE_HUMIDITY_SENSOR
#ifdef DHT22_PIN
void HydroMonitorHumiditySensor::begin(HydroMonitorMySQL *l, DHT22 *dht) {
  dht22 = dht;
  l->writeInfo("HydroMonitorHumiditySensor: configured DHT22 sensor.");
#endif

/*
 * Configure the sensor as BME280.
 */
#ifdef USE_BME280
void HydroMonitorHumiditySensor::begin(HydroMonitorMySQL *l, BME280 *bme) {
  bme280 = bme;
  l->writeInfo("HydroMonitorHumiditySensor: configured BME280 sensor.");
#endif
  logging = l;
  if (HUMIDITY_SENSOR_EEPROM > 0)
    EEPROM.get(HUMIDITY_SENSOR_EEPROM, settings);

  return;
}
#endif
  
/*
 * Take a measurement from the sensor.
 */
float HydroMonitorHumiditySensor::readSensor() {
#ifdef USE_BME280
  humidity = (float)bme280->readHumidity();
#elif defined(DHT22_PIN)
  humidity = (float)dht22->readHumidity();
#endif
  return humidity;
}

/*
 * Calculate the dewpoint - the temperature (in °C) at which condensation will take place.
 */
float calcDewpoint(float hum, float temp) {
  float k;
  k = log(hum/100) + (17.62 * temp) / (243.12 + temp);
  return 243.12 * k / (17.62 - k);
}
	
/*
 * The sensor settings as html.
 */
String HydroMonitorHumiditySensor::settingsHtml(void) {
  return "";
}

/*
 * The sensor data as html.
 */
String HydroMonitorHumiditySensor::dataHtml(void) {
  String html = F("<tr>\n\
    <td>Relative humidity</td>\n\
    <td>");
  if (humidity < 0) html += F("Sensor not connected.</td>\n\
  </tr>");
  else {
    html += String(humidity);
    html += F(" %.</td>\n\
  </tr>");
  }
  return html;
}

/*
 * Update the settings for this sensor, if any.
 */
void HydroMonitorHumiditySensor::updateSettings(String keys[], String values[], uint8_t nArgs) {
  return;
}

