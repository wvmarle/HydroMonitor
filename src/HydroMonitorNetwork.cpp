#include <HydroMonitorNetwork.h>

#include <Time.h>
#include <ESP8266WebServer.h>

const char* ntpServerName = "time.nist.gov";

HydroMonitorNetwork::HydroMonitorNetwork() {
}

void HydroMonitorNetwork::begin(Settings s) {
  setSettings(s);
  return;
}

void HydroMonitorNetwork::setSettings(Settings s) {
  settings = s;
  return;
}

/**
 * Initiate connection to the WiFi network
 */
void HydroMonitorNetwork::connectInit() {
  WiFi.mode(WIFI_STA);
  WiFi.hostname(settings.Hostname);
  WiFi.begin(settings.Ssid, settings.WiFiPassword);
  yield();
  return;
}

/**
 * Send the latest data by https to the server, where it will be stored in a database.
 */
void HydroMonitorNetwork::sendData(SensorData sensorData) {

  // Try to make the connection with the server.
  if (!client.connect(settings.MysqlHostname, HTTPSPORT)) return;

  // Prepre the data string to be sent to the server.
  String getdata = "user=" + String(settings.MysqlUsername) + "&password=" + String(settings.MysqlPassword);
  if (sensorData.useECSensor) getdata += "&ec=" + String(sensorData.EC);
  if (sensorData.useBrightnessSensor) getdata += "&brightness=" + String(sensorData.brightness);
  if (sensorData.useWaterTempSensor) getdata += "&watertemp=" + String(sensorData.waterTemp);
  if (sensorData.useWaterLevelSensor) getdata += "&waterlevel=" + String(sensorData.waterLevel);
  if (sensorData.usePressureSensor) getdata += "&pressure=" + String(sensorData.pressure);
  if (sensorData.useGrowlight) getdata += "&growlight=" + String(sensorData.growlight);
  
  String url = settings.MysqlUrlBase + getdata;
  client.print(String("GET ") + settings.MysqlUrlBase + " HTTP/1.1\r\n" +
               "Host: " + settings.MysqlHostname + "\r\n" +
               "User-Agent: HydroMonitor\r\n" +
               "Connection: close\r\n\r\n");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {

      // Headers all received.
      break;
    }
  }
  client.stop();
  return;
}



/*
  String request;

  WiFiClient client = server.available();
  if (!client) return {client, ""};

  // Wait until the client sends some data - or timeout.
  long startTime = millis();
  while(!client.available()){
    if (millis() - startTime > 1000) return {client, ""};
    yield();
  }
 
  // Read the first line of the request
  request = client.readStringUntil('\r');
  client.flush();

  // Request for the settings page.
  if (request.indexOf("/settings") != -1) return {client, "settings"};
  
  // The web API.
  if (request.indexOf("/api") != -1) {
*/  
  

/*
    // get_data: return all available sensor data as JSON string.
    if (request.indexOf("/get_data") != -1) {
      String jsondata = 
        String("{\"hydrodata\": {"
                "\"time\": {"
                  "\"year\": " + String(year()) + ","
                  "\"month\": " + String(month()) + ","
                  "\"day\": " + String(day()) + ","
                  "\"hour\": " + String(hour()) + ","
                  "\"minute\": " + String(minute()) + ","
                  "\"second\": " + String(second()) + "}");
      if (sensorData.useECSensor) jsondata += "\", EC\": " + String(sensorData.EC);
      if (sensorData.useBrightnessSensor) jsondata += "\", brightness\": " + String(sensorData.brightness);
      if (sensorData.useWaterTempSensor) jsondata += "\", waterTemp\": " + String(sensorData.waterTemp);
      if (sensorData.useWaterLevelSensor) jsondata += "\", waterLevel\": " + String(sensorData.waterLevel);
      if (sensorData.usePressureSensor) jsondata += "\", pressure\": " + String(sensorData.pressure);
      if (sensorData.useGrowlight) jsondata += "\", growlight\": " + String(sensorData.growlight);
      jsondata += "}";
      client.println(jsondata);
      return {client, ""};
    }
*/


/*
 * Handle an incoming API request, process and verify all data, and return
 * the command and sets of keys and values.
 */
  
