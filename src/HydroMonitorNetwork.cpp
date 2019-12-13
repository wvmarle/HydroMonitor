#include <HydroMonitorNetwork.h>

/*
   Take care of network connections and building up html pages.
*/

/*
   The constructor.
*/
HydroMonitorNetwork::HydroMonitorNetwork() {
}

/*
   Start the network services.
*/
void HydroMonitorNetwork::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l, ESP8266WebServer *srv) {
  sensorData = sd;
  logging = l;
  logging->writeTrace(F("HydroMonitorNetwork: configured networking services."));
  if (NETWORK_EEPROM > 0) {
#ifdef USE_24LC256_EEPROM
    sensorData->EEPROM->get(NETWORK_EEPROM, settings);
#else
    EEPROM.get(NETWORK_EEPROM, settings);
#endif
  }
  server = srv;
}

/*
   Send html response.
*/
void HydroMonitorNetwork::htmlResponse(ESP8266WebServer *server) {

  server->sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
  server->sendHeader(F("Pragma"), F("no-cache"));
  server->sendHeader(F("Expires"), F("-1"));
  server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  // here begin chunked transfer
  server->send(200, F("text/html"), F(""));
}

/*
   send plain response.
*/
void HydroMonitorNetwork::plainResponse(ESP8266WebServer *server) {
  server->send_P(200, PSTR("text/plain"), "");
}

/*
   Initiate NTP update if connection was established
*/
void HydroMonitorNetwork::ntpUpdateInit() {
  udp.begin(LOCAL_NTP_PORT);
  updateTime = millis();
  startTime = millis();
  if (WiFi.hostByName(NTP_SERVER_NAME, timeServerIP) ) {    // Get a random server from the pool.
    sendNTPpacket(timeServerIP);                            // Send an NTP packet to a time server.
  }
  else {
    udp.stop();
  }
}

/**
   This function should be called frequently until it returns false, meaning either
   time has been received, or an ntp timeout.

   Check if NTP packet was received
   Re-request every 5 seconds
   Stop trying after a timeout
   Returns true if not done updating; false if either timeout or a time has been received.
*/
bool HydroMonitorNetwork::ntpCheck() {

  // Timeout after 30 seconds.
  if (millis() - startTime > 30 * 1000ul) {
    logging->writeTrace(F("HydroMonitorNetwork: NTP timeout."));
    return false;
  }

  // Re-request every 5 seconds.
  if (millis() - updateTime > 5 * 1000ul) {
    logging->writeTrace(F("HydroMonitorNetwork: Re-requesting the time from NTP server."));
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
    setTime(epoch);                                         // We use UTC internally, easier overall.
    logging->writeTrace(F("HydroMonitorNetwork: successfully received time over NTP."));
    return false;
  }

  // No response; no timeout; we're still updating.
  return true;
}

/**
   Send NTP packet to NTP server
*/
uint32_t HydroMonitorNetwork::sendNTPpacket(IPAddress & address) {

  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);

  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;                             // LI, Version, Mode
  packetBuffer[1] = 0;                                      // Stratum, or type of clock
  packetBuffer[2] = 6;                                      // Polling Interval
  packetBuffer[3] = 0xEC;                                   // Peer Clock Precision

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
  logging->writeTrace(F("HydroMonitorNetwork: NTP packet sent."));
}

/**
   Check if a packet was recieved.
   Process NTP information if yes
*/
bool HydroMonitorNetwork::doNtpUpdateCheck() {

  yield();
  uint16_t cb = udp.parsePacket();
  if (cb) {

    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE);                // read the packet into the buffer

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
   Create the header part of the main html page.
*/
void HydroMonitorNetwork::htmlPageHeader(ESP8266WebServer *server, bool refresh) {

  char buff[12];
  server->sendContent_P(PSTR("\
<!DOCTYPE html>\n\
<html><head>\n"));
  if (refresh) {
    server->sendContent_P(PSTR("\
    <meta http-equiv=\"refresh\" content=\""));
    server->sendContent(itoa(REFRESH, buff, 10));
    server->sendContent_P(PSTR("\">\n"));
  }
  server->sendContent_P(PSTR("\
    <meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />\n\
    <title>City Hydroponics - grow your own, in the city! - HydroMonitor output</title>\n\
</head>\n\
<body>\n\
  <a href=\"/\">\n\
    <img src=\"https://www.cityhydroponics.hk/tmp/logo_banner.jpg\" alt=\"City Hydroponics\">\n\
  </a>\n\
  <h1>HydroMonitor - monitor your hydroponic system data</h1>\n\
  <h2>"));
  if (strlen(sensorData->systemName) > 0) {
    server->sendContent(sensorData->systemName);
  }
  server->sendContent_P(PSTR("</h2>\n\
  <p></p>\n"));
}

void HydroMonitorNetwork::htmlPageFooter(ESP8266WebServer *server) {
  server->sendContent_P(PSTR("\
</body></html>"));
  server->sendContent(F("")); // this tells web client that transfer is done
  server->client().stop();
}

/*
   The settings as HTML.
*/
void HydroMonitorNetwork::settingsHtml(ESP8266WebServer *server) {
}


/*
   The settings as JSON.
*/
bool HydroMonitorNetwork::settingsJSON(ESP8266WebServer* server) {
  return false; // None.
}

/*
   Update the settings.
*/
void HydroMonitorNetwork::updateSettings(ESP8266WebServer* server) {
}
