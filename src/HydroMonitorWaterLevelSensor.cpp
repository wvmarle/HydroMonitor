#include <HydroMonitorWaterLevelSensor.h>
#include <Average.h>

HydroMonitorWaterLevelSensor::HydroMonitorWaterLevelSensor() {
  lastWarned = millis() - WARNING_INTERVAL;
}


#ifdef USE_WATERLEVEL_SENSOR
// The below set of #ifdef tags splits the function; the first bit is for the specific way the
// trigPin is connected; the last chunk is the same for all three options.

// It's possible of course to have echoPin also on a port expander, this is not implemented as there is
// no need and this particular sensor is probably obsolete for the whole project anyway.

#ifdef USE_HCSR04
/*
 * Ultrasound sensor, trig pin connected through a MCP23008 port expander.
 */
#if defined(TRIG_MCP_PIN)
void HydroMonitorWaterLevelSensor::begin(HydroMonitorMySQL *l, Adafruit_MCP23008 *mcp) {

  // Set the parameters.
  mcp23008 = mcp;
  mcp23008->pinMode(TRIG_MCP_PIN, OUTPUT);
  l->writeTesting("HydroMonitorWaterLevelSensor: configured HC-SR04 sensor with trig pin on MCP port expander.");
  
#elif defined(TRIG_PCF_PIN)
/*
 * Ultrasound sensor, trig pin connected through a PCF8574 port expander.
 */
void HydroMonitorWaterLevelSensor::begin(HydroMonitorMySQL *l, PCF857x *pcf) {

  // Set the parameters.
  pcf8574 = pcf;
  l->writeTesting("HydroMonitorWaterLevelSensor: configured HC-SR04 sensor with trig pin on PCF port expander.");

#elif defined(TRIG_PIN)
/*
 * Ultrasound sensor, trig pin connected directly.
 */
void HydroMonitorWaterLevelSensor::begin(HydroMonitorMySQL *l) {

  // Set the parameters.
  pinMode(TRIG_PIN, OUTPUT);
  l->writeTesting("HydroMonitorWaterLevelSensor: configured HC-SR04 sensor.");
#endif
  
  pinMode(ECHO_PIN, INPUT);
  l->writeTesting("HydroMonitorWaterLevelSensor: set up HC-SR04 sensor.");
#endif

#ifdef USE_MS5837
/*
 * MS5837 pressure sensor.
 */
void HydroMonitorWaterLevelSensor::begin(HydroMonitorMySQL *l, MS5837 *ms) {

  // Set the parameters.
  ms5837 = ms;
  l->writeTesting("HydroMonitorWaterLevelSensor: set up MS5837 sensor.");
#endif

  logging = l;
  if (WATERLEVEL_SENSOR_EEPROM > 0)
    EEPROM.get(WATERLEVEL_SENSOR_EEPROM, settings);
    
  // Check whether any reasonable settings have been set, if not apply defaults.
  if (settings.reservoirHeight < 1 or settings.reservoirHeight > 500) {
    l->writeTesting("HydroMonitorWaterLevelSensor: applying default settings.");
    settings.reservoirHeight = 30;
    settings.zeroLevel = 0;
  }
  return;
}
#endif

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
float HydroMonitorWaterLevelSensor::readSensor() {

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
    fill = 100.0 * (settings.reservoirHeight - reading) / (settings.reservoirHeight - 2);
  else 
    fill = -1;

  warning();
  return fill;
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
  pcf8574->write (TRIG_PCF_PIN, LOW);
  delayMicroseconds(2); 
  pcf8574->write (TRIG_PCF_PIN, HIGH);
  delayMicroseconds(10);
  pcf8574->write (TRIG_PCF_PIN, LOW);
  delayMicroseconds(2);
#elif defined(TRIG_MCP_PIN)
  mcp23008->digitalWrite (TRIG_MCP_PIN, LOW);
  delayMicroseconds(2); 
  mcp23008->digitalWrite (TRIG_MCP_PIN, HIGH);
  delayMicroseconds(10);
  mcp23008->digitalWrite (TRIG_MCP_PIN, LOW);
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
#endif

/*
 * Measure water level using the MS5837 pressure sensor.
 * Requires the atmospheric pressure as compensation.
 */
#ifdef USE_MS5837
float HydroMonitorWaterLevelSensor::readSensor(float p) {
  float fill = -1;
  
  // Get the water level in cm.
  // The reservoir is considered "full" at 95% of the total level.
  float reading = ms5837->readWaterLevel(p) + settings.zeroLevel;
  if (reading > 0 && reading < settings.reservoirHeight)
    fill = 100.0 * reading / (0.95 * settings.reservoirHeight);
    
  warning();
  return fill;
}


/*
 * Use the current reading as zero water level (there's always a small difference between
 * the two sensors), and store the difference in EEPROM.
 */
void HydroMonitorWaterLevelSensor::setZero(float p) {

  // Get the water level in cm.
  float reading = ms5837->readWaterLevel(p);
  settings.zeroLevel = p - reading;
  EEPROM.put(WATERLEVEL_SENSOR_EEPROM, settings);
  EEPROM.commit();
  return;
}
#endif

void HydroMonitorWaterLevelSensor::warning() {

   // Send warning if it's been long enough ago & fill is <20%.
  if (millis() - lastWarned > WARNING_INTERVAL && fill >= 0 && fill < 20) {
    char message[110] = "The reservoir is almost empty, and is in urgent need of a refill.\nCurrent reservoir fill level: ";
    char buf[5];
    snprintf(buf, 5, "%f", fill);
    strcat(message, buf);
    strcat(message, "%.");
    logging->sendWarning(message);
  }
}

/*
 * The settings as html.
 */
String HydroMonitorWaterLevelSensor::settingsHtml() {
  String html = F("\
      <tr>\n\
        <th colspan=\"2\">Water Level Sensor Settings.</th>\n\
      </tr><tr>\n\
        <td>Distance from sensor to bottom of the reservoir:</td>\n\
        <td><input type=\"number\" step=\"0.1\" name=\"waterlevel_reservoirheight\" value=\"");
  html += String(settings.reservoirHeight);
  html += F("\"> cm.</td>\n");
#ifdef USE_MS5837
  html += F("      </tr><tr>\n\
        <td></td>\n\
        <td><input type=\"submit\" formaction=\"/zero_reservoir_level\" formmethod=\"post\" name=\"zero_level\" value=\"Set zero level\"></td>\n");
#endif
  html += F("</tr>\n");
  return html;
}

/*
 * The sensor data as html.
 */
String HydroMonitorWaterLevelSensor::dataHtml() {
  String html = F("<tr>\n\
    <td>Reservoir water level</td>\n\
    <td>");
  if (fill < 0) html += F("Sensor not connected.</td>\n\
  </tr>");
  else {
    html += String(fill);
    html += F(" % full.</td>\n\
  </tr>");
  }
  return html;
}

/*
 * Process the settings from the key/value pairs.
 */
void HydroMonitorWaterLevelSensor::updateSettings(String keys[], String values[], uint8_t nArgs) {
  for (uint8_t i=0; i<nArgs; i++) {
    if (keys[i] == "waterlevel_reservoirheight") {
      if (core.isNumeric(values[i])) {
        float val = values[i].toFloat();
        if (val > 0 && val <= 200) settings.reservoirHeight = val;
      }
    }
  }
  EEPROM.put(WATERLEVEL_SENSOR_EEPROM, settings);
  EEPROM.commit();
  return;
}

