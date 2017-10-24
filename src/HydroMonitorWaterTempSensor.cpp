/*
 * Measure the water temperature through NTC or MS5837 sensor.
 */

#include <HydroMonitorWaterTempSensor.h>
 
HydroMonitorWaterTempSensor::HydroMonitorWaterTempSensor () {
  waterTemp = -1;
}

#ifdef USE_WATERTEMPERATURE_SENSOR
/*
 * Configure the sensor.
 */
#ifdef USE_NTC
#ifdef NTC_ADS_PIN
void HydroMonitorWaterTempSensor::begin(HydroMonitorMySQL *l, Adafruit_ADS1115 *ads) {
  ads1115 = ads;
  l->writeTesting("HydroMonitorWaterTempSensor: configured NTC probe on ADS port expander.");
  
#elif defined(NTC_PIN)
void HydroMonitorWaterTempSensor::begin(HydroMonitorMySQL *l) {
  l->writeTesting("HydroMonitorWaterTempSensor: configured NTC probe.");
#endif
#endif

#ifdef USE_MS5837
void HydroMonitorWaterTempSensor::begin(HydroMonitorMySQL *l, MS5837 *ms) {
  ms5837 = ms;
  l->writeTesting("HydroMonitorWaterTempSensor: configured MS5837 sensor.");
#endif
  logging = l;
  if (WATERTEMPERATURE_SENSOR_EEPROM > 0)
    EEPROM.get(WATERTEMPERATURE_SENSOR_EEPROM, settings);
  return;
}
#endif

/*
 * Get a reading from the sensor.
 */
float HydroMonitorWaterTempSensor::readSensor() {

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
  if (reading < 0.03*ADCMAX || reading > 0.97*ADCMAX) 
    return -1;

  for (uint8_t i = 0; (1 << NTCSAMPLES) - 1; i++) {
#ifdef NTC_ADS_PIN
    reading += ads1115->readADC_SingleEnded(NTC_ADS_PIN);
#elif defined(NTC_PIN)
    reading += analogRead(NTC_PIN);
#endif
    delay(10);
    yield();
  }
  //Calculate temperature using the Beta Factor equation
  waterTemp = 1.0 / (log (NTCSERIESRESISTOR / ((ADCMAX / (reading >> NTCSAMPLES) - 1) * THERMISTORNOMINAL)) / BCOEFFICIENT + 1.0 / (TEMPERATURENOMINAL + 273.15)) - 273.15;
#elif defined(USE_MS5837)
  waterTemp = ms5837->readTemperature();
#endif
  return waterTemp;
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
  if (waterTemp < 0) html += F("Sensor not connected.</td>\n\
  </tr>");
  else {
    html += String(waterTemp);
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
