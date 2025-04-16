#pragma once


class GasSensor {
  private:
    int pin;
    float a;
    float b;
    float rl;
    float ro;
    float vc;

  public:
    GasSensor(int pin, float a, float b, float rl, float ro, float vc);

    float readVoltage();
    float readResistance();
    float readConcentration();
};



class CO2Sensor {
  private:
    int pin;
    float a;     // Calibration parameter, initially 1500
    float b;     // Constant, initially 600
    float d;     // Constant, initially 400 (e.g., CO2 concentration in ppm)
    float vc;    // Supply voltage (0-5V range)

  public:
    CO2Sensor(int pin, float a = 1500, float b = 600, float d = 400, float vc = 5);

    float readVoltage();    // Reads the analog voltage from the sensor
    float readConcentration(); // Calculates the CO2 concentration in ppm
    void calibrate(float v, float c1); // Adjusts `a` based on the current CO2 concentration

    void setB(float newB);  // Method to update `b` based on a new `c1`
    void setD(float newD);  // Method to update `d` based on a new `c1`
};
