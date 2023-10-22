#include <Arduino.h>

//  Variables
int PulseSensorPurplePin = 27; // Pulse Sensor PURPLE WIRE connected to ANALOG

int Signal;          // holds the incoming raw data. Signal value can range from 0-1024
int Threshold = 580; // Determine which Signal to "count as a beat", and which to ingore.

// The SetUp Function:
void setup()
{                     // pin that will blink to your heartbeat!
  Serial.begin(9600); // Set's up Serial Communication at certain speed.
}

// The Main Loop Function
void loop()
{

  Signal = analogRead(PulseSensorPurplePin); // Read the PulseSensor's value.

  Serial.println(Signal); // Send the Signal value to Serial Plotter

  delay(10);
}