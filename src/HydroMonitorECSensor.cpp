#include <HydroMonitorECSensor.h>

#ifdef USE_EC_SENSOR
float CYCLETIME = 12.5;  // The time (in nanoseconds) of each processor cycle - 12.5 ns at 80 MHz.
static volatile uint32_t endCycle; // Used in the interrupt handler: stores the cycle number when the sampling is complete.

/*
 * Measure the EC value.
 */
HydroMonitorECSensor::HydroMonitorECSensor () { 
  calibratedSlope = 1;
  calibratedIntercept = 0;
  EC = -1;
  solutionVolume = 0;
  concentration = 0;
  targetEC = 0;
  lastWarned = millis() - WARNING_INTERVAL;
}

/*
 * Set up the sensor.
 */
void HydroMonitorECSensor::begin (HydroMonitorMySQL *l) {
  logging = l;
  logging->writeTesting("HydroMonitorECSensor: configured EC sensor.");
  if (EC_SENSOR_EEPROM > 0)
    EEPROM.get(EC_SENSOR_EEPROM, settings);
  
  // Get the calibration data.
  readCalibration();
  return;
}

/*
 * Read the calibration data from EEPROM, and calculate the slope and intercept parameters
 * with it.
 */
 
void HydroMonitorECSensor::readCalibration() {

  // Retrieve the calibration data, and calculate the slope and intercept.
  core.readCalibration(EC_SENSOR_CALIBRATION_EEPROM, timestamp, ECValue, reading, enabled);
  
  // The discharge time is linear with 1/EC, so we have to take the reciprocals.
  float oneOverECValue[DATAPOINTS];
  for (int i=0; i<DATAPOINTS; i++) {
    if (ECValue[i] > 0 && enabled[i]) oneOverECValue[i] = 1.0/ECValue[i];
    else oneOverECValue[i] = 0;
  }

  // Calculate the slope.
  core.leastSquares(oneOverECValue, reading, DATAPOINTS, &calibratedSlope, &calibratedIntercept);
  return;
}

/*
 * Take a measurement from the sensor.
 */
float HydroMonitorECSensor::readSensor(float waterTemp) {
  float ECValue;
  uint32_t reading = takeReading();
  if (reading > 0) {
    temperatureCorrection(&reading, waterTemp);
    EC = calibratedSlope / (reading - calibratedIntercept);

    // Send warning if it's been long enough ago & EC is >30% below target.
    if (millis() - lastWarned > WARNING_INTERVAL && EC < 0.7 * targetEC) {
      lastWarned = millis();
      char message[115] = "EC level is too low; additional fertiliser is urgently needed.\nTarget set: ";
      char buf[5];
      snprintf(buf, 5, "%f", targetEC);
      strcat(message, buf);
      strcat(message, " mS/cm, current EC: ");
      snprintf(buf, 5, "%f", EC);
      strcat(message, buf);
      strcat(message, " mS/cm.");
      logging->sendWarning(message);
    }
    return EC;
  }
  return -1;
}

/**
 * As ion activity changes drastically with the temperature of the liquid, we have to correct for that. The 
 * temperature correction is a simple "linear correction", typical value for this ALPHA factor is 2%/degC.
 * 
 * Source and more information:
 * https://www.analyticexpert.com/2011/03/temperature-compensation-algorithms-for-conductivity/
 *
 * Experimenally found for this system: 3.1%/degC gives the most straight line, so this is used for temperature
 * compensation.
 */
void HydroMonitorECSensor::temperatureCorrection(uint32_t *reading, float waterTemp) {
  const float ALPHA = 0.031;
  if (waterTemp > 0) {
    *reading = (double)*reading / (1 + ALPHA * (waterTemp - 25)); // temperature correction.
  }
  return;
}

