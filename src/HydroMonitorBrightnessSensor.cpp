#include <HydroMonitorBrightnessSensor.h>

#include <Adafruit_TSL2561_U.h>

/*
 * Measure the ambient light brightness.
 * The TSL sensor connects over I2C.
 */
 
HydroMonitorBrightnessSensor::HydroMonitorBrightnessSensor () {
  static int brightness = -1;  
  tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
  brightnessSensorPresent = false;
}

void HydroMonitorBrightnessSensor::begin(Settings s) {

  setSettings(s);
  return;
}

void HydroMonitorBrightnessSensor::setSettings(Settings s) {

  settings = s;
  return;
}

int HydroMonitorBrightnessSensor::readSensor() {

  int brightness = -1;
  if (!brightnessSensorPresent) {

    // Try whether we have one now, it never hurts to check.
    brightnessSensorPresent = tsl.begin();

    // If we just detected a sensor, set it up. Otherwise, we're finished here.
    if (brightnessSensorPresent) {
      tsl.enableAutoRange(true);
      tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS); // medium fast and medium resolution - good for us
    }
  }

  if (brightnessSensorPresent) {
  
    // Get sensor event.
    sensors_event_t event;
    tsl.getEvent(&event);
    brightness = event.light;
    if (brightness == 65536) { // The sensor is either saturated or has been disconnected.
      brightnessSensorPresent = tsl.begin(); // this should return false if not connected.
      if (!brightnessSensorPresent) brightness = -1;
    }
  }

  return brightness;
}

String HydroMonitorBrightnessSensor::settingsHtml() {
  return "";
}


