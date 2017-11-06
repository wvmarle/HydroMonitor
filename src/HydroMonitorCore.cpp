#include <HydroMonitorCore.h>

HydroMonitorCore::HydroMonitorCore () {

}

/*
 * Validate the string to contain a number: all characters should be a digit, may start with + or -.
 */
bool HydroMonitorCore::isNumeric(String str) {
  uint8_t stringLength = str.length();
  if (stringLength == 0) { // zero-length strings don't contian a number.
    return false;
  }
  bool seenDecimal = false;
  for(uint8_t i = 0; i < stringLength; ++i) { // Iterate over the string.
    if (isDigit(str.charAt(i))) continue;     // It's a digit: continue with the next.
    if (i == 0 && (str.charAt(i) == '-' || str.charAt(i) == '+')) continue; // First character of the string may be + or -.
    if (i > 0 && str.charAt(i) == '.') {      // Decimal point at the second or later character.
      if (seenDecimal) {                      // Only allowed to be seen once, a number has only one decimal point.
        return false;
      }
      seenDecimal = true;
      continue;
    }
    return false;                             // It's an invalid character, whatever it is.
  }
  return true;
}

/*
 * The least squares method: to calculate linear calibration curves.
 *
 * x: array with the values at which the sensor has been calibrated,
 * y: array with the readings at the calibration values,
 * n: the number of (x, y) points,
 * slope: the slope of the resulting line,
 * intercept: the zero-intercept of the line.
 */
void HydroMonitorCore::leastSquares(float *x, uint32_t *y, uint8_t n, float *slope, float *intercept) {
  double xsum = 0, x2sum = 0, ysum = 0, xysum = 0;  // Variables for sums/sigma of xi, yi, xi^2, xiyi.
  if (n < 2) return;          // At least two points needed for a line.
  for (uint8_t i=0; i<n; i++) {
    xsum=xsum+x[i];           // Calculate sigma(xi).
    ysum=ysum+y[i];           // Calculate sigma(yi).
    x2sum=x2sum+x[i]*x[i];    // Calculate sigma(xi^2).
    xysum=xysum+x[i]*y[i];    // Calculate sigma(xi*yi).
  }
  *slope = (n * xysum - xsum * ysum) / (n * x2sum - xsum * xsum);     //calculate slope
  *intercept = (x2sum * ysum - xsum * xysum) / (x2sum * n - xsum * xsum); //calculate intercept
}

/*
 * Read the calibration data of a sensor from the EEPROM.
 *
 * eeprom: the start address of the data,
 * timestamp: array to store the timestamps,
 * value: array to store the calibration values,
 * reading: array to store the readings at the values,
 * enabled: array with bools indicating whether a calibration point is enabled.
 */
void HydroMonitorCore::readCalibration(uint16_t eeprom, uint32_t *timestamp, float *value, uint32_t *reading, bool *enabled) {
  CalibrationData calibrationData;        // The standard struct for calibrationData.
  EEPROM.get(eeprom, calibrationData);    // Read the data.
  uint16_t en = calibrationData.enabled;  // Bits 0-9 are set when the respective point is enabled.
  for (uint8_t i=0; i<DATAPOINTS; i++) {  // Store the data in the arrays.
    timestamp[i] = calibrationData.datapoint[i].time;
    value[i] = calibrationData.datapoint[i].value;
    reading[i] = calibrationData.datapoint[i].reading;
    enabled[i] = (en >> i) & 1;           // Right-shift en to the desired bit, then check whether it's 1 using a bitwise and.
  }
}

/*
 * Store the calibration data of a sensor onto the EEPROM.
 *
 * eeprom: the start address of the data,
 * timestamp: array to store the timestamps,
 * value: array to store the calibration values,
 * reading: array to store the readings at the values,
 * enabled: array with bools indicating whether a calibration point is enabled.
 */
void HydroMonitorCore::writeCalibration(uint16_t eeprom, uint32_t *timestamp, float *value, uint32_t *reading, bool *enabled) {
  CalibrationData calibrationData;        // The standard struct for calibrationData.
  uint16_t en = 0;                        // An int to store which data points are enabled.
  for (uint8_t i=0; i<DATAPOINTS; i++) {  // Copy the data points into the struct.
    calibrationData.datapoint[i].time = timestamp[i];
    calibrationData.datapoint[i].value = value[i];
    calibrationData.datapoint[i].reading = reading[i];
    if (enabled[i]) en |= 1 << i;         // Set bit i if this datapoint is enabled.
  }
  calibrationData.enabled = en;
  EEPROM.put(eeprom, calibrationData);   // Store the data.
  EEPROM.commit();                       // Write it to the permanent storage.
}

