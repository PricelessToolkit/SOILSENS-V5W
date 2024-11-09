/****************************************************************
 * SparkFun_Qwiic_Humidity_AHT20.h
 * SparkFun Qwiic Humidity AHT20 Library Header File
 * Priyanka Makin @ SparkFun Electronics
 * Original Creation Date: March 25, 2020
 * https://github.com/sparkfun/SparkFun_Qwiic_Humidity_AHT20_Arduino_Library
 * 
 * Pickup a board here: https://wwww.sparkfun.com/products/16618
 * 
 * This file prototypes the AHT20 class implemented in SparkFun_Qwiic_Humidity_AHT20.cpp
 * 
 * Development environment specifics:
 *  IDE: Arduino 1.8.9
 *  Hardware Platform: Arduino Uno
 *  Qwiic Humidity AHT20 Version: 1.0.0
 * 
 * This code is lemonade-ware; if you see me (or any other SparkFun employee)
 * at the local, and you've found our code helpful, please buy us a round!
 * 
 ******************************************************************/

#ifndef AHT20_H
#define AHT20_H

#include <Arduino.h>
#include <Wire.h>

#define AHT20_DEFAULT_ADDRESS 0x38

enum registers
{
    sfe_aht20_reg_reset = 0xBA,
    sfe_aht20_reg_initialize = 0xBE,
    sfe_aht20_reg_measure = 0xAC,
};

class AHT20
{
private:
    uint8_t _deviceAddress;
    bool measurementStarted = false;

    struct
    {
        uint32_t humidity;
        uint32_t temperature;
    } sensorData;

    struct
    {
        uint8_t temperature : 1;
        uint8_t humidity : 1;
    } sensorQueried;

public:
    explicit AHT20(const uint8_t deviceAddress = AHT20_DEFAULT_ADDRESS);
    //Device status
    bool begin();                  //Sets the address of the device
    bool isConnected();                   //Checks if the AHT20 is connected to the I2C bus
    bool available();                     //Returns true if new data is available

    //Measurement helper functions
    uint8_t getStatus();       //Returns the status byte
    bool isCalibrated();       //Returns true if the cal bit is set, false otherwise
    bool isBusy();             //Returns true if the busy bit is set, false otherwise
    bool initialize();         //Initialize for taking measurement
    bool triggerMeasurement(); //Trigger the AHT20 to take a measurement
    void readData();           //Read and parse the 6 bytes of data into raw humidity and temp
    bool softReset();          //Restart the sensor system without turning power off and on

    //Make measurements
    float getTemperature(); //Goes through the measurement sequence and returns temperature in degrees celcius
    float getHumidity();    //Goes through the measurement sequence and returns humidity in % RH
};
#endif
