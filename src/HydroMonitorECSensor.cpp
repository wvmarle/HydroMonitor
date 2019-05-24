#include <HydroMonitorECSensor.h>

#ifdef USE_EC_SENSOR
float CYCLETIME = 12.5;  // The time (in nanoseconds) of each processor cycle - 12.5 ns at 80 MHz.
static volatile uint32_t endCycle; // Used in the interrupt handler: stores the cycle number when the sampling is complete.

/**
 * As ion activity changes drastically with the temperature of the liquid, we have to correct for that. The 
 * temperature correction is a simple "linear correction", typical value for this ALPHA factor is 2%/degC.
 * 
 * Source and more information:
 * https://www.analyticexpert.com/2011/03/temperature-compensation-algorithms-for-conductivity/
 */
//const float ALPHA = 0.031;
const float ALPHA = 0.02;

/*
 * Measure the EC value.
 */
HydroMonitorECSensor::HydroMonitorECSensor () { 
  calibratedSlope = 1;
  calibratedIntercept = 0;
  lastWarned = millis() - WARNING_INTERVAL;
}

/*
 * Set up the sensor.
 */
void HydroMonitorECSensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l) {
  logging = l;
  logging->writeTrace(F("HydroMonitorECSensor: configured EC sensor."));
  sensorData = sd;
  if (EC_SENSOR_EEPROM > 0)
#ifdef USE_24LC256_EEPROM
    sensorData->EEPROM->get(EC_SENSOR_EEPROM, settings);
#else
    EEPROM.get(EC_SENSOR_EEPROM, settings);
#endif
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
#ifdef USE_24LC256_EEPROM
  sensorData->EEPROM->get(EC_SENSOR_CALIBRATION_EEPROM, calibrationData); // Read the data.
#else
  EEPROM.get(EC_SENSOR_CALIBRATION_EEPROM, calibrationData); // Read the data.
#endif

  // The discharge time is linear with 1/EC, so we have to take the reciprocals.
  float oneOverECValue[DATAPOINTS];
  uint32_t usedReadings[DATAPOINTS];
  uint8_t nPoints = 0;
  for (int i=0; i<DATAPOINTS; i++) {
    if (calibrationData[i].value > 0 && calibrationData[i].enabled) {
      oneOverECValue[nPoints] = 1.0/calibrationData[i].value;
      usedReadings[nPoints] = calibrationData[i].reading;
      nPoints++;
    }      
  }

  // Calculate the slope.
  core.leastSquares(oneOverECValue, usedReadings, nPoints, &calibratedSlope, &calibratedIntercept);
  return;
}

/*
 * Take a measurement from the sensor.
 */
void HydroMonitorECSensor::readSensor(bool readNow) {
  static uint32_t lastReadSensor = -REFRESH_SENSORS;
  if (millis() - lastReadSensor > REFRESH_SENSORS ||
      readNow) {
    lastReadSensor = millis();
#ifdef USE_ISOLATED_SENSOR_BOARD
    uint32_t reading = sensorData->ecReading;
#else
    uint32_t reading = takeReading();
#endif
    if (reading > 0) {
      sensorData->EC = calibratedSlope / (reading - calibratedIntercept);
      if (sensorData->waterTemp > 0) {
        sensorData->EC = (double)sensorData->EC / (1 + ALPHA * (sensorData->waterTemp - 25)); // temperature correction: measured to nominal.
      }

      // Send warning if it's been long enough ago & EC is >30% below target.
      if (millis() - lastWarned > WARNING_INTERVAL && sensorData->EC < 0.7 * sensorData->targetEC) {
        lastWarned = millis();
        char message[120];
        sprintf_P(message, PSTR("ECSensor 01: EC level is too low; additional fertiliser is urgently needed.\n"
                                "Target set: %2.2f mS/cm, current EC: %2.2f mS/cm."), 
                                sensorData->targetEC, sensorData->EC);
        logging->writeWarning(message);
      }

      // Send warning if EC is exceptionally high.
      if (millis() - lastWarned > WARNING_INTERVAL && sensorData->EC > 5) {
        lastWarned = millis();
        char message[120];
        sprintf_P(message, PSTR("ECSensor 02: EC level is exceptionally high: %2.2f mS/cm. Check sensor."), 
                                sensorData->EC);
        logging->writeWarning(message);
      }
    }
    else {
      sensorData->EC = -1;
      if (millis() - lastWarned > WARNING_INTERVAL) {
        lastWarned = millis();
        char message[120];
        sprintf_P(message, PSTR("ECSensor 03: EC sensor not detected."));
        logging->writeWarning(message);
      }
    }
    Serial.println();
    Serial.print(F("ECSensor: got reading "));
    Serial.print(reading);
    Serial.print(F(" cycles and water temperature "));
    Serial.print(sensorData->waterTemp);
    Serial.print(F(", calculated EC: "));
    Serial.print(sensorData->EC);
    Serial.println(F(" mS/cm."));
  }
}