String HydroMonitorNetwork::handleAPI(ESP8266WebServer server, String *sensorList, int nSensors, String *keys, String *values) {
  
  String request = server.arg("request");
  
  if (request == "growlight") {
   
    // growlight: controls the growlight.
    // Expect one and only one argument called status, with allowed values "on", "off" and "auto".
    
    String value = server.arg("status");
    if (value == "on" || value == "off" || value == "auto") {
      keys[0] = "growlight";
      values[0] = value;
      return "growlight";
    }
  }   
  
  else if (request == "history") {
    
    // history: request the logging history.
    // Expect three arguments: fromdate, todate and parameter.
    // fromdate and todate must be a valid date in YYYYMMDD format; parameter must be one of the
    // available sensors.
    
    String fromdate = validateDate(server.arg(fromdate));
    String todate = validateDate(server.arg(todate));
    String parameter = server.arg(parameter);
    if (!hasValue(parameter, sensorList, nSensors)) parameter = "";
    if (fromdate != "" && todate != "" && parameter != "") {
      keys[0] = "fromdate";
      values[0] = fromdate;
      keys[1] = "todate";
      values[1] = todate;
      keys[3] = "parameter";
      values[3] = parameter;
      return "history";
    }
  }
    
  else if (request == "calibration") {
    
    // calibration: get calibration data of a specific sensor.
    // Expect one argument: sensor, which is the name of one the sensor of which the data is requested.
    String sensor = server.arg("sensor");
    if (hasValue(sensor, sensorList, nSensors)) {
      keys[0] = "sensor";
      values[0] = sensor;
      return "calibration";
    }
  }
    
  else if (request == "do_calibrate") {
    
    // do_calibrate: perform calibration of a sensor.
    // Expect two arguments: the sensor and the value to calibrate it at.
    
    String sensor = server.arg("sensor");
    String value = server.arg("value");
    value = validateNumber(value);
    if (sensor != "" && value != "") {
      if (sensor != "" && value != "") {
        keys[0] = "sensor";
        values[0] = sensor;
        keys[1] = "value";
        values[1] = value;
        return "do_calibrate";
      }
    }
  }
    
  else if (request == "set_settings") {
    // TODO
  /*
    // set_settings: set the various settings of a sensor.
    // Expect two or more arguments: the sensor and the key/value pairs of settings.
    if (request.indexOf("/api/set_settings") != -1) {
      String validKeys[2] = {"sensor", "value"};
      if (server.args() >= 2) {
        int nArgs = server.args();
        String keys[nArgs];
        String values[nArgs];
        int nKeys = 0;
        for (int i=0; i<nArgs; i++) {
          String key = server.argName(i);
          String value = server.arg(i);
          
          // Make sure it's a valid key and we don't have it already.
          if (hasValue(key, validKeys, 2) && !hasValue(key, keys, 2)) {
            
            //TODO Validate the data.
            
            // Add key/value combination.
            nKeys++;
            keys[i] = key;
            values[i] = value;
          }
        }
        if (nKeys == nArgs) return {client, "set_settings", keys, values};
      }
    */
  }


  else if (request == "get_settings") {
  
    // get_settings: get settings of a specific sensor.
    // Expect one argument: sensor, which is the name of one the sensor of which the data is requested.
    
    String sensor = server.arg("sensor");
    if (!hasValue(sensor, sensorList, nSensors)) sensor = "";
    
    if (sensor != "") {
      keys[0] = "sensor";
      values[0] = sensor;
      return "get_settings";
    }
  }
    
  // Failed request!
      
  return "";

}

bool HydroMonitorNetwork::hasValue(String val, String *arr, int size){
  int i;
  for (i=0; i < size; i++) {
    if (arr[i] == val)
      return true;
  }
  return false;
}


String HydroMonitorNetwork::validateDate(String date){

  //TODO implement this.
  
  return date;
}


String HydroMonitorNetwork::validateNumber(String number){

  // TODO implement this.
  
}


/*
 *
 */

void HydroMonitorNetwork::htmlResponse(ESP8266WebServer server, String response) {

  server.send(200, "text/html", response);
  return;
}

void HydroMonitorNetwork::plainResponse(ESP8266WebServer server, String response) {

  server.send(200, "text/plain", response);
  yield();
  return;
}

/*
 * Initiate NTP update if connection was established
 */
void HydroMonitorNetwork::ntpUpdateInit() {
  udp.begin(LOCAL_NTP_PORT);
  if ( WiFi.hostByName(ntpServerName, timeServerIP) ) { //get a random server from the pool
    sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  }
  else {
    udp.stop();
//    tNtpUpdate.disable();
    return;
  }

//  tNtpUpdate.set( TASK_SECOND, CONNECT_TIMEOUT, &ntpCheck );
//  tNtpUpdate.enableDelayed();
}

/**
 * Check if NTP packet was received
 * Re-request every 5 seconds
 * Stop trying after a timeout
 */
