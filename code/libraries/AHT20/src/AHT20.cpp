/****************************************************************
 * SparkFun_Qwiic_Humidity_AHT20.cpp
 * SparkFun Qwiic Humidity AHT20 Library Source File
 * Priyanka Makin @ SparkFun Electronics
 * Original Creation Date: March 25, 2020
 * https://github.com/sparkfun/SparkFun_Qwiic_Humidity_AHT20_Arduino_Library
 * 
 * Pick up a board here: https://www.sparkfun.com/products/16618
 * 
 * This file implements all the functions of the AHT20 class.
 * Functions in this library can return the humidity and temperature 
 * measured by the sensor. All data is communicated over I2C bus.
 * 
 * Development environment specifics:
 *  IDE: Arduino 1.8.9
 *  Hardware Platform: Arduino Uno
 *  Qwiic Humidity AHT20 Version: 1.0.0
 * 
 * This code is lemonade-ware; if you see me (or any other SparkFun
 * employee) at the local, and you've found our code helpful, please
 * buy us a round!
 * 
 * Distributed as-is; no warranty is given.
 ***************************************************************/

#include <AHT20.h>

/*--------------------------- Device Status ------------------------------*/
AHT20::AHT20(const uint8_t deviceAddress)
{
  _deviceAddress = deviceAddress;
}

bool AHT20::begin()
{
    if (isConnected() == false)
        return false;

    //Wait 40 ms after power-on before reading temp or humidity. Datasheet pg 8
    delay(40);

    //Check if the calibrated bit is set. If not, init the sensor.
    if (isCalibrated() == false)
    {
        //Send 0xBE0800
        initialize();

        //Immediately trigger a measurement. Send 0xAC3300
        triggerMeasurement();

        delay(75); //Wait for measurement to complete

        uint8_t counter = 0;
        while (isBusy())
        {
            delay(1);
            if (counter++ > 100)
                return (false); //Give up after 100ms
        }

        //This calibration sequence is not completely proven. It's not clear how and when the cal bit clears
        //This seems to work but it's not easily testable
        if (isCalibrated() == false)
        {
            return (false);
        }
    }

    //Check that the cal bit has been set
    if (isCalibrated() == false)
        return false;

    //Mark all datums as fresh (not read before)
    sensorQueried.temperature = true;
    sensorQueried.humidity = true;

    return true;
}

//Ping the AHT20's I2C address
//If we get a response, we are correctly communicating with the AHT20
bool AHT20::isConnected()
{
    Wire.beginTransmission(_deviceAddress);
    if (Wire.endTransmission() == 0)
        return true;

    //If IC failed to respond, give it 20ms more for Power On Startup
    //Datasheet pg 7
    delay(20);

    Wire.beginTransmission(_deviceAddress);
    if (Wire.endTransmission() == 0)
        return true;

    return false;
}

/*------------------------ Measurement Helpers ---------------------------*/

uint8_t AHT20::getStatus()
{
    Wire.requestFrom(_deviceAddress, (uint8_t)1);
    if (Wire.available())
        return (Wire.read());
    return (0);
}

//Returns the state of the cal bit in the status byte
bool AHT20::isCalibrated()
{
    return (getStatus() & (1 << 3));
}

//Returns the state of the busy bit in the status byte
bool AHT20::isBusy()
{
    return (getStatus() & (1 << 7));
}

bool AHT20::initialize()
{
    Wire.beginTransmission(_deviceAddress);
    Wire.write(sfe_aht20_reg_initialize);
    Wire.write((uint8_t)0x08);
    Wire.write((uint8_t)0x00);
    if (Wire.endTransmission() == 0)
        return true;
    return false;
}

bool AHT20::triggerMeasurement()
{
    Wire.beginTransmission(_deviceAddress);
    Wire.write(sfe_aht20_reg_measure);
    Wire.write((uint8_t)0x33);
    Wire.write((uint8_t)0x00);
    if (Wire.endTransmission() == 0)
        return true;
    return false;
}

//Loads the
void AHT20::readData()
{
    //Clear previous data
    sensorData.temperature = 0;
    sensorData.humidity = 0;

    if (Wire.requestFrom(_deviceAddress, (uint8_t)6) > 0)
    {
        Wire.read(); // Read and discard state

        uint32_t incoming = 0;
        incoming |= (uint32_t)Wire.read() << (8 * 2);
        incoming |= (uint32_t)Wire.read() << (8 * 1);
        uint8_t midByte = Wire.read();

        incoming |= midByte;
        sensorData.humidity = incoming >> 4;

        sensorData.temperature = (uint32_t)midByte << (8 * 2);
        sensorData.temperature |= (uint32_t)Wire.read() << (8 * 1);
        sensorData.temperature |= (uint32_t)Wire.read() << (8 * 0);

        //Need to get rid of data in bits > 20
        sensorData.temperature = sensorData.temperature & ~(0xFFF00000);

        //Mark data as fresh
        sensorQueried.temperature = false;
        sensorQueried.humidity = false;
    }
}

//Triggers a measurement if one has not been previously started, then returns false
//If measurement has been started, checks to see if complete.
//If not complete, returns false
//If complete, readData(), mark measurement as not started, return true
bool AHT20::available()
{
    if (measurementStarted == false)
    {
        triggerMeasurement();
        measurementStarted = true;
        return (false);
    }

    if (isBusy() == true)
    {
        return (false);
    }

    readData();
    measurementStarted = false;
    return (true);
}

bool AHT20::softReset()
{
    Wire.beginTransmission(_deviceAddress);
    Wire.write(sfe_aht20_reg_reset);
    if (Wire.endTransmission() == 0)
        return true;
    return false;
}

/*------------------------- Make Measurements ----------------------------*/

float AHT20::getTemperature()
{
    if (sensorQueried.temperature == true)
    {
        //We've got old data so trigger new measurement
        triggerMeasurement();

        delay(75); //Wait for measurement to complete

        uint8_t counter = 0;
        while (isBusy())
        {
            delay(1);
            if (counter++ > 100)
                return (false); //Give up after 100ms
        }

        readData();
    }

    //From datasheet pg 8
    float tempCelsius = ((float)sensorData.temperature / 1048576) * 200 - 50;

    //Mark data as old
    sensorQueried.temperature = true;

    return tempCelsius;
}

float AHT20::getHumidity()
{
    if (sensorQueried.humidity == true)
    {
        //We've got old data so trigger new measurement
        triggerMeasurement();

        delay(75); //Wait for measurement to complete

        uint8_t counter = 0;
        while (isBusy())
        {
            delay(1);
            if (counter++ > 100)
                return (false); //Give up after 100ms
        }

        readData();
    }

    //From datasheet pg 8
    float relHumidity = ((float)sensorData.humidity / 1048576) * 100;

    //Mark data as old
    sensorQueried.humidity = true;

    return relHumidity;
}
