#include <HydroMonitorWaterLevelSensor.h>
#include <Average.h>

#ifdef USE_WATERLEVEL_SENSOR
HydroMonitorWaterLevelSensor::HydroMonitorWaterLevelSensor() {
  lastWarned = millis() - WARNING_INTERVAL;
}

// The below set of #ifdef tags splits the function; the first bit is for the specific way the
// trigPin is connected; the last chunk is the same for all three options.

// It's possible of course to have echoPin also on a port expander, this is not implemented as there is
// no need and this particular sensor is probably obsolete for the whole project anyway.

#ifdef USE_HCSR04
/*
 * Ultrasound sensor, trig pin connected through a MCP23008 port expander.
 */
#if defined(TRIG_MCP_PIN)
void HydroMonitorWaterLevelSensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l, Adafruit_MCP23008 *mcp) {

  // Set the parameters.
  mcp23008 = mcp;
  mcp23008->pinMode(TRIG_MCP_PIN, OUTPUT);
  l->writeTrace(F("HydroMonitorWaterLevelSensor: configured HC-SR04 sensor with trig pin on MCP port expander."));
  
#elif defined(TRIG_PCF_PIN)
/*
 * Ultrasound sensor, trig pin connected through a PCF8574 port expander.
 */
void HydroMonitorWaterLevelSensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l, PCF857x *pcf) {

  // Set the parameters.
  pcf8574 = pcf;
  l->writeTrace(F("HydroMonitorWaterLevelSensor: configured HC-SR04 sensor with trig pin on PCF port expander."));

#elif defined(TRIG_PIN)
/*
 * Ultrasound sensor, trig pin connected directly.
 */
void HydroMonitorWaterLevelSensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l) {

  // Set the parameters.
  pinMode(TRIG_PIN, OUTPUT);
  l->writeTrace(F("HydroMonitorWaterLevelSensor: configured HC-SR04 sensor."));
#endif
  
  pinMode(ECHO_PIN, INPUT);
  l->writeTrace(F("HydroMonitorWaterLevelSensor: set up HC-SR04 sensor."));

/*
 * MS5837 pressure sensor.
 */
#elif defined(USE_MS5837)
void HydroMonitorWaterLevelSensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l, MS5837 *ms) {

  // Set the parameters.
  ms5837 = ms;
  l->writeTrace(F("HydroMonitorWaterLevelSensor: set up MS5837 sensor."));
  
/*
 * DS1603L ultrasound sensor.
 */
#elif defined(USE_DS1603L)
void HydroMonitorWaterLevelSensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l, DS1603L *ds) {

  // Set the parameters.
  ds1603l = ds;
  
/*
 * MPXV5004 pressure sensor.
 */
#elif defined (USE_MPXV5004)
void HydroMonitorWaterLevelSensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l) {

/*
 * Three float switches.
 */
#elif defined (USE_FLOATSWITCHES)
#if defined(FLOATSWITCH_HIGH_MCP17_PIN) || defined(FLOATSWITCH_MEDIUM_MCP17_PIN) || defined(FLOATSWITCH_LOW_MCP17_PIN)
void HydroMonitorWaterLevelSensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l,  Adafruit_MCP23017 *mcp) {
  mcp23017 = mcp;
#else
void HydroMonitorWaterLevelSensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l) {
#endif

#ifdef FLOATSWITCH_HIGH_MCP17_PIN
  mcp23017->pinMode(FLOATSWITCH_HIGH_MCP17_PIN, INPUT);
  mcp23017->pullUp(FLOATSWITCH_HIGH_MCP17_PIN, true);
#else
  pinMode(FLOATSWITCH_HIGH_PIN, INPUT_PULLUP);
#endif

#ifdef FLOATSWITCH_MEDIUM_MCP17_PIN
  mcp23017->pinMode(FLOATSWITCH_MEDIUM_MCP17_PIN, INPUT);
  mcp23017->pullUp(FLOATSWITCH_MEDIUM_MCP17_PIN, true);
#else
  pinMode(FLOATSWITCH_MEDIUM_PIN, INPUT_PULLUP);
#endif

#ifdef FLOATSWITCH_LOW_MCP17_PIN
  mcp23017->pinMode(FLOATSWITCH_LOW_MCP17_PIN, INPUT);
  mcp23017->pullUp(FLOATSWITCH_LOW_MCP17_PIN, true);
#else
  pinMode(FLOATSWITCH_LOW_PIN, INPUT_PULLUP);
#endif

#endif

  logging = l;
  sensorData = sd;
  if (WATERLEVEL_SENSOR_EEPROM > 0)
#ifdef USE_24LC256_EEPROM
    sensorData->EEPROM->get(WATERLEVEL_SENSOR_EEPROM, settings);
#else
    EEPROM.get(WATERLEVEL_SENSOR_EEPROM, settings);
#endif
    
  // Check whether any reasonable settings have been set, if not apply defaults.
  // Note: for some sensors the reservoir heights are in cm, for others it's the ADC reading.
  if (settings.reservoirHeight < 1 || settings.reservoirHeight > 1024) {
    l->writeTrace(F("HydroMonitorWaterLevelSensor: applying default settings."));
    settings.reservoirHeight = 30;
    settings.zeroLevel = 0;
#ifdef USE_24LC256_EEPROM
    sensorData->EEPROM->put(WATERLEVEL_SENSOR_EEPROM, settings);
#else
    EEPROM.put(WATERLEVEL_SENSOR_EEPROM, settings);
    EEPROM.commit();
#endif
  }
  return;
}

