/**************************************************************************
   Tests the getTemperature and getHumidity functions of the aht20 library
 **************************************************************************/
#include <Wire.h>
#include <AHT20.h>
AHT20 aht20;

void setup()
{
  Serial.begin(115200);
  Serial.println("AHT20 example");

  Wire.begin();
  if (aht20.begin() == false)
  {
    Serial.println("AHT20 not detected. Please check wiring. Freezing.");
    while(true);
  }
}

void loop()
{
  float temperature = aht20.getTemperature();
  float humidity = aht20.getHumidity();

  Serial.print("T: ");
  Serial.print(temperature, 2);
  Serial.print(" C\t H: ");
  Serial.print(humidity, 2);
  Serial.println("% RH");

  delay(2000);
}
