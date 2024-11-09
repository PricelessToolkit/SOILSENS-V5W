/******************************************************************************
Example2_One-Shot_Temperature_Reading.ino
Example for the TMP102 I2C Temperature Sensor
Alex Wende @ SparkFun Electronics
April 29th 2016
~

This sketch connects to the TMP102 temperature sensor and enables the
one-shot temperature measurement mode using the oneShot() function.
The function returns 0 until the temperature measurement is ready to
read (takes around 25ms). After the measurment is read, the TMP102 is
placed back into sleep mode before the loop is repeated. This can be 
useful to save power or increase the continuous conversion rate from
8Hz up to a maximum of 40Hz.

Resources:
Wire.h (included with Arduino IDE)
SparkFunTMP102.h

Development environment specifics:
Arduino 1.0+

This code is beerware; if you see me (or any other SparkFun employee) at
the local, and you've found our code helpful, please buy us a round!

Distributed as-is; no warranty is given.   
******************************************************************************/

// Include the SparkFun TMP102 library.
// Click here to get the library: http://librarymanager/All#SparkFun_TMP102

#include <Wire.h> // Used to establied serial communication on the I2C bus
#include <SparkFunTMP102.h> // Used to send and recieve specific information from our sensor

// Connections
// VCC = 3.3V
// GND = GND
// SDA = A4
// SCL = A5

TMP102 sensor0;
// Sensor address can be changed with an external jumper to:
// ADD0 - Address
//  VCC - 0x49
//  SDA - 0x4A
//  SCL - 0x4B

void setup() {
  Serial.begin(115200);
  Wire.begin(); //Join I2C Bus
  
  /* The TMP102 uses the default settings with the address 0x48 using Wire.
  
     Optionally, if the address jumpers are modified, or using a different I2C bus,
   these parameters can be changed here. E.g. sensor0.begin(0x49,Wire1)
   
   It will return true on success or false on failure to communicate. */
  if(!sensor0.begin())
  {
    Serial.println("Cannot connect to TMP102.");
    Serial.println("Is the board connected? Is the device ID correct?");
    while(1);
  }
  
  Serial.println("Connected to TMP102!");  
  delay(100);
  sensor0.sleep(); // Put sensor to sleep
}
 
void loop()
{
  sensor0.oneShot(1); // Set One-Shot bit

  while(sensor0.oneShot() == 0); // Wait for conversion to be ready
  Serial.println(sensor0.readTempC());  // Print temperature reading

  sensor0.sleep(); // Return sensor to sleep
  delay(1000);
}
