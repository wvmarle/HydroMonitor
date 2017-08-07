#include <HydroMonitorMaster.h>
#include <EEPROM.h>

ESP8266WebServer server(80);

HydroMonitorMaster::HydroMonitorMaster() {
  useECSensor1 = false;
  useECSensor2 = false;
  useECSensor3 = false;
  useBrightnessSensor = false;
  useGrowlight = false;
  usePressureSensor = false;
  useWaterLevelSensor = false;
  useWaterTempSensor = false;
  useNetwork = false;
  EC1 = EC2 = EC3 = -1;
  brightness = -1;
  watertemp = -1;
  waterlevel = -1;
  pressure = -1;
  lastReadSensor = millis() - (10 * 60  * 1000);
}

/*
 * Initialise some stuff, read the settings.
 */
void HydroMonitorMaster::begin() {

  Serial.print(F("Size (in bytes) of settings: "));
  Serial.println(String(sizeof(settings)));

  // Initialise the EEPROM storage.
  EEPROM.begin(EEPROM_SIZE);
  
  // Start by trying to read the existing settings from flash storage.
  readSettings();

  // Set up the http request handlers.
  server.on("/", std::bind(&HydroMonitorMaster::handleRoot, this));
  server.on("/api", std::bind(&HydroMonitorMaster::handleAPI, this));
  server.onNotFound(std::bind(&HydroMonitorMaster::handleNotFound, this));
  server.begin();

  yield();
  return;
}

/*
 * The main loop: this should be called as frequent as possible, as it's where all the
 * other functions are called from.
 */
 
void HydroMonitorMaster::execute() {
  
  // Check for incoming connections.
  server.handleClient();
  
  // Every 1 minute: read the sensors.
  if (millis() - lastReadSensor > 1 * 60 * 1000) {
    lastReadSensor = millis();
    readSensors();
  }
  
  // Every 10 minutes: send the sensor data to the database.
  if (millis() - lastSendData > 10 * 60 * 1000) {
    lastSendData = millis();
    sendData();
  }
}


void HydroMonitorMaster::enableECSensor1(unsigned char capPos, unsigned char capNeg, unsigned char ECpin) {
  useECSensor1 = true;
//  ECSensor1.begin(settings.ECSensor1, capPos, capNeg, ECpin);
  return;
}

void HydroMonitorMaster::enableECSensor2(unsigned char capPos, unsigned char capNeg, unsigned char ECpin) {
  useECSensor2 = true;
//  ECSensor2.begin(settings.ECSensor2, capPos, capNeg, ECpin);
  return;
}

void HydroMonitorMaster::enableECSensor3(unsigned char capPos, unsigned char capNeg, unsigned char ECpin) {
  useECSensor3 = true;
//  ECSensor3.begin(settings.ECSensor3, capPos, capNeg, ECpin);
  return;
}

void HydroMonitorMaster::enableTSL2561() {
  useBrightnessSensor = true;
//  brightnessSensor.begin(settings.BrightnessSensor, "TSL2561");
//  brightnessSensor.begin(settings.BrightnessSensor);
  return;
}

void HydroMonitorMaster::enableGrowlight(unsigned char growlightPin) {
  useGrowlight = true;
//  growlight.begin(settings.Growlight, growlightPin);
  return;  
}

void HydroMonitorMaster::enableBMP180() {
  usePressureSensor = true;
//  pressureSensor.begin(settings.PressureSensor, "BMP180");
//  pressureSensor.begin(settings.PressureSensor);
  return;
}

void HydroMonitorMaster::enableHCSR04(unsigned char trigPin, unsigned char echoPin, PCF857x *pcf8574) {
  useWaterLevelSensor = true;
//  waterLevelSensor.begin(settings.WaterLevelSensor, trigPin, echoPin, &pcf8574);
  return;
}

void HydroMonitorMaster::enableThermistor(unsigned char sensorPin, unsigned int seriesResistor, unsigned int NTCNominal, unsigned int BCoefficient, unsigned int TNominal, unsigned int ADCMax, Adafruit_ADS1115 *ads) {

  useWaterTempSensor = true;
  waterTempSensor.begin(settings.WaterTempSensor, sensorPin, seriesResistor, NTCNominal, BCoefficient, TNominal, ADCMax, &ads);
  return;
}

void HydroMonitorMaster::enableThermistor(unsigned char sensorPin, unsigned int seriesResistor, unsigned int NTCNominal, unsigned int BCoefficient, unsigned int TNominal, unsigned int ADCMax) {
  useWaterTempSensor = true;
  waterTempSensor.begin(settings.WaterTempSensor, sensorPin, seriesResistor, NTCNominal, BCoefficient, TNominal, ADCMax);
  return;
}

void HydroMonitorMaster::enableDHT22(unsigned char sensorPin) {
  useAirTempSensor = true;
  useHumiditySensor = true;
  airTempSensor.begin(settings.AirTempSensor, "DHT22", sensorPin);
  humiditySensor.begin(settings.HumiditySensor, "DHT22", sensorPin);
  return;
}


void HydroMonitorMaster::enableNetwork() {
  useNetwork = true;
  network.begin(settings.Network);
  return;
}

/**
 * Read all the enabled sensors, and store the results.
 */
