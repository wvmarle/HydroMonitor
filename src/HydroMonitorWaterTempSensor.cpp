/*
 * Measure the water temperature through NTC or MS5837 sensor.
 */
#include <HydroMonitorWaterTempSensor.h>

#ifdef USE_WATERTEMPERATURE_SENSOR
 
HydroMonitorWaterTempSensor::HydroMonitorWaterTempSensor () {
}

/*
 * Configure the sensor.
 */
#ifdef USE_NTC
#ifdef NTC_ADS_PIN
void HydroMonitorWaterTempSensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorMySQL *l, Adafruit_ADS1115 *ads) {
  ads1115 = ads;
  l->writeTesting("HydroMonitorWaterTempSensor: configured NTC probe on ADS port expander.");
  
#elif defined(NTC_PIN)
void HydroMonitorWaterTempSensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorMySQL *l) {
  l->writeTesting("HydroMonitorWaterTempSensor: configured NTC probe.");
#endif
#endif

#ifdef USE_MS5837
void HydroMonitorWaterTempSensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorMySQL *l, MS5837 *ms) {
  ms5837 = ms;
  l->writeTesting("HydroMonitorWaterTempSensor: configured MS5837 sensor.");
#endif
  sensorData = sd;
  logging = l;
  if (WATERTEMPERATURE_SENSOR_EEPROM > 0)
    EEPROM.get(WATERTEMPERATURE_SENSOR_EEPROM, settings);
  return;
}

/*
 * Get a reading from the sensor.
 */
void HydroMonitorWaterTempSensor::readSensor() {

#ifdef USE_NTC 
  // read the value from the NTC sensor:
  uint16_t reading = 0;
  
  // Check whether the NTC sensor is present.
#ifdef NTC_ADS_PIN
  reading = ads1115->readADC_SingleEnded(NTC_ADS_PIN);
#elif defined(NTC_PIN)
  reading = analogRead(NTC_PIN);
#else
#error no ntc pin defined.
#endif
  if (reading < 0.03*ADCMAX || reading > 0.97*ADCMAX) {
    sensorData->waterTemp = -1;
    return;
  }

  for (uint8_t i = 0; i < (1 << NTCSAMPLES) - 1; i++) {
#ifdef NTC_ADS_PIN
    reading += ads1115->readADC_SingleEnded(NTC_ADS_PIN);
#elif defined(NTC_PIN)
    reading += analogRead(NTC_PIN);
#endif
    delay(10);
    yield();
  }
  
  //Calculate temperature using the Beta Factor equation
  sensorData->waterTemp = 1.0 / (log (NTCSERIESRESISTOR / ((ADCMAX / (reading >> NTCSAMPLES) - 1) * THERMISTORNOMINAL)) / BCOEFFICIENT + 1.0 / (TEMPERATURENOMINAL + 273.15)) - 273.15;
#elif defined(USE_MS5837)
  sensorData->waterTemp = ms5837->readTemperature();
#endif
  return;
}

/*
 * The settings as html.
 */
String HydroMonitorWaterTempSensor::settingsHtml() {
  return "";
}

/*
 * The sensor data as html.
 */
String HydroMonitorWaterTempSensor::dataHtml() {
  String html = F("<tr>\n\
    <td>Water temperature</td>\n\
    <td>");  
  if (sensorData->waterTemp < 0) html += F("Sensor not connected.</td>\n\
  </tr>");
  else {
    html += String(sensorData->waterTemp);
    html += F(" &deg;C.</td>\n\
  </tr>");
  }
  return html;
}

/*
 * Process the settings from the key/value pairs.
 */
 void HydroMonitorWaterTempSensor::updateSettings(String keys[], String values[], uint8_t nArgs) {
  return;
}
#endif

