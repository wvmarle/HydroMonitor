#include <HydroMonitorECSensor.h>
#include <Average.h>

double CYCLETIME = 12.5;  // The time (in nanoseconds) of each processor cycle - 12.5 ns at 80 MHz.
static volatile unsigned long endCycle; // Used in the interrupt handler: stores the cycle number when the sampling is complete.

/*
 * Measure the EC value.
 */
HydroMonitorECSensor::HydroMonitorECSensor () { 
}

void HydroMonitorECSensor::begin (Settings s, unsigned char cp, unsigned char cn, unsigned char ec) {
  capPos = cp;
  capNeg = cn;
  ECpin = ec;

  setSettings(s);
  return;
}

void HydroMonitorECSensor::setSettings(Settings s) {

  // Retrieve and set the Settings.
  settings = s;
  
  // Check whether any sensible settings have been set, if not apply defaults.
  if (settings.Samples == 0 || settings.Samples > 500) {
    settings.CalibratedSlope = 0.0002;
    settings.CalibratedIntercept = 1.5;
    settings.Samples = 100;
  }
  return;
}

/**
 * capacitor based TDS measurement
 * pin CapPos ----- 330 ohm resistor R1 -|------------|                                             
 *                                       |            |
 *                                      cap        EC probe or
 *                                       |         resistor (for simulation)
 * pin CapNeg ---------------------------|            |
 *                                                    |
 * pin ECpin ------ 330 ohm resistor R2 --------------|
 * 
 * So, what's going on here?
 * EC - electic conductivity - is the reciprocal of the resistance of the liquid.
 * So we have to measure the resistance, but this can not be done directly as running
 * a DC current through an ionic liquid just doesn't work, as we get electrolysis and
 * the migration of ions to the respective electrodes.
 * 
 * So this routing is using the pins of the NodeMCU and a capacitor to produce a
 * high frequency AC current (1 kHz or more - based on the pulse length, but the
 * pulses come at intervals). Alternating the direction of the current in these 
 * short pulses prevents the problems mentioned above.
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
 * Cap is a small capacitor, in this system we use 47 nF but with other probes a
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
 * This function uses delay() in various forms, no yield() or so as it's meant to
 * be a real time measurement. Yield()ing to the task scheduler is a bad idea.
 * With the measurement taking only just over 0.1 seconds this should not be an
 * issue.
 * 
 * Original research this is based upon:
 * https://hal.inria.fr/file/index/docid/635652/filename/TDS_Logger_RJP2011.pdf
 * 
 */


