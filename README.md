# SOILSENS-V5W
Wireless Soil Moisture Sensor

# Quick Start Guide for "SOILSENS-V5W"

Welcome to the setup guide for the SOILSENS-V5W sensor! This device measures soil moisture, soil temperature, air temperature, air humidity, and light intensity. It supports two modes: ESP-NOW for communication with a Capibridge gateway and WiFi for MQTT-based connectivity.

## 1. Initial Power On and Default Operation

Upon powering on, the SOILSENS-V5W operates in ESP-NOW mode by default. This mode does not require any initial configuration and communicates directly with the Capibridge gateway. Default Key is "xy"

## 2. Configuration

To access the configuration settings, follow these steps:

1. **Initiate Configuration Mode:**
   - Press and hold the calibration button.
   - While holding the calibration button, briefly press the trigger button.
   - Continue holding the calibration button for more than 3 seconds and release it.
   - The blue LED will blink **five times**, indicating the device has entered configuration mode.

2. **Access Configuration via WiFi:**
   - After entering configuration mode, the sensor creates a WiFi Access Point (AP) named `SOILSENS-V5W`.
   - Connect to this AP using the default password: `password`.
   - Open a web browser and navigate to `http://192.168.4.1` to access the configuration page.

3. **Web Configuration:**
   - **Node Name:** Enter a unique name for your sensor.
   - **Gateway Key:** Enter the key for the Capibridge gateway.
   - **Mode:** Select the desired mode:
     - **1 (ESP-NOW):** For communication with Capibridge.
     - **0 (WiFi):** For standard WiFi and MQTT setup.
   - For WiFi mode:
     - **WiFi SSID:** Enter your WiFi network name.
     - **Password:** Enter the WiFi password.
     - **MAC Address:** Specify the MAC address if needed.
     - **Local IP, Gateway, Subnet:** Enter IP configuration details.
     - **MQTT Server, Port, Username, Password:** Provide MQTT broker details.

After entering the necessary information, click **Submit** to save the configuration. The sensor will restart with the new settings.

## 3. Calibrating the Soil Moisture Sensor

Accurate soil moisture readings require proper calibration. Follow these steps:
   - Put the sensor in dry soil.
   - Press and hold the calibration button.
   - While holding the calibration button, briefly press the trigger button.
   - Continue holding the calibration button for less than 2 seconds and release it.
   - The blue LED will blink twice, indicating the start of `dry soil` calibration.
   - The device will complete the calibration, indicated by three blinks.
   - Place the sensor in fully wet soil, you have 5 seconds to do so.
   - The blue LED will blink twice, indicating the start of `wet soil` calibration.
   - The device will complete the `wet soil` calibration, indicated by three blinks.


## Troubleshooting

 - Ensure you are following the correct sequence: hold the calibration button, briefly press the trigger button, and continue holding the calibration button.
 -  More than 3 seconds configuration mode.
 -  Less than 3 seconds Calibration mode.
- For connectivity issues in WiFi mode, verify the WiFi SSID, password, MAC, Channel, and MQTT server details.
