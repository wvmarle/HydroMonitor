/*
 * Measure the water temperature through NTC or MS5837 sensor.
 */
#include <HydroMonitorWaterTempSensor.h>

#ifdef USE_WATERTEMPERATURE_SENSOR
 
HydroMonitorWaterTempSensor::HydroMonitorWaterTempSensor () {
  lastWarned = millis() - WARNING_INTERVAL;
}

/*
 * Configure the sensor.
 */
#ifdef USE_NTC
#ifdef NTC_ADS_PIN
void HydroMonitorWaterTempSensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l, Adafruit_ADS1115 *ads) {
  ads1115 = ads;
  l->writeTrace(F("HydroMonitorWaterTempSensor: configured NTC probe on ADS port expander."));
  
#elif defined(NTC_PIN)
void HydroMonitorWaterTempSensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l) {
  l->writeTrace(F("HydroMonitorWaterTempSensor: configured NTC probe."));
#endif

#elif defined(USE_MS5837)
void HydroMonitorWaterTempSensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l, MS5837 *ms) {
  ms5837 = ms;
  l->writeTrace(F("HydroMonitorWaterTempSensor: configured MS5837 sensor."));

#elif defined(USE_DS18B20)
  void HydroMonitorWaterTempSensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l, DallasTemperature *ds, OneWire *oneWire) {
  ds18b20 = ds;
  ds18b20->begin();
  ds18b20->setWaitForConversion(false);
  startConversion();
  if (sensorPresent) {
    l->writeTrace(F("WaterTempSensor: configured DS18B20 sensor."));
  }
  else {
    l->writeError(F("WaterTempSensor 10: no DS18B20 temperature sensor found."));
  }  
  
#elif defined(USE_ISOLATED_SENSOR_BOARD)
void HydroMonitorWaterTempSensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l) {
  l->writeTrace(F("HydroMonitorWaterTempSensor: configured isolated sensor board."));
#endif

  sensorData = sd;
  logging = l;
  if (WATERTEMPERATURE_SENSOR_EEPROM > 0)
#ifdef USE_24LC256_EEPROM
    sensorData->EEPROM->get(WATERTEMPERATURE_SENSOR_EEPROM, settings);
#else
    EEPROM.get(WATERTEMPERATURE_SENSOR_EEPROM, settings);
#endif
  return;
}

/*
 * Get a reading from the sensor.
 */
void HydroMonitorWaterTempSensor::readSensor(bool readNow) {
  
////////////////////////////////////////////////////////////
// Code for reading the temperature using an NTC probe.
#ifdef USE_NTC 
  static uint32_t lastReadSensor = -REFRESH_SENSORS;
  if (millis() - lastReadSensor > REFRESH_SENSORS ||
      readNow) {
    lastReadSensor = millis();
    uint32_t reading = 0;
  
  // Check whether the NTC sensor is present.
#ifdef NTC_ADS_PIN
    reading = ads1115->readADC_SingleEnded(NTC_ADS_PIN);
#elif defined(NTC_PIN)
    reading = analogRead(NTC_PIN);
#else
#error no ntc pin defined.
#endif
    if (reading < 0.03*ADCMAX || reading > 0.97*ADCMAX) {
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
//  reading = reading >> NTCSAMPLES;
    sensorData->waterTemp = 1.0 / (log (NTCSERIESRESISTOR / ((ADCMAX / (reading >> NTCSAMPLES) - 1) * THERMISTORNOMINAL)) / BCOEFFICIENT + 1.0 / (TEMPERATURENOMINAL + 273.15)) - 273.15;
  }
  
////////////////////////////////////////////////////////////
// Code for reading the temperature using the MS5837 underwater pressure and temperature sensor.
#elif defined(USE_MS5837)
  static uint32_t lastReadSensor = -REFRESH_SENSORS;
  if (millis() - lastReadSensor > REFRESH_SENSORS ||
      readNow) {
    lastReadSensor = millis();
    sensorData->waterTemp = ms5837->readTemperature();
  }

////////////////////////////////////////////////////////////
// Code for reading the temperature using the DS18B20 digital temperature sensor.
#elif defined(USE_DS18B20)
  static uint32_t lastReadSensor = -REFRESH_SENSORS;
  static bool conversionInProgress;
  static uint32_t temp;
  if (millis() - lastReadSensor > REFRESH_SENSORS ||
      readNow) {
    lastReadSensor = millis();
    conversionInProgress = true;                            // we have to start a new conversion.
    startConversion();
  }

  // When the conversion time is over, read the sensor data.
  if (millis() - lastReadSensor > 750 &&                    // We need at least that much time for a 12-bit conversion.
      conversionInProgress) {
    conversionInProgress = false;
    if (sensorPresent) {                                    // If we detected a sensor,
      sensorData->waterTemp = ds18b20->getTempC(deviceAddress); // take a reading.
      if (sensorData->waterTemp < -100) {                   // Missing sensor returns -127.
        sensorPresent = false;
      }
    }
    else {
      sensorData->waterTemp = -127;
    }
  }

////////////////////////////////////////////////////////////
// Code for if we get the temperature from the isolated sensor board.
#elif defined(USE_ISOLATED_SENSOR_BOARD)
  // Nothing to do here.
#endif
    if (millis() - lastWarned > WARNING_INTERVAL && 
        (sensorData->waterTemp < 5 || sensorData->waterTemp > 70)) {
      lastWarned = millis();
      char message[120];
      sprintf_P(message, PSTR("WaterTempSensor 01: unusual temperature value measured: %2.1f °C. Check water temperature sensor."), 
                sensorData->waterTemp);
      logging->writeWarning(message);
    }

  return;
}

#ifdef USE_DS18B20
void HydroMonitorWaterTempSensor::startConversion() {
  if (sensorPresent == false) {
    sensorPresent = ds18b20->getAddress(deviceAddress, 0);  // Check whether the sensor is connected.
    if (sensorPresent) {                                    // Sensor found, set it up.
      ds18b20->setResolution(deviceAddress, 12);            // 12-bit, 0.0625C resolution - 750 ms conversion time. Higest available.
    }
  }

  // No else if here: sensorPresent may get set above.
  if (sensorPresent) {
    ds18b20->requestTemperatures();                         // Send the command to get temperatures
  }
}
#endif

/*
 * The settings as html.
 */
void HydroMonitorWaterTempSensor::settingsHtml(ESP8266WebServer* server) {
  return;
}

/*
 * The settings as JSON.
 */
bool HydroMonitorWaterTempSensor::settingsJSON(ESP8266WebServer* server) {
  return false; // none.
}

/*
 * The sensor data as html.
 */
void HydroMonitorWaterTempSensor::dataHtml(ESP8266WebServer* server) {
  char buff[10];
  server->sendContent_P(PSTR("<tr>\n\
    <td>Water temperature</td>\n\
    <td>"));  
  if (sensorData->waterTemp < 0) {
    server->sendContent_P(PSTR("Sensor not connected.</td>\n\
  </tr>"));
  }
  else {
    sprintf_P(buff, PSTR("%.1f"), sensorData->waterTemp);
    server->sendContent(buff);
    server->sendContent_P(PSTR(" &deg;C.</td>\n\
  </tr>"));
  }
}

/*
 * Process the settings from the key/value pairs.
 */
 void HydroMonitorWaterTempSensor::updateSettings(ESP8266WebServer* server) {
  return;
}
#endif

