#include <HydroMonitorNetwork.h>

// For whatever reason this has to be included here again, or the defines are not recognised later!
#include <HydroMonitorBoardDefinitions.h>

/*
 * Take care of network connections and building up html pages.
 */

/*
 * The constructor.
 */
HydroMonitorNetwork::HydroMonitorNetwork() {
}

/*
 * Start the network services.
 */
void HydroMonitorNetwork::begin(HydroMonitorMySQL *l, ESP8266WebServer *srv) {
  logging = l;
  logging->writeTesting("HydroMonitorNetwork: configured networking services.");  
  if (NETWORK_EEPROM > 0)
    EEPROM.get(NETWORK_EEPROM, settings);
    
  // Default settings, to be applied if no sensible value for timezone is
  // found in the EEPROM data.
  if (settings.timezone < -12 || settings.timezone > 14) {
    logging->writeTesting("HydroMonitorNetwork: applying default settings.");  
    settings.timezone = 0;
  }
  server = srv;
  return;
}

/*
 * Send html response.
 */
void HydroMonitorNetwork::htmlResponse(String response) {
  server->send(200, "text/html", response);
  return;
}

/*
 * send plain response.
 */
void HydroMonitorNetwork::plainResponse(String response) {
  server->send(200, "text/plain", response);
  yield();
  return;
}

/*
 * Initiate NTP update if connection was established
 */
void HydroMonitorNetwork::ntpUpdateInit() {
  udp.begin(LOCAL_NTP_PORT);
  updateTime = millis();
  startTime = millis();
  if ( WiFi.hostByName(NTP_SERVER_NAME, timeServerIP) ) { //get a random server from the pool
    sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  }
  else udp.stop();
}

/**
 * This function should be called frequently until it returns false, meaning either
 * time has been received, or an ntp timeout.
 *
 * Check if NTP packet was received
 * Re-request every 5 seconds
 * Stop trying after a timeout
 * Returns true if not done updating; false if either timeout or a time has been received.
 */
bool HydroMonitorNetwork::ntpCheck() {

  // Timeout after 30 seconds.
  if (millis() - startTime > 30 * 1000) { 
    logging->writeTesting("HydroMonitorNetwork: NTP timeout.");
    return false;
  }

  // Re-request every 5 seconds.
  if (millis() - updateTime > 5 * 1000) {
    logging->writeTrace("HydroMonitorNetwork: Re-requesting the time from NTP server.");
    udp.stop();
    yield();
    udp.begin(LOCAL_NTP_PORT);
    sendNTPpacket(timeServerIP);
    updateTime = millis();
    return true;
  }

  // Check whether a response has been received.
  if (doNtpUpdateCheck()) {
    udp.stop();
    setTime(epoch + settings.timezone * 60 * 60);
    logging->writeTesting("HydroMonitorNetwork: successfully received time over NTP.");
    return false;
  }

  // No response; no timeout; we're still updating.
  return true;
}

/**
 * Send NTP packet to NTP server
 */
uint32_t HydroMonitorNetwork::sendNTPpacket(IPAddress & address) {

  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);

  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
  yield();
  logging->writeTrace("HydroMonitorNetwork: NTP packet sent.");
}

/**
 * Check if a packet was recieved.
 * Process NTP information if yes
 */
bool HydroMonitorNetwork::doNtpUpdateCheck() {

  yield();
  uint16_t cb = udp.parsePacket();
  if (cb) {

    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:
    uint32_t highWord = word(packetBuffer[40], packetBuffer[41]);
    uint32_t lowWord = word(packetBuffer[42], packetBuffer[43]);

    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    uint32_t secsSince1900 = highWord << 16 | lowWord;

    // now convert NTP time into everyday time:
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const uint32_t seventyYears = 2208988800UL;

    // subtract seventy years:
    epoch = secsSince1900 - seventyYears;
    return (epoch != 0);
  }
  return false;
}

/*
 * Create an html page from the provided body; add html headers and page content headers.
 */
String HydroMonitorNetwork::createHtmlPage(String body, bool refresh) {

  String html = F("<!DOCTYPE html>\n\
  <html><head>\n");
  if (refresh) {
    html += F("   <meta http-equiv=\"refresh\" content=\"");
    html += String(REFRESH);
    html += F("\">\n");
  }
  html += F("   <meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />\n");
  html += F("   <title>City Hydroponics - grow your own, in the city! - HydroMonitor output</title>\n\
</head>\n\
<body>\n\
  <img src=\"https://www.cityhydroponics.hk/tmp/logo_banner.jpg\" alt=\"City Hydroponics\">\n\
  <h1>HydroMonitor - monitor your hydroponic system data</h1>\n\
  <p></p>\n");
  html += body;
  html += F("</body></html>");
  return html;
}

/*
 * The settings as HTML.
 */
String HydroMonitorNetwork::settingsHtml() {
  String html;
  html = F("\
      <tr>\n\
        <th colspan=\"2\">Networking settings.</th>\n\
      </tr><tr>\n\
        <td>Time zone:</td>\n\
        <td><input type=\"text\" name=\"network_timezone\" value=\"");
  html += String(settings.timezone);
  html += F("\"></td>\n\
      </tr>");
  return html;
}

/*
 * Update the settings.
 */
void HydroMonitorNetwork::updateSettings(String keys[], String values[], uint8_t nArgs) {
  for (uint8_t i=0; i<nArgs; i++) {
    if (keys[i] == F("network_timezone")) {
      if (core.isNumeric(values[i])) {
        int8_t val = values[i].toInt();
        if (val >= -12 && val <= 14) settings.timezone = val;
      }
    }
  }
  EEPROM.put(NETWORK_EEPROM, settings);
  EEPROM.commit();
  return;
}