#ifdef USE_HCSR04
/*
 * HC-SR04 distance sensor
 * 
 * To improve on the stability, a number of measurements is taken. We want
 * to have HCSR04SAMPLES good readings, but will take a maximum of 
 * 2 * HCSR04SAMPLES measurements for that. The level is the average 
 * of those HCSR04SAMPLES readings.
 *
 * The sensor returns the level in fill % (where 100% is 2 cm below the sensor).
 */
void HydroMonitorWaterLevelSensor::readSensor() {

  float reading = 0;

  for (uint8_t i = 0; i < (1 << HCSR04SAMPLES); i++) {
    reading += measureLevel();
    delay(10); // to make sure the echoes have died out
  }
  reading /= (1 << HCSR04SAMPLES);
    
  // Only calculate the fill level for readings that have a positive value and are less than the
  // reservoirheight. Any readings outside that range are impossible and considered invalid.
  // The reservoir is considered 100% full at 2 cm below the sensor, which is the minimum distance for
  // it to measure - and a minimum safe distance between the water and the sensor.
  if (reading > 0 && reading < settings.reservoirHeight)
    sensorData->waterLevel = 100.0 * (settings.reservoirHeight - reading) / (settings.reservoirHeight - 2);
  else 
    sensorData->waterLevel = -1;

  warning();
  return;
}

/*
 * This function triggers the device and then reads the result, and calculates the distance
 * between the probe and the water from that.
 */
float HydroMonitorWaterLevelSensor::measureLevel() {
  float duration = 0;
  float distance = 0;

  // Calculate the pulse_in timeout: 1 1/2 times the maximum roundtrip based on the reservoir
  // height set by the user.
  // Speed of sound = 0.03 cm/microsecond, 33 1/3 microsecond per cm, times 3 for (roundtrip * 1.5) 
  // gives a timeout of 100 microseconds per cm reservervoir height.
  uint16_t timeout = settings.reservoirHeight * 100;
  
  // Trigger the measurement.
#ifdef TRIG_PCF_PIN
  pcf8574->write(TRIG_PCF_PIN, LOW);
  delayMicroseconds(2); 
  pcf8574->write(TRIG_PCF_PIN, HIGH);
  delayMicroseconds(10);
  pcf8574->write(TRIG_PCF_PIN, LOW);
  delayMicroseconds(2);
#elif defined(TRIG_MCP_PIN)
  mcp23008->digitalWrite(TRIG_MCP_PIN, LOW);
  delayMicroseconds(2); 
  mcp23008->digitalWrite(TRIG_MCP_PIN, HIGH);
  delayMicroseconds(10);
  mcp23008->digitalWrite(TRIG_MCP_PIN, LOW);
  delayMicroseconds(2);
#elif defined(TRIG_PIN)
  digitalWrite(TRIG_PIN, LOW); 
  delayMicroseconds(2); 
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
#else
#error no trigpin defined.
#endif
  
  // Get the result.
  duration = pulseIn(ECHO_PIN, HIGH, timeout);
  distance = (duration/2) / 29.1;
  delay(0);
  if (distance == 0) 
    return -1;    // No valid reading received.

  return distance;
}

/*
 * Measure water level using the MS5837 pressure sensor.
 * Requires the atmospheric pressure as compensation.
 */
#elif defined(USE_MS5837)
void HydroMonitorWaterLevelSensor::readSensor() {
  sensorData->waterLevel = -1;
  
  // Get the water level in cm.
  // The reservoir is considered "full" at 95% of the total level.
  float reading = ms5837->readWaterLevel(sensorData->pressure) - settings.zeroLevel;
  if (reading > 0 && reading < settings.reservoirHeight) {
    sensorData->waterLevel = 100.0 * reading / (0.95 * settings.reservoirHeight);
  }
  else {
    sensorData->waterLevel = -1;
  }
  bitWrite(sensorData->systemStatus, STATUS_RESERVOIR_LEVEL_LOW, sensorData->waterLevel < 40);  
  warning();
  return;
}

