#include "GasSensor.h"
#include <Arduino.h>
GasSensor::GasSensor(int pin, float a, float b, float rl, float ro, float vc) 
  : pin(pin), a(a), b(b), rl(rl), ro(ro), vc(vc) {}

float GasSensor::readVoltage() {
  int analogValue = analogRead(pin);
  return analogValue * 2.0 * (vc / 4095.0);  // Adjust for ADC resolution
}

float GasSensor::readResistance() {
  float voltage = readVoltage();
  Serial.printf("Voltage: %f \n", voltage);
  return rl * ((5.0 / voltage) - 1.0);  // 6.0V is assumed here for demonstration
}

float GasSensor::readConcentration() {
  float rs = readResistance();
  Serial.printf("rs: %.3f\n", rs);
  float ratio = rs / ro;
  return a * pow(ratio, b);
}



CO2Sensor::CO2Sensor(int pin, float a, float b, float d, float vc)
  : pin(pin), a(a), b(b), d(d), vc(vc) {}

float CO2Sensor::readVoltage() {
  int analogValue = 2 * analogRead(pin);
  return (analogValue * vc) / 4095.0; // Convert analog value to voltage
}

float CO2Sensor::readConcentration() {
  float v=0.0f;
  for (int i=0;i<100;i++)
  {
  v += readVoltage()*10.0f; // Convert voltage to millivolts
  delay(10);
  }
  return pow(d, (a - b*v ));   // Calculate CO2 concentration in ppm
}

void CO2Sensor::calibrate(float v, float c1) {
  a = v + b; // Simple calibration formula when d = c1
}

void CO2Sensor::setB(float newB) {
  b = newB;
}

void CO2Sensor::setD(float newD) {
  d = newD;
  b = 100 * log(newD); // Update b based on new d (c1)
}
