#include <HydroMonitorAirPressureSensor.h>

/*
 * Measure the atmospheric pressure.
 * The BMP180 sensor connects over I2C.
 */
 
HydroMonitorAirPressureSensor::HydroMonitorAirPressureSensor () {
  pressureSensorPresent = false;
}

void HydroMonitorAirPressureSensor::begin(Settings s) {

  setSettings(s);
  return;
}

void HydroMonitorAirPressureSensor::setSettings(Settings s) {

  settings = s;
  return;
}

double HydroMonitorAirPressureSensor::readSensor() {

  if (!pressureSensorPresent) pressureSensorPresent = pressureSensor.begin();
  if (!pressureSensorPresent) return -1;

  char status = pressureSensor.startTemperature();
  double T, P;
  if (status != 0)
  {
    delay(status);
    status = pressureSensor.getTemperature(T);
    if (status != 0)
    {

      // Start a pressure measurement:
      // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
      // If request is successful, the number of ms to wait is returned.
      // If request is unsuccessful, 0 is returned.
      status = pressureSensor.startPressure(3);
      if (status != 0)
      {
        // Wait for the measurement to complete:
        delay(status);

        // Retrieve the completed pressure measurement:
        // Note that the measurement is stored in the variable P.
        // Note also that the function requires the previous temperature measurement (T).
        // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
        // Function returns 1 if successful, 0 if failure.

        status = pressureSensor.getPressure(P, T);
        if (status != 0) pressure = pressureSensor.sealevel(P, settings.Altitude);
      }
    }
  }
  return pressure;
}

String HydroMonitorAirPressureSensor::settingsHtml() {
  String html;
  html = F("<tr>\
        <th colspan=\"2\">EC Sensor settings.</th>\
      </tr><tr>\
        <td>Number of samples:</td>\
        <td><input type=\"number\" name=\"pressure_altitude\" value=\"");
  html += String(settings.Altitude);
  html += F("\"> meters above sealevel.</td>\
      </tr>");
  return html;
}


