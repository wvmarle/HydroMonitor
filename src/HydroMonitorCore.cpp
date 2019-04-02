#include <HydroMonitorCore.h>

HydroMonitorCore::HydroMonitorCore () {
}

void HydroMonitorCore::begin (HydroMonitorCore::SensorData *sd) {
  sensorData = sd;
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
  double xsum = 0, x2sum = 0, ysum = 0, xysum = 0;          // Variables for sums/sigma of xi, yi, xi^2, xiyi.
  if (n < 2) return;                                        // At least two points needed for a line.
  for (uint8_t i=0; i<n; i++) {
    xsum=xsum+x[i];                                         // Calculate sigma(xi).
    ysum=ysum+y[i];                                         // Calculate sigma(yi).
    x2sum=x2sum+x[i]*x[i];                                  // Calculate sigma(xi^2).
    xysum=xysum+x[i]*y[i];                                  // Calculate sigma(xi*yi).
  }
  *slope = (n * xysum - xsum * ysum) / (n * x2sum - xsum * xsum); //calculate slope
  *intercept = (x2sum * ysum - xsum * xysum) / (x2sum * n - xsum * xsum); //calculate intercept
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
void HydroMonitorCore::calibrationHtml(ESP8266WebServer *server, char *title, char *action, Datapoint *datapoint) {
  server->sendContent_P(PSTR("\
    <h1>"));
  server->sendContent(title);
  server->sendContent_P(PSTR("</h1>\n\
    <form method=\"get\" action=\""));
  server->sendContent(action);
  server->sendContent_P(PSTR("\">\n\
      Value: <input name=\"value\"> <button type=\"submit\" name=\"calibrate\">Take reading</button>\n\
      <table>\n\
        <tr>\n\
          <th>Enable</th>\n\
          <th>Date</th>\n\
          <th>Value</th>\n\
          <th>Reading</th>\n\
          <th></th>\n\
        </tr>"));
  char buffer[21];
  for (uint8_t i=0; i<DATAPOINTS; i++) {
    server->sendContent_P(PSTR("<tr>\n\
          <td><input type=\"checkbox\" onchange = \"this.form.submit();\" name=\"enable"));
    server->sendContent(itoa(i, buffer, 10));
    server->sendContent_P(PSTR("\""));
    if (datapoint[i].enabled) {
      server->sendContent_P(PSTR(" checked"));
    }
    server->sendContent_P(PSTR("></td>\n\
          <td>"));
    datetime(buffer, datapoint[i].timestamp); 
    server->sendContent(buffer);
    server->sendContent_P(PSTR("</td>\n\
          <td>"));
    sprintf(buffer, "%.2f", datapoint[i].value);
    server->sendContent(buffer);
    server->sendContent_P(PSTR("</td>\n\
          <td>"));
    server->sendContent(itoa(datapoint[i].reading, buffer, 10));
    server->sendContent_P(PSTR("</td>\n\
          <td><button type=\"submit\" name=\"delete\" value=\""));
    server->sendContent(itoa(i, buffer, 10));
    server->sendContent_P(PSTR("\">Delete</button></td>\n\
        </tr>"));
  }
  server->sendContent_P(PSTR("\n\
      </table>\n\
    </form>\n"));
}

/*
 * Convert the calibration data into a single JSON structure, and send this to the web server.
 */
void HydroMonitorCore::calibrationData (ESP8266WebServer *server, Datapoint *calibrationData) {
  server->sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
  server->sendHeader(F("Pragma"), F("no-cache"));
  server->sendHeader(F("Expires"), F("-1"));
  server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  server->send(200, F("application/json"), F(""));
  server->sendContent_P(PSTR("{\"calibration\":["));
  char buffer[100];
  for (uint8_t i=0; i<DATAPOINTS; i++) {
    sprintf_P(buffer, PSTR("{\"timestamp\":%u,\"value\":%.3f,\"reading\":%u,\"enabled\":%u}"), 
              calibrationData[i].timestamp, calibrationData[i].value, calibrationData[i].reading, calibrationData[i].enabled);
    server->sendContent(buffer);
    if (i < DATAPOINTS - 1) {
      server->sendContent_P(PSTR(","));  // Add a comma here for all but the last element.
    }
  }
  server->sendContent_P(PSTR("]}"));
}

/*
 * Take the datetime as seconds from epoch, and return a nicely formatted date & time string.
 * timestamp should have space for at least 20 characters.
 */
void HydroMonitorCore::datetime(char *timestamp, time_t t) {

//  if (t == 0 || t == 4294967295) return "n/a";
  if (t == 0 || t == -1) {
    strcpy_P(timestamp, PSTR("n/a"));
    return;
  }
  // server.sendContent expects stringa, which are null-terminated char arrays. So everywhere we have to add a null
  // character to the array, or it just won't work.
  
  sprintf_P(timestamp, PSTR("%04d/%02d/%02d %02d:%02d:%02d"), year(t), month(t), day(t), hour(t), minute(t), second(t));
  }

void HydroMonitorCore::datetime(char* timestamp) {
  if (timeStatus() != timeNotSet) {
    datetime(timestamp, now());
  }
  else {
    strcpy_P(timestamp, PSTR("n/a"));
  }
}


/*
 * Basic URL encoding/decoding code.
 */
String HydroMonitorCore::urldecode(String str) {
  String decodedString = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == '+') {
      decodedString += ' ';
    }
    else if (c == '%') {
      i++;
      code0 = str.charAt(i);
      i++;
      code1 = str.charAt(i);
      c = (h2int(code0) << 4) | h2int(code1);
      decodedString += c;
    }
    else {
      decodedString += c;
    }
  }
  return decodedString;
}

String HydroMonitorCore::urlencode(String str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  char code2;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encodedString += '+';
    }
    else if (isalnum(c)) {
      encodedString += c;
    }
    else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      code2 = '\0';
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
  }
  return encodedString;
}

unsigned char HydroMonitorCore::h2int(char c)
{
  if (c >= '0' && c <= '9') {
    return ((unsigned char)c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return ((unsigned char)c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F') {
    return ((unsigned char)c - 'A' + 10);
  }
  return (0);
}
