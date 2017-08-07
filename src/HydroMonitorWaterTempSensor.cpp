#include <HydroMonitorWaterTempSensor.h>

#include <Average.h>
#include <Adafruit_ADS1015.h>

Adafruit_ADS1115 ads1115;
      
/*
 * Calculate the water temperature from the reading of the thermistor.
 */
HydroMonitorWaterTempSensor::HydroMonitorWaterTempSensor () {
  NTCPresent = false;
  useAds1115 = false;
}

void HydroMonitorWaterTempSensor::begin(Settings s, unsigned char sp, int sr, int nn, int b, float tn, int adc, Adafruit_ADS1115 ads) {

  // This routing if we're connecting the NTC to an external ADS1115 ADC.
  useAds1115 = true;
  ads1115 = ads;
  begin(s, sp, sr, nn, b, tn, adc);
  return;
}

void HydroMonitorWaterTempSensor::begin(Settings s, unsigned char sp, int sr, int nn, int b, float tn, int adc) {

  sensorPin = sp;
  seriesResistor = sr;
  NTCNominal = nn;
  BCoefficient = b;
  TNominal = tn;
  ADCMax = adc;
  
  setSettings(s);
  return;
}

void HydroMonitorWaterTempSensor::setSettings(Settings s) {

  // Retrieve and set the Settings.
  settings = s;
  // Check whether any settings have been set, if not apply defaults.
  if (settings.Samples == 0 or settings.Samples > 100) {
    settings.Samples = 10;
  }
  return;
}

double HydroMonitorWaterTempSensor::readSensor() {

  // read the value from the NTC sensor:
  float reading;
  Average<float> measurements(settings.Samples);

  // Check whether the NTC sensor is present.
  if (useAds1115) reading = ads1115.readADC_SingleEnded(sensorPin);
  else reading = analogRead(sensorPin);
  if (reading < 0.03*ADCMax || reading > 0.97*ADCMax) {
    NTCPresent = false;
    return -1;
  }

  NTCPresent = true;
  for (int i = 0; i < settings.Samples; i++) {
    if (useAds1115) reading = ads1115.readADC_SingleEnded(sensorPin);
    else reading = analogRead(sensorPin);
  
    //Calculate temperature using the Beta Factor equation
    float measurement = 1.0 / (log (seriesResistor / ((ADCMax / reading - 1) * NTCNominal)) / BCoefficient + 1.0 / (TNominal + 273.15)) - 273.15;
    measurements.push(measurement);    
    delay(10);
    yield();
  }
  float t = measurements.mean();
  return t;
}

String HydroMonitorWaterTempSensor::settingsHtml() {
  String html = F("<tr>\
        <th colspan=\"2\">Water Temperature Sensor Settings.</th>\
      </tr><tr>\
        <td>Number of samples:</td>\
        <td><input type=\"number\" name=\"watertemp_samples\" value=\"");
  html += String(settings.Samples);
  html += F("\"></td>\
      </tr>");
  return html;
}

