#include <MyoWare.h> // need to download libraries for the following to then support Arduino running code
#include <math.h> //just in case we utilize computations
#include <Wire.h> // Include the Wire library for I2C communication
#include <Adafruit_MCP4725.h> // Include the Adafruit MCP4725 library for DAC control

#define MCP4725_ADDRESS 0x60 // I2C address of the MCP4725 DAC

// Pin assignments
const int sensorPin1 = A0; // Analog input pin for sensor 1
const int sensorPin2 = A1; // Analog input pin for sensor 2

const int nitinolPin = {5,6,7,8,9}; // Pin connected to the nitinol wire control circuit. Right now all pins 
// are controlled with a single integer, but this can be broken into an array by adding []. This will manipulte 
// all fingers at the same time with the same voltage.

// Variables
int desiredHighVoltage = 5;
int desiredLowVoltage = 0; // Desired Voltage 
// (this can change depending on what we get from testing and calibration)

float currentScalingFactor = 0.5; // Calibration factor to convert sensor readings to current
// calibration needed for range of fully relaxed to fully contracted
// calculate middle contraction level when the wire is heating up and cooling down (parameters to be established)

MyoWare myo1(sensorPin1);
MyoWare myo2(sensorPin2);

void setup() {
  pinMode(nitinolPin, OUTPUT);
  // Initialize serial communication for debugging
  Serial.begin(9600);
}

void loop() {
  // Read EMG signals from the MyoWare sensors
  int sensorValue1 = myo1.read();
  int sensorValue2 = myo2.read();

  // Convert sensor readings to acceptable voltage
  float voltage1 = map(sensorValue1, 0, 1023, 0, 500) * currentScalingFactor;
  float voltage2 = map(sensorValue2, 0, 1023, 0, 500) * currentScalingFactor;

  // Calculate average voltage
  float averageVoltage = (voltage1 + voltage2) / 2.0;

 // Control nitinol wire based on desired voltage
  if (averageVoltage < desiredHighVoltage && averageVoltage > desiredLowVoltage) {
    // Set DAC output to half voltage
    dac.setVoltage(2048, false); // 2048 is halfway (4096 max)
  } else if (averageVoltage == desiredHighVoltage) {
    // Set DAC output to max voltage
    dac.setVoltage(4095, false); // 4095 is maximum (12-bit DAC)
  } else {
    // Set DAC output to zero voltage
    dac.setVoltage(0, false);
  }

  // Debugging: print sensor readings and calculated voltage
  Serial.print("Sensor 1: ");
  Serial.print(sensorValue1);
  Serial.print(", Sensor 2: ");
  Serial.print(sensorValue2);
  Serial.print(", Average Voltage: ");
  Serial.println(averageVoltage);

  // You may add some delay here if needed
  delay(100);
} 