void HydroMonitorMaster::readSensors() {

/*
  // Read all the sensors and construct the output data strings.
  if (useBrightnessSensor) brightness = brightnessSensor.readSensor();
  if (useWaterTempSensor) watertemp = waterTempSensor.readSensor();
  if (useECSensor1) EC1 = ECSensor1.readSensor(watertemp);
  if (useECSensor2) EC2 = ECSensor2.readSensor(watertemp);
  if (useECSensor3) EC3 = ECSensor3.readSensor(watertemp);
  if (useWaterLevelSensor) waterlevel = waterLevelSensor.readSensor();
  if (usePressureSensor) pressure = pressureSensor.readSensor();
  if (useGrowlight) growlight.checkGrowlight(brightness);
*/
  currentTime();
  sensorData = {useECSensor1, EC1,
                useECSensor2, EC2,
                useECSensor3, EC3,
                useBrightnessSensor, brightness,
                useWaterTempSensor, watertemp,
                useWaterLevelSensor, waterlevel,
                usePressureSensor, pressure,
                useGrowlight, growlight.getStatus()};
}

void HydroMonitorMaster::sendData() {
  network.sendData(sensorData);
}

void HydroMonitorMaster::handleRoot() {

  server.send(200, "text/html", network.createHtmlPage(network.createSensorHtml(sensorData), true));
}


/*
  HydroMonitorNetwork::Request req = network.WiFiConnection(sensorData);
  
  
  if (req.request == "settings") manageSettings(req.client);
  if (req.request == "calibrate_ec") calibrate_ec(req.client);
  if (req.request == "calibrate_ec1") calibrate_ec1(req.client);
  if (req.request == "calibrate_ec2") calibrate_ec2(req.client);
  if (req.request == "calibrate_ec3") calibrate_ec3(req.client);
  if (req.request == "growlight_on") {
    growlight.on();
    manageSettings(req.client);
  }
  if (req.request == "growlight_off") {
    growlight.off();
    manageSettings(req.client);
  }
  if (req.request == "growlight_auto") {
    growlight.automatic();
    manageSettings(req.client);
  }
*/


/*
 * Incoming /api request received.
 *
 * Call Network to decode the request, and return us a validated set of keys and values, then handle
 * the request accordingly.
 */
void HydroMonitorMaster::handleAPI() {

  server.send(200, "text/plain", "Request for API received.");

  // The arrays keys and values will be filled by the handleAPI function.
//  String keys[10];
//  String values[10];
//  String request = network.handleAPI(server, sensorList, nSensors, keys, values);
  
  //TODO
  
}

void HydroMonitorMaster::handleNotFound() {
  server.send(200, "text/plain", "Unknown request received.");
//  network.handleNotFound(server);
}

/*
 * Take an int, prepend zeroes to make it the required length, and return
 * the result as String.
 */
String HydroMonitorMaster::prependZeros(unsigned int value, unsigned char digits) {
  long reference = 10^digits;
  String result = "";
  while (value < reference) {
    result += "0";
    reference /= 10;
  }
  result += String(value);
  return result;
}

void HydroMonitorMaster::manageSettings(ESP8266WebServer server) {
  String html = F("<table>");
/*
  if (useNetwork) html += brightnessSensor.settingsHtml();
  if (useBrightnessSensor) html += brightnessSensor.settingsHtml();
  if (useWaterTempSensor) html += waterTempSensor.settingsHtml();
  if (useECSensor1) html += ECSensor1.settingsHtml();
  if (useECSensor2) html += ECSensor2.settingsHtml();
  if (useECSensor3) html += ECSensor3.settingsHtml();
  if (useWaterLevelSensor) html += waterLevelSensor.settingsHtml();
  if (usePressureSensor) html += pressureSensor.settingsHtml();
  if (useGrowlight) html += growlight.settingsHtml();
*/
  html += F("</table>");
  network.htmlResponse(server, network.createHtmlPage(html, false));
  return;
}

void HydroMonitorMaster::calibrate_ec(ESP8266WebServer server) {
  manageSettings(server);
}

void HydroMonitorMaster::calibrate_ec1(ESP8266WebServer server) {
  manageSettings(server);
}

void HydroMonitorMaster::calibrate_ec2(ESP8266WebServer server) {
  manageSettings(server);
}

void HydroMonitorMaster::calibrate_ec3(ESP8266WebServer server) {
  manageSettings(server);
}

void HydroMonitorMaster::currentTime() {
  timestamp = F("n/a");
  if (timeStatus() != timeNotSet) {
    // digital clock display of the time

    timestamp = String(String(year()) + "-" 
                + HydroMonitorMaster::prependZeros(month(), 2) + "-" 
                + HydroMonitorMaster::prependZeros(day(), 2) + ", "
                + HydroMonitorMaster::prependZeros(hour(), 2) + ":" 
                + HydroMonitorMaster::prependZeros(minute(), 2) + ":" 
                + HydroMonitorMaster::prependZeros(second(), 2));
  }
  return;
}

void HydroMonitorMaster::readSettings() {
  EEPROM.get(EEPROM_ADDRESS, settings);
  return;
}

void HydroMonitorMaster::writeSettings() {
  EEPROM.put(EEPROM_ADDRESS, settings);
  EEPROM.commit();
  return;
}