/*
 * Create a chunk of HTML listing the calibration data of a sensor.
 *
 * title: shown as heading in the page.
 * action: the url that this form has to call.
 * timestamp, value, reading, enabled: the calibration data.
 *
 * This html contains a form with various buttons.
 * To find out which action, check for the presence of the following parameters:
 * calibrate: take a reading of the sensor for the value given in the input field value.
 * delete=i: delete calibration point i.
 * If none of the above is found: a checkbox was toggled, check which enabled<i>=on parameters are present.
 * 
 */
String HydroMonitorCore::calibrationHtml (char *title, char *action, uint32_t *timestamp, float *value, uint32_t *reading, bool *enabled) {
  String html;
  html += F("\
    <h1>");
  html += title;
  html += F("</h1>\n\
    <form method=\"get\" action=\"");
  html += action;
  html += F("\">\n\
      Value: <input name=\"value\"> <button type=\"submit\" name=\"calibrate\">Take reading</button>\n\
      <table>\n\
        <tr>\n\
          <th>Enable</th>\n\
          <th>Date</th>\n\
          <th>Value</th>\n\
          <th>Reading</th>\n\
          <th></th>\n\
        </tr>");
  for (uint8_t i=0; i<DATAPOINTS; i++) {
    html += F("<tr>\n\
          <td><input type=\"checkbox\" onchange = \"this.form.submit();\" name=\"enable");
    html += i;
    html += F("\"");
    if (enabled[i]) html += F(" checked");
    html += F("></td>\n\
          <td>");
    html += datetime(timestamp[i]);
    html += F("</td>\n\
          <td>");
    html += value[i];
    html += F("</td>\n\
          <td>");
    html += reading[i];
    html += F("</td>\n\
          <td><button type=\"submit\" name=\"delete\" value=\"");
    html += i;
    html += F("\">Delete</button></td>\n\
        </tr>");
  }
  html += F("\n\
      </table>\n\
    </form>\n");
  return html;
}

/*
 * Convert the calibration data into a single JSON structure, and return this.
 *
 * timestamp: array to store the timestamps,
 * value: array to store the calibration values,
 * reading: array to store the readings at the values,
 * enabled: array with bools indicating whether a calibration point is enabled.
 */
String HydroMonitorCore::calibrationData (uint32_t *timestamp, float *value, uint32_t *reading, bool *enabled) {
  String JSON = "{\"calibration\":[";
  for (uint8_t i=0; i<DATAPOINTS; i++) {
    JSON += "{\"timestamp\":";
    JSON += timestamp[i];
    JSON += ",\"value\":";
    JSON += value[i];
    JSON += ",\"reading\":";
    JSON += reading[i];
    JSON += ",\"enabled\":";
    JSON += enabled[i];
    JSON += "}";
    if (i < DATAPOINTS - 1) JSON += ",";  // Add a comma here for all but the last element.
  }
  JSON += "]}";
  return JSON;
}

/*
 * Take the datetime as seconds from epoch, and return a nicely formatted date & time string.
 */
String HydroMonitorCore::datetime(time_t t) {

//  if (t == 0 || t == 4294967295) return "n/a";
  if (t == 0 || t == -1) return "n/a";
  
  // strcat expects stringa, which are null-terminated char arrays. So everywhere we have to add a null
  // character to the array, or it just won't work.
  
  char timestamp[21];
  timestamp[0] = '\0'; 
  char y[5]; sprintf(y, "%04d", year(t));
  y[4] = '\0';
  char mo[3]; sprintf(mo, "%02d", month(t));
  mo[2] = '\0';
  char d[3]; sprintf(d, "%02d", day(t));
  d[2] = '\0';
  char h[3]; sprintf(h, "%02d", hour(t));
  h[2] = '\0';
  char mi[3]; sprintf(mi, "%02d", minute(t));
  mi[2] = '\0';
  char s[3]; sprintf(s, "%02d", second(t));
  s[2] = '\0';

  // Concatenate them to a complete timestamp, then return as String.
  strcat(timestamp, y);
  strcat(timestamp, "/");
  strcat(timestamp, mo);
  strcat(timestamp, "/");
  strcat(timestamp, d);
  strcat(timestamp, ", ");
  strcat(timestamp, h);
  strcat(timestamp, ":");
  strcat(timestamp, mi);
  strcat(timestamp, ":");
  strcat(timestamp, s);
  timestamp[20] = '\0';
  return String(timestamp);
}

String HydroMonitorCore::datetime() {
  if (timeStatus() != timeNotSet) return datetime(now());
  return "n/a";
}