/*
 * Use the current reading as zero water level (there's always a small difference between
 * the two sensors), and store the difference in EEPROM.
 */
void HydroMonitorWaterLevelSensor::setZero() {

  // Get the water level in cm.
  float reading = ms5837->readWaterLevel(sensorData->pressure);
  settings.zeroLevel = reading;
#ifdef USE_24LC256_EEPROM
  sensorData->EEPROM->put(WATERLEVEL_SENSOR_EEPROM, settings);
#else
  EEPROM.put(WATERLEVEL_SENSOR_EEPROM, settings);
  EEPROM.commit();
#endif
  return;
}

/*
 * Measure water level using the DS1603L ultrasound sensor.
 */
#elif defined(USE_DS1603L)
void HydroMonitorWaterLevelSensor::readSensor() {
  
  // Get the water level in cm.
  // Sensor returns the value in mm as uint16_t, we divide this by 10 to get to cm.
  // The reservoir is considered "full" at 95% of the total level.
  float reading = ds1603l->readSensor() / 10.0;
  if (reading > 0 && reading < settings.reservoirHeight * 1.5) {
    sensorData->waterLevel = 100 * reading / (0.95 * settings.reservoirHeight);
  }
  else {
    sensorData->waterLevel = -1;
  }
  bitWrite(sensorData->systemStatus, STATUS_RESERVOIR_LEVEL_LOW, sensorData->waterLevel < 40);  
  warning();
}

/*
 * Measure water level using the MPXV5004 or MP3V5004 (or similar) pressure sensor.
 */
#elif defined(USE_MPXV5004)
void HydroMonitorWaterLevelSensor::readSensor() {
  uint16_t reading = analogRead(MPXV5004_PIN);
  float waterLevel = 100.0 * (reading - settings.zeroLevel) / (settings.reservoirHeight - settings.zeroLevel);
  if (isnan(waterLevel)) {
    waterLevel = -1;
  }
  sensorData->waterLevel = waterLevel;
  bitWrite(sensorData->systemStatus, STATUS_RESERVOIR_LEVEL_LOW, sensorData->waterLevel < 40);  
  warning();
}

/*
 * Measure the zero offset of the sensor - typically 0.6V at no pressure difference between the two inputs.
 */
void HydroMonitorWaterLevelSensor::setZero() {
  float reading = analogRead(MPXV5004_PIN);
  settings.zeroLevel = reading;
#ifdef USE_24LC256_EEPROM
  sensorData->EEPROM->put(WATERLEVEL_SENSOR_EEPROM, settings);
#else
  EEPROM.put(WATERLEVEL_SENSOR_EEPROM, settings);
  EEPROM.commit();
#endif
  return;
}

/*
 * Measure the maximum water level - the 100% level as given by the user.
 */
void HydroMonitorWaterLevelSensor::setMax() {
  float reading = analogRead(MPXV5004_PIN);
  settings.reservoirHeight = reading;
#ifdef USE_24LC256_EEPROM
  sensorData->EEPROM->put(WATERLEVEL_SENSOR_EEPROM, settings);
#else
  EEPROM.put(WATERLEVEL_SENSOR_EEPROM, settings);
  EEPROM.commit();
#endif
  return;
}

/*
 * Measure the water level using three float switches (giving high, medium and low level).
 */
#elif defined (USE_FLOATSWITCHES)
void HydroMonitorWaterLevelSensor::readSensor() {
  bool high, medium, low;
#ifdef FLOATSWITCH_HIGH_MCP17_PIN
  high = mcp23017->digitalRead(FLOATSWITCH_HIGH_MCP17_PIN);
#else
  high = digitalRead(FLOATSWITCH_HIGH_PIN);
#endif
#ifdef FLOATSWITCH_MEDIUM_MCP17_PIN
  medium = mcp23017->digitalRead(FLOATSWITCH_MEDIUM_MCP17_PIN);
#else
  medium = digitalRead(FLOATSWITCH_MEDIUM_PIN);
#endif
#ifdef FLOATSWITCH_LOW_MCP17_PIN
  low = mcp23017->digitalRead(FLOATSWITCH_LOW_MCP17_PIN);
#else
  low = digitalRead(FLOATSWITCH_LOW_PIN);
#endif
  if (high) {
    sensorData->waterLevel = 100;
  }
  else if (medium) {
    sensorData->waterLevel = 70;
  }
  else if (low) {
    sensorData->waterLevel = 30;
  }
  else {
    sensorData->waterLevel = 0;
  }
}
#endif

