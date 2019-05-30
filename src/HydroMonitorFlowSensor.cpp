#include <HydroMonitorFlowSensor.h>

#ifdef USE_FLOW_SENSOR

static volatile uint32_t pulseCount;

/*
   The constructor.
*/
HydroMonitorFlowSensor::HydroMonitorFlowSensor() {
}

/*
   The ISR which counts the pulses.
*/
void ICACHE_RAM_ATTR HydroMonitorFlowSensor::countPulse() {
  pulseCount++;
}

/*
   Setup the sensor.
*/
void HydroMonitorFlowSensor::begin(HydroMonitorCore::SensorData *sd, HydroMonitorLogging *l) {
  logging = l;
  logging->writeTrace(F("HydroMonitorFlowSensor: configured flow sensor."));
  sensorData = sd;
  sensorData->flow = -1; // Default: no sensor detected yet.

  // The sensor is driven high/low but this prevents the pin from floating when there's no sensor attached.
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  if (FLOW_SENSOR_EEPROM > 0)
    EEPROM.get(FLOW_SENSOR_EEPROM, settings);

  // To read the pulses of this sensor, we set up an interrupt routine.
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), reinterpret_cast<void (*)()>(&countPulse), FALLING);
  pulseCount = 0;
  timeStartCounting = millis();
}

/*
   Read the flow.
*/
void HydroMonitorFlowSensor::readSensor() {
  // We want at least one second between measurements.
  uint32_t timeCounted = millis() - timeStartCounting;
  if (timeCounted > 1000) {
    timeStartCounting = millis();
    uint32_t pulsesCounted = pulseCount;
    pulseCount = 0;
    // If we have counted pulses, calculate the flow in liters per minute.
    // 7055 pulses = 1 litre.
    if (pulsesCounted > 0) {
      sensorData->flow = (pulsesCounted / 7055.0) * (60000.0 / timeCounted);
    }

    // Else if we used to have flow, but not any more, set it to zero.
    // This way we keep the value of -1 (no sensor detected) until at least one pulse
    // has been counted.
    else if (sensorData->flow > 0) {
      sensorData->flow = 0;
    }
  }
}

/*
   The sensor settings as html.
*/
String HydroMonitorFlowSensor::settingsHtml() {
  return "";
}

/*
   The sensor settings as html.
*/
String HydroMonitorFlowSensor::dataHtml() {
  String html = F("<tr>\n\
    <td>Current water flow: </td>\n\
    <td>");
  if (sensorData->flow < 0) {
    html += F("No sensor detected.</td>\n\
  </tr>");
  }
  else if (sensorData->flow == 0) {
    html += F("Flow stopped - possible pump failure.</td>\n\
  </tr>");
  }
  else {
    html += String(sensorData->flow);
    html += F(" liters per minute.</td>\n\
  </tr>");
  }
  return html;
}

/*
   Update the settings.
*/
void HydroMonitorFlowSensor::updateSettings(String keys[], String values[], uint8_t nArgs) {
}
#endif