double HydroMonitorECSensor::readSensor(double) {

  unsigned int dischargeTime;    // the time it took for the capacitor to discharge.
  unsigned int chargeDelay = 100; // The time (in microseconds) given to the cap to fully charge/discharge - at least 5x RC.
  unsigned int timeout = 2;      // discharge timeout in milliseconds - if not triggered within this time, the EC probe 
                                 // is probably not connected or not in the liquid.
  
  delay(100);
  Average<unsigned int> discharge(settings.Samples); // The sampling results.
  double ECValue;
  double ALPHA = 0.02;
  for (int i=0; i<settings.Samples; i++) { // take <settings.Samples> measurements of the EC.

    // Stage 1: fully charge capacitor for positive cycle.
    // CapPos output high, CapNeg output low, ECpin input.
    pinMode (ECpin, INPUT);
    pinMode (capPos,OUTPUT);
    pinMode (capNeg, OUTPUT);
    digitalWrite(capPos, HIGH);
    digitalWrite(capNeg, LOW);
    delayMicroseconds(chargeDelay); // allow the cap to charge fully.
    yield();

    // Stage 2: positive side discharge; measure time it takes.
    // CapPos input, CapNeg output low, ECpin output low.
    endCycle = 0;
    startTime = millis();
    pinMode (capPos,INPUT);
    
    // Use cycle counts and an interrupt for the most precise time measurement possible with the ESP8266.
    //
    // Important: 
    // startCycle must be set before the interrupt is attached, as in some cases the interrupt can come 
    // in almost instantly. In that case the may be that endCycle is less than startCycle and stuff
    // starts to go terribly wrong.
    startCycle = ESP.getCycleCount(); 
    attachInterrupt(digitalPinToInterrupt(capPos), reinterpret_cast<void (*)()>(&capDischarged), FALLING);
    pinMode (ECpin, OUTPUT);
    digitalWrite(ECpin, LOW);
    
    // No yield() in this loop as we really don't want the ESP8266 to occupy itself and throw off the
    // timeing accuracy. The timeout here is 2 ms so that's safe for the WDT.
    while (endCycle == 0) {
      if (millis() > (startTime + timeout)) break;
    }
    detachInterrupt(digitalPinToInterrupt(capPos));
    if (endCycle == 0) dischargeTime = 0;
    else {
  
      // Handle potential overflow of micros() just as we measure, this happens about every 54 seconds
      // on a 80-MHz board.
      if (endCycle < startCycle) dischargeTime = (4294967295 - startCycle + endCycle) * CYCLETIME;
      else dischargeTime = (endCycle - startCycle) * CYCLETIME;
      discharge.push(dischargeTime);
    }
    yield();
    
    // Stage 3: fully charge capacitor for negative cycle. CapPos output low, CapNeg output high, ECpin input.
    pinMode (ECpin, INPUT); 
    pinMode (capPos,OUTPUT);
    digitalWrite(capPos, LOW);
    pinMode (capNeg, OUTPUT);
    digitalWrite(capNeg, HIGH);
    delayMicroseconds(chargeDelay);
    yield();

    // Stage 4: negative side charge; don't measure as we just want to balance it the directions.
    // CapPos input, CapNeg high, ECpin high.
    pinMode (capPos,INPUT);
    pinMode (ECpin, OUTPUT); 
    digitalWrite(ECpin, HIGH);
    delayMicroseconds(dischargeTime/1000);
    yield();
  }

  // Stop any charge from flowing while we're not measuring by setting all ports to OUTPUT, LOW.
  // This will of course also completely discharge the capacitor.
  pinMode (capPos, OUTPUT);
  digitalWrite (capPos, LOW);
  pinMode (capNeg, OUTPUT);
  digitalWrite (capNeg, LOW);
  pinMode (ECpin, OUTPUT);
  digitalWrite (ECpin, LOW);
  double dischargeAverage = discharge.mean();
    
  /**
   * Calculate EC from the discharge time.
   * 
   * As ion activity changes drastically with the temperature of the liquid, we have to correct for that. The 
   * temperature correction is a simple "linear correction", typical value for this ALPHA factor is 2%/degC.
   * 
   * Source and more information:
   * https://www.analyticexpert.com/2011/03/temperature-compensation-algorithms-for-conductivity/
   */

  if (dischargeAverage > 0) {
    ECValue = 1 / (settings.CalibratedSlope * dischargeAverage + settings.CalibratedIntercept);
    
    // Only do temperature correction if we actually have a valid temperature!
    // Negative values (i.e. temperatures below freezing - which should never happen in the first place) are 
    // used to indicate no probe present or other errors.
    if (waterTemp > 0) {
      ECValue /= (1 + ALPHA * (waterTemp - 25)); // temperature correction.
    }
  }
  else ECValue = -1;
  yield();
  if (calibrationMode) return dischargeAverage;
  return ECValue;
}

// Upon interrupt: register the cycle count of when the cap has discharged.
void ICACHE_RAM_ATTR HydroMonitorECSensor::capDischarged() {
  endCycle = ESP.getCycleCount();
}

void HydroMonitorECSensor::setCalibrationMode (bool b) {
  calibrationMode = b;
  return;
}

String HydroMonitorECSensor::settingsHtml(void) {
  String html;
  html = F("\
      <tr>\
        <th colspan=\"2\">EC Sensor settings.</th>\
      </tr><tr>\
        <td>Number of samples:</td>\
        <td><input type=\"number\" name=\"ec_samples\" value=\"");
  html += String(settings.Samples);
  html += F("\"></td>\
      </tr><tr>\
        <td>Calibrated slope:</td>\
        <td>");
  html += String(settings.CalibratedSlope, 8);
  html += F("</td>\
      </tr><tr>\
        <td>calibrated intercept:</td>\
        <td>");
  html += String(settings.CalibratedIntercept, 3);
  html += F("&nbsp;&nbsp;<input type=\"submit\" formaction=\"/calibrateECSensor\" formmethod=\"post\" name=\"calibrate\" value=\"Calibrate now\">  </td>\
      </tr>");
  return html;
}


