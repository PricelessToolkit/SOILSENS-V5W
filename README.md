<img src="https://raw.githubusercontent.com/PricelessToolkit/SOILSENS-V5/main/img/banner.jpg"/>

🤗 Please consider subscribing to my [YouTube channel](https://www.youtube.com/@PricelessToolkit/videos) Your subscription goes a long way in backing my work. if you feel more generous, you can buy me a coffee


[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/U6U2QLAF8)

# Wireless "SOILSENS-V5W" and Wired ["SOILSENS-V5"](https://github.com/PricelessToolkit/SOILSENS-V5)

The SOILSENS-V5W is a reliable wireless capacitive soil moisture sensor that utilizes "ESP-NOW or Wi-Fi" for connectivity. It includes a soil temperature sensor, an air humidity and temperature sensor, and a light intensity sensor. One of its key advantages is that the capacitive sensor electrodes are embedded within the inner layer of the PCB, providing protection. It boasts very low power consumption, a compact design, long-range capabilities in ESP-NOW mode, and configuration changes can be made without the need for reflashing.

## 🛒 Can be purchased from http://www.PricelessToolkit.com

## Initial Power On and Default Operation
- SOILSENS-V5W supports two modes: ESP-NOW and WiFi for MQTT-based connectivity.
- Upon powering on, the SOILSENS-V5W operates in ESP-NOW mode by default. This mode does not require any initial configuration and communicates directly with the Capibridge gateway. Default Key is "xy"

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