/**
 * capacitor based TDS measurement
 * pin CapPos ---------------------------+------------+
 *                                       |            |
 *                                10-47 nF cap     EC probe or
 *                                       |         resistor (for simulation)
 * pin CapNeg ----- 330 ohm resistor ----+            |
 *                                                    |
 * pin ECpin -----------------------------------------+
 * 
 * So, what's going on here?
 * EC - electic conductivity - is the reciprocal of the resistance of the liquid.
 * So we have to measure the resistance, but this can not be done directly as running
 * a DC current through an ionic liquid just doesn't work, as we get electrolysis and
 * the migration of ions to the respective electrodes.
 * 
 * So this routing is using the pins of the microprocessor and a capacitor to produce a
 * high frequency AC current (at least 1 kHz, best 3 kHz - based on the pulse length, but the
 * pulses come at intervals). Alternating the direction of the current in these 
 * short pulses prevents the problems mentioned above. Maximum frequency should be about
 * 500 kHz (i.e. pulse length about 1 microsecond).
 *
 * To get the needed timing resolution, especially for higher EC values, we measure
 * clock pulses rather than using micros().
 * 
 * Then to get the resistance it is not possible to measure the voltage over the
 * EC probe (the normal way of measuring electrical resistance) as this drops with
 * the capacitor discharging. Instead we measure the time it takes for the cap to
 * discharge enough for the voltage on the pin to drop so much, that the input
 * flips from High to Low state. This time taken is a direct measure of the
 * resistance encountered (the cap and the EC probe form an RC circuit) in the
 * system, and that's what we need to know.
 * 
 * Now the working of this technique.
 * Stage 1: charge the cap full through pin CapPos.
 * Stage 2: let the cap drain through the EC probe, measure the time it takes from
 * flipping the pins until CapPos drops LOW.
 * Stage 3: charge the cap full with opposite charge.
 * Stage 4: let the cap drain through the EC probe, for the same period of time as
 * was measured in Stage 2 (as compensation).
 * Cap is a small capacitor, in this system we use 10-47 nF but with other probes a
 * larger or smaller value can be required (the original research this is based
 * upon used a 3.3 nF cap). Resistor R1 is there to protect pin CapPos and 
 * CapNeg from being overloaded when the cap is charged up, resistor R2
 * protects ECpin from too high currents caused by very high EC or shorting the
 * probe.
 * 
 * Pins set to input are assumed to have infinite impedance, leaking is not taken into
 * account. The specs of NodeMCU give some 66 MOhm for impedance, several orders of
 * magnitude above the typical 1-100 kOhm resistance encountered by the EC probe.
 * 
 * Original research this is based upon:
 * https://hal.inria.fr/file/index/docid/635652/filename/TDS_Logger_RJP2011.pdf
 * 
 */

