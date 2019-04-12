#include <HydroMonitorHumiditySensor.h>

#ifdef USE_HUMIDITY_SENSOR

/**
 * Manages the relative humidity sensor.
 * Constructor.
 *
 */
HydroMonitorHumiditySensor::HydroMonitorHumiditySensor () {
}

/*
 * Configure the sensor as DHT22.
 */
#ifdef DHT22_PIN
void HydroMonitorHumiditySensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l, DHT22 *dht) {
  dht22 = dht;
  l->writeInfo(F("HydroMonitorHumiditySensor: configured DHT22 sensor."));
#endif

/*
 * Configure the sensor as BME280.
 */
#ifdef USE_BME280
void HydroMonitorHumiditySensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l, BME280 *bme) {
  bme280 = bme;
  l->writeInfo(F("HydroMonitorHumiditySensor: configured BME280 sensor."));
#endif
  sensorData = sd;
  logging = l;
  if (HUMIDITY_SENSOR_EEPROM > 0)
    EEPROM.get(HUMIDITY_SENSOR_EEPROM, settings);

  return;
}
  
/*
 * Take a measurement from the sensor.
 */
void HydroMonitorHumiditySensor::readSensor() {
#ifdef USE_BME280
  sensorData->humidity = (float)bme280->readHumidity();
#elif defined(DHT22_PIN)
  sensorData->humidity = (float)dht22->readHumidity();
#endif
  return;
}

/*
 * Calculate the dewpoint - the temperature (in °C) at which condensation will take place.
 */
float HydroMonitorHumiditySensor::calcDewpoint(float hum, float temp) {
  float k;
  k = log(hum/100) + (17.62 * temp) / (243.12 + temp);
  return 243.12 * k / (17.62 - k);
}
	
/*
 * Calculate the dewpoint - for the current situation.
 */
float HydroMonitorHumiditySensor::calcDewpoint() {
  float k;
  k = log(sensorData->humidity/100) + (17.62 * sensorData->temperature) / (243.12 + sensorData->temperature);
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
  if (sensorData->humidity < 0) html += F("Sensor not connected.</td>\n\
  </tr>");
  else {
    html += String(sensorData->humidity);
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

#endif