void HydroMonitorWaterLevelSensor::warning() {

   // Send warning if it's been long enough ago & fill is <20%.
  if (millis() - lastWarned > WARNING_INTERVAL && sensorData->waterLevel >= 0 && sensorData->waterLevel < 20) {
    lastWarned += WARNING_INTERVAL;
    char message[140];
    snprintf_P(message, 140, PSTR("HydroMonitorWaterLevelSensor: the reservoir is almost empty, and is in urgent need of a refill. Current reservoir fill level: %.1f %%."), sensorData->waterLevel);
    logging->writeWarning(message);
  }
}

/*
 * The settings as html.
 */
void HydroMonitorWaterLevelSensor::settingsHtml(ESP8266WebServer *server) {
  char buff[10];
  server->sendContent_P(PSTR("\
      <tr>\n\
        <th colspan=\"2\">Water Level Sensor Settings.</th>\n\
      </tr><tr>"));
#ifdef USE_MPXV5004:
  server->sendContent_P(PSTR("\
        <td></td>\n\
        <td><input type=\"submit\" formaction=\"/max_reservoir_level\" formmethod=\"post\" name=\"max_level\" value=\"Measure maximum level\"></td>\n\
        <td>Current reading: "));
  sprintf_P(buff, PSTR("%.0f"), settings.reservoirHeight);
  server->sendContent(buff);
  server->sendContent_P(PSTR("</td>\n"));
#else
  server->sendContent_P(PSTR("\
        <td>Maximum water depth:</td>\n\
        <td><input type=\"number\" step=\"0.1\" name=\"waterlevel_reservoirheight\" value=\""));
  sprintf_P(buff, PSTR("%.1f"), settings.reservoirHeight);
  server->sendContent(buff);
  server->sendContent_P(PSTR("\"> cm.</td>\n"));
#endif

#ifdef USE_MS5837
  server->sendContent_P(PSTR("\
      </tr><tr>\n\
        <td></td>\n\
        <td><input type=\"submit\" formaction=\"/zero_reservoir_level\" formmethod=\"post\" name=\"zero_level\" value=\"Measure zero level\"></td>\n"));
#elif defined(USE_MPXV5004)
  server->sendContent_P(PSTR("\
      </tr><tr>\n\
        <td></td>\n\
        <td><input type=\"submit\" formaction=\"/zero_reservoir_level\" formmethod=\"post\" name=\"zero_level\" value=\"Measure zero level\"></td>\n\
        <td>Current reading: "));
  sprintf_P(buff, PSTR("%.0f"), settings.zeroLevel);
  server->sendContent(buff);
  server->sendContent_P(PSTR("</td>\n"));
#endif
  server->sendContent_P(PSTR("</tr>\n"));
}


/*
 * The settings as JSON.
 */
bool HydroMonitorWaterLevelSensor::settingsJSON(ESP8266WebServer *server) {
  char buff[10];
  server->sendContent_P(PSTR("  \"waterlevel_sensor\": {\n"
                             "    \"max_level\":\""));
  sprintf_P(buff, PSTR("%.0f"), settings.reservoirHeight);
  server->sendContent(buff);
  server->sendContent_P(PSTR("\",\n"
                             "    \"zero_level\":\""));
  sprintf_P(buff, PSTR("%.0f"), settings.zeroLevel);
  server->sendContent(buff);
  server->sendContent_P(PSTR("\"\n"
                             "  }"));
  return true;
}

/*
 * The sensor data as html.
 */
void HydroMonitorWaterLevelSensor::dataHtml(ESP8266WebServer *server) {
  char buff[10];
  server->sendContent_P(PSTR("<tr>\n\
    <td>Reservoir water level</td>\n\
    <td>"));
  if (sensorData->waterLevel < 0) {
    server->sendContent_P(PSTR("Sensor not connected.</td>\n\
  </tr>"));
  }
  else {
    sprintf_P(buff, PSTR("%.1f"), sensorData->waterLevel);
    server->sendContent(buff);
    server->sendContent_P(PSTR(" % full.</td>\n\
  </tr>"));
  }
}

/*
 * Process the settings from the key/value pairs.
 */
void HydroMonitorWaterLevelSensor::updateSettings(ESP8266WebServer* server) {
  for (uint8_t i=0; i<server->args(); i++) {
    if (server->argName(i) == "waterlevel_reservoirheight") {
      if (core.isNumeric(server->arg(i))) {
        float val = server->arg(i).toFloat();
        if (val > 0 && val <= 200) settings.reservoirHeight = val;
      }
    }
  }
#ifdef USE_24LC256_EEPROM
  sensorData->EEPROM->put(WATERLEVEL_SENSOR_EEPROM, settings);
#else
  EEPROM.put(WATERLEVEL_SENSOR_EEPROM, settings);
  EEPROM.commit();
#endif
  return;
}
#endif