uint32_t HydroMonitorECSensor::takeReading() {

  uint32_t dischargeCycles = 0;             // The number of clock cycles it took for the capacitor to discharge.
  uint32_t totalCycles = 0;                 // The cumulative number of clock cycles over all measurements.
  uint32_t chargeDelay = 80;                // The time (in microseconds) given to the cap to fully charge/discharge - at least 5x RC.
                                            // 330 Ohm x 47 nF = 15.5 microseconds RC constant.
  uint32_t timeout = 2000;                  // discharge timeout in microseconds - if not triggered within this time, the EC probe 
                                            // is probably not connected or not in the liquid.
                                            // 2000 us makes for a 250 Hz signal; well below the minimum 1000 Hz needed for accurate readings.
  uint32_t startCycle;                      // The clock cycle count at which the measurement starts.
  uint32_t startTime;                       // The micros() count at which the measurement starts (for timeout).
  for (uint16_t i=0; i< (1 << ECSAMPLES); i++) {  // take 2^ECSAMPLES measurements of the EC.

    // Stage 1: fully charge capacitor for positive cycle.
    // CapPos output high, CapNeg output low, ECpin input.
    pinMode (EC_PIN, INPUT);
    pinMode (CAPPOS_PIN,OUTPUT);
    pinMode (CAPNEG_PIN, OUTPUT);
    digitalWrite(CAPPOS_PIN, HIGH);
    digitalWrite(CAPNEG_PIN, LOW);
    delayMicroseconds(chargeDelay);         // allow the cap to charge fully.

    // Stage 2: positive side discharge; measure time it takes.
    // CapPos input, CapNeg output low, ECpin output low.
    endCycle = 0;
    startTime = micros();
    pinMode (CAPPOS_PIN,INPUT);
    
    // Use cycle counts and an interrupt for the most precise time measurement possible with the ESP8266.
    //
    // Important: 
    // startCycle must be set before the interrupt is attached, as in some cases the interrupt can come 
    // in almost instantly. In that case the may be that endCycle is less than startCycle and stuff
    // starts to go terribly wrong.
    startCycle = ESP.getCycleCount();
    attachInterrupt(digitalPinToInterrupt(CAPPOS_PIN), reinterpret_cast<void (*)()>(&capDischarged), FALLING);
    pinMode(EC_PIN, OUTPUT);
    digitalWrite(EC_PIN, LOW);
    
    // No yield() in this loop as we really don't want the ESP8266 to occupy itself and throw off the
    // timing accuracy. The timeout here is 2 ms so that's safe for the WDT.
    while (endCycle == 0) { // Gets set in the ISR, when an interrupt is received.
      if (micros() - startTime > timeout) break;
    }
    detachInterrupt(digitalPinToInterrupt(CAPPOS_PIN));
    if (endCycle == 0) 
      dischargeCycles = 0;
    else {
      dischargeCycles = endCycle - startCycle;
      totalCycles += dischargeCycles;
    }
    
    // Stage 3: fully charge capacitor for negative cycle. CapPos output low, CapNeg output high, ECpin input.
    pinMode (EC_PIN, INPUT); 
    pinMode (CAPPOS_PIN,OUTPUT);
    digitalWrite (CAPPOS_PIN, LOW);
    pinMode (CAPNEG_PIN, OUTPUT);
    digitalWrite (CAPNEG_PIN, HIGH);
    delayMicroseconds (chargeDelay);

    // Stage 4: negative side charge; don't measure as we just want to balance it the directions.
    // CapPos input, CapNeg high, ECpin high.
    pinMode (CAPPOS_PIN,INPUT);
    pinMode (EC_PIN, OUTPUT); 
    digitalWrite (EC_PIN, HIGH);
    if (dischargeCycles)
      delayMicroseconds (dischargeCycles * CYCLETIME/1000);
    else
      delayMicroseconds (timeout);
      
    delay(0); // For the ESP8266: allow for background processes to run.
  }

  // Stop any charge from flowing while we're not measuring by setting all ports to INPUT.
  // CapNeg may have a pull up or pull down resistor attached; this doesn't matter as it's blocked by the
  // capacitor.
  pinMode (CAPPOS_PIN, INPUT);
  pinMode (CAPNEG_PIN, INPUT);
  pinMode (EC_PIN, INPUT);
  dischargeCycles = (totalCycles >> ECSAMPLES);

  delay(0); // For the ESP8266: allow for background processes to run.
  return dischargeCycles;
}

/*
 * The ISR which registers when the cap has discharged to the point the pin flips.
 */
void ICACHE_RAM_ATTR HydroMonitorECSensor::capDischarged() {
  endCycle = ESP.getCycleCount();
}

void HydroMonitorECSensor::setSolutionVolume(uint16_t v) {
  solutionVolume = v;
}

void HydroMonitorECSensor::setFertiliserConcentration(uint16_t c) {
  concentration = c;
}

void HydroMonitorECSensor::setTargetEC(float t) {
  targetEC = t;
}

/*
 * The sensor settings as html.
 */
String HydroMonitorECSensor::settingsHtml() {
  String html;
  html = F("\
      <tr>\n\
        <th colspan=\"2\">EC Sensor settings.</th>\n\
      </tr><tr>\n\
        <td></td>\n\
        <td><input type=\"submit\" formaction=\"/calibrate_ec\" formmethod=\"post\" name=\"calibrate\" value=\"Calibrate now\"></td>\n\
      </tr>");
  return html;
}

/*
 * The sensor settings as html.
 */