/**
 * capacitor based TDS measurement
 * pin CapPos ----- 330 ohm resistor ----+------------+
 *                                       |            |
 *                                10-47 nF cap     EC probe or
 *                                       |         resistor (for simulation)
 * pin CapNeg ---------------------------+            |
 *                                                    |
 * pin ECpin ------ 330 ohm resistor -----------------+
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

#ifndef USE_ISOLATED_SENSOR_BOARD
uint32_t HydroMonitorECSensor::takeReading() {

  uint32_t dischargeCycles = 0;                             // The number of clock cycles it took for the capacitor to discharge.
  uint32_t totalCycles = 0;                                 // The cumulative number of clock cycles over all measurements.
  uint32_t chargeDelay = 80;                                // The time (in microseconds) given to the cap to fully charge/discharge - at least 5x RC.
                                                            // 330 Ohm x 47 nF = 15.5 microseconds RC constant.
  uint32_t timeout = 2000;                                  // discharge timeout in microseconds - if not triggered within this time, the EC probe 
                                                            // is probably not connected or not in the liquid.
                                                            // 2000 us makes for a 250 Hz signal; well below the minimum 1000 Hz needed for accurate readings.
  uint32_t startCycle;                                      // The clock cycle count at which the measurement starts.
  uint32_t startTime;                                       // The micros() count at which the measurement starts (for timeout).
  for (uint16_t i=0; i< (1 << ECSAMPLES); i++) {            // take 2^ECSAMPLES measurements of the EC.

    // Stage 1: fully charge capacitor for positive cycle.
    // CapPos output high, CapNeg output low, ECpin input.
    pinMode (EC_PIN, INPUT);
    pinMode (CAPPOS_PIN,OUTPUT);
    pinMode (CAPNEG_PIN, OUTPUT);
    digitalWrite(CAPPOS_PIN, HIGH);
    digitalWrite(CAPNEG_PIN, LOW);
    delayMicroseconds(chargeDelay);                         // allow the cap to charge fully.

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
#endif

/*
 * The sensor settings as html.
 */
void HydroMonitorECSensor::settingsHtml(ESP8266WebServer *server) {
  server->sendContent_P(PSTR("\
      <tr>\n\
        <th colspan=\"2\">EC Sensor settings.</th>\n\
      </tr><tr>\n\
        <td></td>\n\
        <td><input type=\"submit\" formaction=\"/calibrate_ec\" formmethod=\"post\" name=\"calibrate\" value=\"Calibrate now\"></td>\n\
      </tr>"));
}

/*
 * The sensor settings as JSON.
 */
bool HydroMonitorECSensor::settingsJSON(ESP8266WebServer *server) {
  return false; // None.
}

/*
 * The sensor data as html.
 */
void HydroMonitorECSensor::dataHtml(ESP8266WebServer *server) {
  char buff[10];
  server->sendContent_P(PSTR("<tr>\n\
    <td>Water conductivity</td>\n\
    <td>"));
  if (sensorData->EC < 0) {
    server->sendContent_P(PSTR("Sensor not connected.</td>\n\
  </tr>"));
  }
  else {
    if (sensorData->targetEC > 0) {                         // A target is set - colour accordingly.
      if (sensorData->EC > 1.4 * sensorData->targetEC || sensorData->EC < 0.6 * sensorData->targetEC) {
        server->sendContent_P(PSTR("<span style=\"color:red\">")); // 40% off target - red.
      }
      else if (sensorData->EC > 1.2 * sensorData->targetEC || sensorData->EC < 0.8 * sensorData->targetEC) {
        server->sendContent_P(PSTR("<span style=\"color:orange\">")); // 20% off target - orange.
      }
      else {
        server->sendContent_P(PSTR("<span style=\"color:green\">")); // Within 20% of target - green.

      }
    }
    sprintf_P(buff, PSTR("%.2f"), sensorData->EC);
    server->sendContent(buff);
    server->sendContent_P(PSTR("</span> mS/cm.</td>\n\
  </tr>"));
    if (sensorData->EC < 0.8 * sensorData->targetEC) {      // EC low: suggest user to add fertiliser solution.
      if (sensorData->EC < sensorData->targetEC && sensorData->fertiliserConcentration > 0) {
        server->sendContent_P(PSTR("\n\
  <tr>\n\
    <td>Amount of fertiliser to be added:</td><td>"));
        server->sendContent(itoa((sensorData->targetEC - sensorData->EC) * 1000 * sensorData->solutionVolume/sensorData->fertiliserConcentration, buff, 10));
        server->sendContent_P(PSTR("ml of each A and B.</td>\n\
  </tr>"));
      }
    }
  }
}