void HydroMonitorNetwork::ntpCheck() {
//  if ( tNtpUpdate.getRunCounter() % 5 == 0) {
    udp.stop();
    yield();
    udp.begin(LOCAL_NTP_PORT);
    sendNTPpacket(timeServerIP);
    return;
//  }

  if ( doNtpUpdateCheck()) {
//    tNtpUpdate.disable();
    udp.stop();
    setTime(epoch + settings.Timezone * 60 * 60);
  }
//  else {
//    if ( tNtpUpdate.isLastIteration() ) {
//      udp.stop();
//    }
//  }
  
}

/**
 * Send NTP packet to NTP server
 */

unsigned long HydroMonitorNetwork::sendNTPpacket(IPAddress & address) {

  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);

  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
  yield();
}

/**
 * Check if a packet was recieved.
 * Process NTP information if yes
 */
 
bool HydroMonitorNetwork::doNtpUpdateCheck() {

  yield();
  int cb = udp.parsePacket();
  if (cb) {

    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);

    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;

    // now convert NTP time into everyday time:
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;

    // subtract seventy years:
    epoch = secsSince1900 - seventyYears;
    return (epoch != 0);
  }
  return false;
}

/**
 * 
 * 
 */
String HydroMonitorNetwork::createSensorHtml(SensorData sensorData) {

  String body = F("<p>Current time: ");
  body += timestamp;
  body += F("</p>\
  <p></p>\
  <table>\
    <tr>\
      <th>Parameter</th>\
      <th>Value</th>\
    </tr>");
  if (sensorData.useWaterTempSensor) {
    body += F("<tr><td>Water temperature</td><td>");
    
    if (sensorData.waterTemp < 0) body += F("Sensor not connected.</td></tr>");
    else {
      body += String(sensorData.waterTemp);
      body += F(" &deg;C</td></tr>");
    }
  }
  if (sensorData.useBrightnessSensor) {
    body += F("<tr><td>Brightness</td><td>");
    if (sensorData.brightness < 0) body += F("Sensor not connected.</td></tr>");
    else {
      body += String(sensorData.brightness);
      body += F(" lux</td></tr>");
    }
  }
  if (sensorData.useWaterLevelSensor) {
    body += F("<tr><td>Reservoir water level</td><td>");
    if (sensorData.waterLevel < 0) body += F("Sensor not connected.</td></tr>");
    else {
      body += String(sensorData.waterLevel);
      body += F(" cm</td></tr>");
    }
  }
  if (sensorData.useECSensor) {
    body += F("<tr><td>Water conductivity</td><td>");
    if (sensorData.EC < 0) body += F("Sensor not connected.</td></tr>");
    else {
      body += String(sensorData.EC);
      body += F(" mS/cm</td></tr>");
    }
  }
  sensorData.usePressureSensor;
  sensorData.pressure;
  if (sensorData.usePressureSensor) {
    body += F("<tr><td>Atmospheric pressure</td><td>");
    if (sensorData.pressure < 0) body += F("Sensor not connected.</td></tr>");
    else {
      body += String(sensorData.pressure);
      body += F(" mbar</td></tr>");
    }
  }

  if (sensorData.useGrowlight) {
    body += F("<tr><td>Growing light</td><td>");
    if (sensorData.growlight) body += F("On.</td></tr>");
    else body += F("Off</td></tr>");
  }
  body += F("</table>");
  return body;
}


/*
 *
 */
String HydroMonitorNetwork::createHtmlPage(String body, bool refresh) {

  String html = F("<html><head>");
  if (refresh) {
    html += F("<meta http-equiv=\"refresh\" content=\"");
    html += String(REFRESH);
    html += F("\">");
  }
  html += F("<title>City Hydroponics - grow your own, in the city! - HydroMonitor output</title>\
</head>\
<body>\
  <img src=\"https://www.cityhydroponics.hk/tmp/logo_banner.jpg\" alt=\"City Hydroponics\">\
  <h1>HydroMonitor - monitor your hydroponic system data</h1>\
  <p></p>");
  html += body;
  html += F("</body></html>");
  return html;
}

/*
 *
 */
void HydroMonitorNetwork::handleNotFound(ESP8266WebServer server) {
	String message = "File Not Found\n\n";
	message += "URI: ";
	message += server.uri();
	message += "\nMethod: ";
	message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
	message += "\nArguments: ";
	message += server.args();
	message += "\n";

	for ( uint8_t i = 0; i < server.args(); i++ ) {
		message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
	}
	server.send ( 404, "text/plain", message );
	return;
}