String HydroMonitorECSensor::dataHtml() {  
  String html= F("<tr>\n\
    <td>Water conductivity</td>\n\
    <td>");
  if (EC < 0) {
    html += F("Sensor not connected.</td>\n\
  </tr>");
  }
  else {
    String extraLine = "";
    
    // 40% off target - red.
    if (EC > 1.4 * targetEC || EC < 0.6 * targetEC) {
      html += F("<span style=\"color:red\">");
      if (EC < targetEC && concentration > 0) {
        extraLine = "\n\
  <tr>\n\
    <td>Amount of fertiliser to be added:</td><td>";
        extraLine += String((int)((targetEC - EC) * 1000 * solutionVolume/concentration));
        extraLine += "ml of each A and B.</td>\n\
  </tr>";
      }
    }
    
    // 20% off target - yellow.
    else if (EC > 1.2 * targetEC || EC < 0.8 * targetEC) {
      html += F("<span style=\"color:yellow\">");
      if (EC < targetEC && concentration > 0) {
        extraLine = "\n\
  <tr>\n\
    <td>Amount of fertiliser to be added:</td><td>";
        extraLine += String((int)((targetEC - EC) * 1000 * solutionVolume/concentration));
        extraLine += "ml of each A and B.</td>\n\
  </tr>";
      }
    }
    
    // Within 20% of target - green.
    else {
      html += F("<span style=\"color:green\">");
    }
    html += String(EC);
    html += F("</span> mS/cm.</td>\n\
  </tr>");
    html += extraLine;
  }
  return html;
}

/*
 * Get a list of past calibrations in html format.
 */
String HydroMonitorECSensor::getCalibrationHtml() {
  return core.calibrationHtml("EC Sensor", "/calibrate_ec_action", timestamp, ECValue, reading, enabled);
}

/*
 * Get a list of past calibrations in json format.
 */ 
String HydroMonitorECSensor::getCalibrationData() {
  return core.calibrationData(timestamp, ECValue, reading, enabled);
}

/*
 * Handle the calibration of the sensor.
 */
void HydroMonitorECSensor::doCalibration(ESP8266WebServer *server, float waterTemp) {
  if (server->hasArg("delete")) {           // User requests deletion of a data point.
    String argVal = server->arg("delete");  // The value of the delete argument is the point we have to clear.
    if (core.isNumeric(argVal)) {
      uint8_t val = argVal.toInt();
      if (val < DATAPOINTS) {               // Make sure it's a valid value.
        timestamp[val] = 0;
        ECValue[val] = 0;
        reading[val] = 0;
        enabled[val] = false;
      }
    }
  }
  else if (server->hasArg("calibrate")) {   // User requests to do a calibration.
    if (server->hasArg("value")) {
      String argVal = server->arg("value"); // The value for which we take the reading.
      if (argVal != "") {                   // if there's a value given, use this to create a calibration point.
        if (core.isNumeric(argVal)) {
          float val = argVal.toFloat();
          uint32_t res = takeReading();  // Take the raw sensor reading.
          temperatureCorrection (&res, waterTemp);
          
          // Find the first available data point where the value can be stored.
          for (uint8_t i=0; i<DATAPOINTS; i++) {
            if (timestamp[i] == 0) {        // empty timestamp: data point is available.
              timestamp[i] = now();
              ECValue[i] = val;
              reading[i] = res;
              enabled[i] = true;
              break;
            }
          }
        }
      }
    }
  }
  else {  // No other commands, so one of the checkboxes was toggled. Set the enabled list accordingly.
    for (uint8_t i=0; i<DATAPOINTS; i++) {
      String key = "enable";
      key += i;                             // Look for argument called enable0, enable1, etc.
      if (server->hasArg(key)) enabled[i] = true;   // If the argument is present, the checkbox is ticked.
      else enabled[i] = false;              // If not present, the checkbox is unticked.
    }
  }
  
  // No more arguments to test for. Store the calibration.
  core.writeCalibration(EC_SENSOR_CALIBRATION_EEPROM, timestamp, ECValue, reading, enabled);
              
  // Re-read the calibration values and update the EC probe parameters.
  readCalibration();
  return;
}

/*
 * Update the settings for this sensor, if any.
 */
void HydroMonitorECSensor::updateSettings(String keys[], String values[], uint8_t nArgs) {
  return;
}
#endif