/*
 * Get a list of past calibrations in html format.
 */
void HydroMonitorECSensor::getCalibrationHtml(ESP8266WebServer *server) {
  core.calibrationHtml(server, "EC Sensor", "/calibrate_ec_action", calibrationData);
}

/*
 * Get a list of past calibrations in json format.
 */ 
void HydroMonitorECSensor::getCalibration(ESP8266WebServer *server) {
  core.calibrationData(server, calibrationData);
}

void HydroMonitorECSensor::doCalibrationAction(ESP8266WebServer *server) {
 if (server->hasArg(F("calibrate"))) {
   doCalibration(server);
 }
 else if (server->hasArg(F("delete"))) {
   deleteCalibration(server);
 }
 else {
   enableCalibration(server);
 }
}

/*
 * Handle the calibration of the sensor.
 */
void HydroMonitorECSensor::doCalibration(ESP8266WebServer *server) {
  if (server->hasArg("value")) {
    String argVal = server->arg("value");                 // The value for which we take the reading.
    if (argVal != "") {                                   // if there's a value given, use this to create a calibration point.
      if (core.isNumeric(argVal)) {
        float nominalValue = argVal.toFloat();
        float EC;
        if (sensorData->waterTemp > 0) {
          EC = (double)nominalValue * (1 + ALPHA * (sensorData->waterTemp - 25)); // temperature correction: nominal to actual.
        }
        else {
          EC = nominalValue;
        }
#ifdef USE_ISOLATED_SENSOR_BOARD
        uint32_t res = sensorData->ecReading;
#else
        uint32_t res = takeReading();
#endif
      
        // Find the first available data point where the value can be stored.
        for (uint8_t i=0; i<DATAPOINTS; i++) {
          if (calibrationData[i].timestamp == 0) {        // empty timestamp: data point is available.
            calibrationData[i].timestamp = now();
            calibrationData[i].value = EC;
            calibrationData[i].nominalValue = nominalValue;
            calibrationData[i].reading = res;
            calibrationData[i].enabled = true;
            break;
          }
        }
        saveCalibrationData();
      }
    }
  }
}

/*
 * Enable/disable calibration points.
 */
void HydroMonitorECSensor::enableCalibration(ESP8266WebServer *server) {

  for (uint8_t i=0; i<DATAPOINTS; i++) {
    String key = "enable";
    key += i;                                             // Look for argument called enable0, enable1, etc.
     calibrationData[i].enabled = (server->hasArg(key));  // If the argument is present, the checkbox is ticked and the datapoint is enabled.
  }
  saveCalibrationData();
}

/*
 * Delete calibration points.
 */
void HydroMonitorECSensor::deleteCalibration(ESP8266WebServer *server) {
  if (server->hasArg(F("delete"))) {                        // User requests deletion of a data point.
    String argVal = server->arg(F("delete"));               // The value of the delete argument is the point we have to clear.
    if (core.isNumeric(argVal)) {
      uint8_t val = argVal.toInt();
      if (val < DATAPOINTS) {                               // Make sure it's a valid value.
        calibrationData[val].timestamp = 0;
        calibrationData[val].value = 0;
        calibrationData[val].reading = 0;
        calibrationData[val].enabled = false;
      }
      saveCalibrationData();
    }
  }
}

/*
 * Save the calibration in EEPROM.
 */
void HydroMonitorECSensor::saveCalibrationData() {
#ifdef USE_24LC256_EEPROM
  sensorData->EEPROM->put(EC_SENSOR_CALIBRATION_EEPROM, calibrationData);
#else
  EEPROM.put(EC_SENSOR_CALIBRATION_EEPROM, calibrationData);
#endif              

  // Re-read the calibration values and update the EC probe parameters.
  readCalibration();
  
  // Take a new reading of the sensor value now we have new calibration values.
  readSensor(true);
  return;
}



/*
 * Update the settings for this sensor, if any.
 */
void HydroMonitorECSensor::updateSettings(ESP8266WebServer* server) {
  return;
}
#endif
