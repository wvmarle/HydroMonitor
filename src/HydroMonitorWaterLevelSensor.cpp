#include <HydroMonitorWaterLevelSensor.h>

#include <Average.h>

HydroMonitorWaterLevelSensor::HydroMonitorWaterLevelSensor() {
  sensorPresent = false;
}

PCF857x pcf(0x20, &Wire);

void HydroMonitorWaterLevelSensor::begin(Settings s, unsigned char ep, unsigned char tp, bool PCF) {

  // Set the parameters.
  echoPin = ep;
  trigPin = tp;
  usePCF = PCF;

  setSettings(s);
  return;
}

void HydroMonitorWaterLevelSensor::setSettings(Settings s) {

  // Retrieve and set the Settings.
  settings = s;
  
  // Check whether any settings have been set, if not apply defaults.
  if (settings.Samples < 1 or settings.Samples > 100) {
    settings.Samples = 10;
    settings.ReservoirHeight = 30;
  }
  return;
}

/*
 * HC-SR04 distance sensor
 * 
 * To improve on the stability, a number of measurements is taken. We want
 * to have 10 good readings, but will take a maximum of 20 measurements for
 * that. The level is the average of those 10 readings.
 */
double HydroMonitorWaterLevelSensor::readSensor() {

  Average<float> measurements(settings.Samples);
  int m = 0;

  // Take up to 2*samples measurements of the distance between the probe and the water level.
  for (int i = 0; i < 2 * settings.Samples; i++) {
    if (m < settings.Samples) {
      float measurement = measureLevel();
      if (measurement > 0) {
        measurements.push(measurement);
        m++;
      }
      else if (measurement == 0) {
        sensorPresent = false;
        return -1;
      }
      delay(10); // to make sure the echoes have died out
    }
  }
  sensorPresent = true;
  float lvl = settings.ReservoirHeight - measurements.mean();
  return lvl;
}


/*
 * This function triggers the device and then reads the result, and calculates the distance
 * between the probe and the water from that.
 */
float HydroMonitorWaterLevelSensor::measureLevel() {
  
  float duration, distance;
  
  // Trigger the measurement.
  if (usePCF) {
    pcf.write (trigPin, LOW);
    delayMicroseconds(2); 
    pcf.write (trigPin, HIGH);
    delayMicroseconds(10);
    pcf.write (trigPin, LOW);
  }
  else {
    pinMode(trigPin, OUTPUT);
    digitalWrite(trigPin, LOW); 
    delayMicroseconds(2); 
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    pinMode(trigPin, INPUT);
  }
  // Get the result.
  duration = pulseIn(echoPin, HIGH);
  distance = (duration/2) / 29.1;
  yield();
  if (duration == 0) {
    
    // It seems the device is not present.
    return 0;
  }
  if (distance >= settings.ReservoirHeight * 1.5 || distance <= 0) return -1; // Invalid reading.
  return distance;
}

void HydroMonitorWaterLevelSensor::setPCF8574(PCF857x p) {
  pcf = p;
  usePCF = true;
  return;
}

String HydroMonitorWaterLevelSensor::settingsHtml() {
  String html = F("\
      <tr>\
        <th colspan=\"2\">Water Level Sensor Settings.</th>\
      </tr><tr>\
        <td>Number of samples:</td>\
        <td><input type=\"number\" name=\"waterlevel_samples\" value=\"");
  html += String(settings.Samples);
  html += F("\"></td>\
      </tr><tr>\
        <td>Distance from sensor to bottom of the reservoir:</td>\
        <td><input type=\"number\" name=\"waterlevel_reservoirheight\" value=\"");
  html += String(settings.ReservoirHeight);
  html += F("\"></td>\
      </tr>");
  return html;
}



