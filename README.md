<img src="img/sens-cap.png"/>


🤗 Please consider subscribing to my [YouTube channel](https://www.youtube.com/@PricelessToolkit/videos) Your subscription goes a long way in backing my work. if you feel more generous, you can buy me a coffee


[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/U6U2QLAF8)

# Wireless "SOILSENS-V5W" and Wired ["SOILSENS-V5"](https://github.com/PricelessToolkit/SOILSENS-V5)

The SOILSENS-V5W is a reliable wireless capacitive soil moisture sensor that utilizes `ESP-NOW or Wi-Fi` for connectivity. It includes a soil temperature sensor, an air humidity and temperature sensor, and a light intensity sensor. One of its key advantages is that the capacitive sensor electrodes are embedded within the inner layer of the PCB, providing protection. It boasts very low power consumption, a compact design, long-range capabilities in ESP-NOW mode, and configuration changes that can be made without the need for reflashing. Additionally, it supports MQTT Autodiscovery through CapiBridge in ESP-NOW mode or simple Wi-Fi, making it 100% compatible with Home Assistant.

<img src="img/ha_entity.png"/>


## 🛒 Where to buy https://www.PricelessToolkit.com


## 📣 Updates, Bugfixes, and Breaking Changes

> [!NOTE]
>  If you're ready to contribute to the project, your support would be greatly appreciated. Due to time constraints, I may not be able to quickly verify new "features" or completely new "code" functionality, so please create a new code/script in the new folder.

- **14.05.2025** - Added FailSafe, if it can't connect to WIFI or MQTT, it will power off after 15s.
- **21.03.2025** - Added Basic WIFI mode.
- -  **ESP-NOW Mode:**  Device-to-device communication or connection to an ESP-NOW gateway, enabling direct, low-latency, and energy-efficient data transmission.
- - **Fast-WIFI Mode:**  A fast connect method with lower power consumption compared to Basic-WIFI mode. It includes advanced WiFi settings like static IP, MAC address, and channel selection, along with full MQTT integration.
- - **Basic-WIFI Mode:**  A simplified WiFi setup that only requires the SSID and password, providing essential connectivity with MQTT support for straightforward deployments, but it will drain battery faster.
- **10.11.2024** - Increased sensor calibration time to 60 seconds.
- **10.11.2024** - Added RSSI and raw moisture ADC values to MQTT payload.
- **24.07.2024** - Work in progress: "Implementing CRC for ESP-NOW."
- **22.07.2024** - Added the ability to configure the sensor through a Wi-Fi hotspot.
- **21.07.2024** - Combined WiFi and ESP-NOW firmware.
- **20.07.2024** - Implemented MQTT Autodiscovery in "WiFi Mode."
- **15.06.2024** - Published battery percentage.


____________

## Product Specification

- **Microcontroller:**
   - ESP32-C3
- **Sensors:**
   - Soil Temperature: TMP102
   - Air Temperature and Humidity: AHT20
   - Light Sensor: ALS-PT19-315C
   - Soil Moisture: PCB Trace in the internal layer
- **Power:**
   - Uses 1S LiPo Battery "Connector PH2.0"
   - Requires external 1S LiPo charger
- **Buttons:**
   - PRG (Programming)
   - RST (Reset)
   - CAL (Calibration)
   - TRG (Trigger)
- **LED Indicators:**
   - Green LED: Indicates the power-on state
   - Blue LED: Indicates calibration/configuration mode
- **Sensor Reading Interval:**
  - Default: 1 hour
  - Configurable: 5m, 10m, 30m, 1h, and 2h
  - Configuration Method: Adjustable via onboard PCB solder jumpers


### Required

- 1S 250mAh or less LiPo Battery (Connector PH2.0) 35x20x4mm MAX.
   - PricelessToolkit [1S 250mAh PH2.0](https://www.pricelesstoolkit.com/en/products/47-battery-li-po-37v-250mah-ph-20mm-2-pin.html)
   - https://s.click.aliexpress.com/e/_oBG0tjZ
   - https://s.click.aliexpress.com/e/_Dk14vS5
   - https://s.click.aliexpress.com/e/_DE5PfEV
   - Adafruit 150mAh https://www.adafruit.com/product/1317


> [!WARNING] 
> ⚠️ Ensure the battery connector has the correct polarity, as batteries from China sometimes come with random polarity. If it’s incorrect, you can manually swap the wires by gently lifting the plastic retainer on the pin, pulling the wire out, and then reversing the polarity before reinserting it.
> Connecting the battery with reverse polarity will damage the sensor permanently. Double-check the polarity before connecting the battery.


> [!NOTE]
> Lipo Polarity explanation "Adafruit Video" https://youtu.be/ILArrTIMFyM?si=WlTsnVo7XTozZ2FT


- 1S 4.20V LiPo charger.
  - PricelessToolkit [Micro-Charger](https://www.pricelesstoolkit.com/en/products/47-battery-li-po-37v-250mah-ph-20mm-2-pin.html)
  - Adafruit [Micro Lipo ](https://www.adafruit.com/product/1304)

> [!WARNING]  
> ⚠️ Ensure the charger’s maximum charging current does not exceed the battery’s capacity. For example, if the battery is 250mAh, the charger should have a maximum charging current of 250mA or less. Also, check the charger’s polarity, as it can sometimes be reversed.


# Initial Power On and Default Operation
- SOILSENS-V5W supports two modes: `ESP-NOW` and `WiFi` for MQTT-based connectivity.
- Upon powering on, the SOILSENS-V5W operates in ESP-NOW mode by default. This mode does not require any initial configuration and communicates directly with the [Capibridge gateway](https://github.com/PricelessToolkit/CapiBridge). The default Gateway Key is "xy"


| **Feature**                   | **ESP-NOW**                                                                 | **Fast-WiFi**                                      |**Basic-WIFI**            |
|-------------------------------|-----------------------------------------------------------------------------|-----------------------------------------------|-----------------------------------------------|
| **Energy Efficiency**         | Highly energy-efficient                                                     | ~3x Higher power consumption                   | ~10+x Higher power consumption     |
| **Range**                     | Long-Range                                                                  | Short range                                   | Short range                                   |
| **Configuration**             | Does not require configuration                                              | Requires network and MQTT configuration       | Requires network and MQTT configuration     |
| **Broadcasting**              | Sensor data can be read from a third device without connecting to a network | Requires connection to a network              | Requires connection to a network     |
| **Gateway Requirement**       | Requires a gateway [Capibridge](https://github.com/PricelessToolkit/CapiBridge)                                             | Does not require a gateway       | Does not require a gateway             |


## Power consumption
> [!NOTE]
> Measured using ``Power Profiler KIT 2`` Battery Capacity 250mAh, Power Cycle 1h

| **Parameter**                              | **ESP-NOW**               | **Fast-WIFI**               | **Basic-WIFI**            |
|--------------------------------------------|---------------------------|-----------------------------|---------------------------|
| **Active Duration (seconds)**              | 0.55 seconds              | 1.5 seconds                 | 1.5 - 10 seconds          |
| **Current During Active Phase**            | 40 mA                     | 47 mA                       | *                         |
| **Energy Consumption in Active Phase**     | 0.006112 mAh              | 0.02 mAh                    | *                         |
| **Inactive Duration**                      | 0.999847 hours            | 0.99956 hours               | *                         |
| **Energy Consumption in Inactive Phase**   | 0.00007 mAh               | 0.00007 mAh                 | *                         |
| **Total Energy Consumption**               | 0.006182 mAh              | 0.02007 mAh                 | *                         |
| **Total Hours of Operation**               | 40,445 hours              | 12,459 hours                | *                         |
| **Total Days of Operation**                | 1,685 days                | 519 days                    | *                         |

(*) Depends on the network topology





## 1. Configuration

To access the configuration settings, follow these steps:
1. **Initiate Configuration Mode:**
   - Press and hold the calibration button.
   - While holding the calibration button, briefly press the trigger button.
   - Continue holding the calibration button for more than 3 seconds and release it.
   - The blue LED will blink **five times**, indicating the device has entered configuration mode.

> [!NOTE]  
> If you enter the wrong credentials or MAC address and the sensor stays on, preventing you from entering configuration mode again, follow these steps: disconnect the battery, press and hold the calibration button, reconnect the battery, wait for 5 seconds, then release the button. You should now be able to connect to the created hotspot and enter new credentials.


2. **Access Configuration via WiFi:**
   - After entering configuration mode, the sensor creates a WiFi Access Point (AP) named `SOILSENS-V5W`.
   - Connect to this AP using the default password: `password`.
   - Open a web browser and navigate to `http://192.168.4.1` to access the configuration page.


| Mode ESP-NOW | Mode Basic-WiFi | Mode Fast-WiFi |
|--------------|------------|------------|
| Default Node name and key. | Only the key is not required. |
| ![Image 1](img/hotspot1.jpg) | ![Image 2](img/hotspot2.jpg) | ![Image 3](img/hotspot3.jpg) |


3. **Web Configuration:**
   - **Node Name:** Provide a unique name for your sensor. ⚠️ Make sure it does not include any spaces.
   - **Gateway Key:** Enter the key for the [Capibridge gateway.](https://github.com/PricelessToolkit/CapiBridge) for WIFI Mode not required
   - **Mode:** Select the desired mode:
     - **1 (ESP-NOW):** Selects ESP-NOW Protocol "Long-Range".
     - **0 (Fast-WiFi):** Selects Fast-WiFi and MQTT setup.
     - **2 (Basic-WiFi):** Selects Basic-WiFi and MQTT setup "NEW Firmware SoilSens-V5W_3Mod".
   - For Fast-WiFi mode:
     - **WiFi SSID:** Enter your WiFi network name. ⚠️ SSID of the WiFi can’t have letters like å,ä,ö, and so on…
     - **Password:** Enter the WiFi password.
     - **Channel:** Enter the WiFi Channel.
        - You can set a fixed channel on your Wi-Fi router to prevent it from changing automatically
     - **MAC Address:** Specify the 2.4Ghz Access point MAC address "9b:84:5a.....".
     - **Local IP, Gateway, Subnet:** Enter IP address ⚠️ "IP Address outside of DHCP Range".
        - When your router's DHCP assigns IP addresses to connected devices, it usually assigns them within the range of 192.168.0.X up to 192.168.0.254. Think of the subnet as the last part of the IP address, which can range from 1 to 254. Normally, DHCP servers do not assign IPs starting from 1 or up to 254, so you usually have free IP addresses that won't be automatically assigned to any connected device. You need to choose an IP from this free range. You can check the DHCP range in your access point settings. 
     - **MQTT Server IP, Port, Username, Password:** Provide MQTT broker details.
       - Usually, the MQTT server IP is the same as the Home Assistant IP.
       - Usually, the MQTT port is 1883.
   - For Basic-WiFi mode: "NEW Firmware SoilSens-V5W_3Mod"
     - **WiFi SSID:** Enter your WiFi network name. ⚠️ SSID of the WiFi can’t have letters like å,ä,ö, and so on…
     - **Password:** Enter the WiFi password.
     - **MQTT Server IP, Port, Username, Password:** Provide MQTT broker details.


After entering the necessary information, click **Submit** to save the configuration. The sensor will restart with the new settings.

> [!NOTE]
> If it does not restart, unplug and reconnect the battery. If the LED stays on after that, it means the sensor cannot connect because the SSID, Wi-Fi password, MAC address, Wi-Fi channel, or MQTT credentials are incorrect!


> [!IMPORTANT]
> ⚠️ Entering to configuration mode will erase all current settings. Therefore, if you enter configuration mode without saving any changes, all settings will revert to their factory defaults, such as `ESP-NOW` with Key `xy`.


## 2. Calibrating the Soil Moisture Sensor

Accurate soil moisture readings require proper calibration. Follow these steps:

1. Insert the sensor into the flowerpot, ensuring the soil is as dry as your plant can tolerate.
2. Press and hold the calibration button. **Avoid moving the sensor to keep it securely embedded in the soil.**
3. While holding the calibration button, briefly press the trigger button.
4. Continue holding the calibration button for less than 2 seconds, then release it.
5. The blue LED will blink twice, indicating the start of the "dry soil" calibration.
6. The device will complete the minimum moisture level calibration, indicated by three blinks.
7. You now have 60 seconds to water your plant to its maximum acceptable moisture level.
   - I recommend watering the entire soil surface for no more than 20 seconds, allowing it to soak in for 40 seconds.
8. After 60 seconds, the blue LED will blink twice, signaling the start of the "wet soil" (100%) calibration.
9. The device will complete the "wet soil" calibration, indicated by three blinks.

> [!NOTE]
> To ensure accurate readings, avoid touching or moving the sensor after calibration to prevent loosening in the soil. If, over time, sediment accumulates in the soil, the Moisture MIN and MAX values may change.



> [!IMPORTANT]
>  ⚠️ Until the soil moisture sensor calibration is not performed, the sensor reading will be 0.


### WiFi Signal Strength


> [!NOTE]
>  Ensure the Wi-Fi signal strength at the installation point is strong and stable.

| RSSI (dBm)      | Signal Quality | Description                                |
|-----------------|----------------|--------------------------------------------|
| -30 to -50 dBm  | Excellent      | Very strong signal, ideal for stable data transmission |
| -51 to -60 dBm  | Good           | Strong signal, generally reliable for data |
| -61 to -70 dBm  | Fair           | Moderate signal, some minor issues possible, but usable |
| -71 to -80 dBm  | Poor           | Weak signal, may experience some data drop |
| -81 to -90 dBm  | Very Poor      | Very weak signal, frequent data drop likely |
| Below -90 dBm   | Unusable       | No effective connection, likely out of range |


## 3. Wake Up Interval

| Solder Jumpers | interval selection |
|--------------|------------|
| ![Image 1](img/solder-jumper.jpg) |  You can change it by cutting the middle of the 1H Jumper. Then bridge the other jumper with the solder. Available intervals are 5m, 10m, 30m, 1h, and 2h, plus every combination is possible. [Parallel Resistor Calculator](https://www.digikey.fr/en/resources/conversion-calculators/conversion-calculator-parallel-and-series-resistor?utm_adgroup=Resistors&utm_source=google&utm_medium=cpc&utm_campaign=Dynamic%20Search_EN_Product&utm_term=&productid=&utm_content=Resistors&utm_id=go_cmp-9416220174_adg-92817183582_ad-676992152346_dsa-56185200348_dev-c_ext-_prd-_sig-CjwKCAjw2Je1BhAgEiwAp3KY79SzTfH1C_ZeBWNZqrVpeANAvpTVWQipCZKAlBfRqYBXxTiVhI5DOxoCsFoQAvD_BwE&gad_source=1&gclid=CjwKCAjw2Je1BhAgEiwAp3KY79SzTfH1C_ZeBWNZqrVpeANAvpTVWQipCZKAlBfRqYBXxTiVhI5DOxoCsFoQAvD_BwE) |


But if you are bored and want to play with different intervals you can change the resistor value.

| Time Interval | Resistor Value (kΩ) | Time Interval | Resistor Value (kΩ) |
|---------------|------------------------|---------------|------------------------|
| 1 min         | 22.00                  | 10 min        | 57.6 - soldered        |
| 2 min         | 29.349                 | 20 min        | 77.579                 |
| 3 min         | 34.729                 | 30 min        | 93.1 - soldered        |
| 4 min         | 39.097                 | 40 min        | 104.625                |
| 5 min         | 42.2 - soldered        | 50 min        | 115.331                |
| 6 min         | 46.301                 | 1 h           | 124.00 - soldered      |
| 7 min         | 49.392                 | 1h 30min      | 149.398                |
| 8 min         | 52.224                 | 2 h           | 169.00 - soldered      |
| 9 min         | 54.902                 |

> [!NOTE]
> Default time interval is 1 Hour. Reducing the time interval will result in increased energy consumption.

# DIY
<img src="img/sens-kc-pcb.png"/>
This project is open-source, enabling you to assemble SOILSENS-V5W yourself. To make this easier, I've included an 'Interactive HTML BOM File' in the PCB folder, which guides you on where to solder each component and indicates polarity, minimizing the chance of errors. I've invested a lot of time and money into making this project open-source. Your support, whether through buying a ready-made device from my shop www.PricelessToolkit.com subscribing to my YouTube channel, or buying me a coffee, is greatly appreciated and helps fund future projects.

## Schematic
<details>
  <summary>View schematic. Click here</summary>
<img src="PCB/Soilsens-V5W_Schematic.jpg"/>
</details>

## Programming using Arduino IDE

1. - Open the provided code in the Arduino IDE.
2. - Install all neccecery libraries.
```c
      
   #include <esp_now.h>
   #include <WiFi.h>
   #include "driver/adc.h"
   #include "esp_adc_cal.h"
   #include "Wire.h"
   #include <SparkFunTMP102.h>
   #include <ArduinoJson.h>
   #include <AHT20.h>
   #include <Preferences.h>
   #include <WebServer.h>
   #include <PubSubClient.h>
           
```
  3. - Select the appropriate board and port parameters (refer to the provided screenshot for settings).
       
<img src="img/board_config.jpg"/>
       
  4. - Connect 3.3V USBTTL adapter to SOILSENS-V5W

   | Sensor Pin | Adapter Pin |
   |------------|-------------|
   | TX         | RX          |
   | RX         | TX          |
   | 3V3        | 3V3         |
   | GND        | GND         |

5. - Press and hold the **PRG** button on the sensor.
6. - While holding the **PRG** button, connect the USB to TTL adapter to the USB port of your computer.
7. - Click on the **Upload** button to upload the code to the sensor.


## 3D-Printed Case
The ideal material for the case is transparent and UV-resistant. ASA "UV-resistant" or ABS in white is the best choice as it allows light to enter, enabling the luminance sensor to function properly. For indoor use, consider translucent PETG to increase the luminance sensor's range and opt for a 3D-printed case with vent holes to ensure proper function of the air humidity and temperature sensors. For outdoor use, a case without vent holes is recommended to prevent water ingress during heavy! rain.
Glue for case https://s.click.aliexpress.com/e/_DcvGBY9

<img src="img/3dcolor.jpg"/>


## Frequently Asked Questions
### Why is my sensor giving inconsistent readings?
Due to the capacitive sensing principle, readings can vary based on soil moisture, tightness, and insertion depth. Repeated insertions might loosen the soil, affecting subsequent readings.


## Troubleshooting

- Holding the calibration button.
    -  More than 3 seconds configuration mode.
    -  Less than 2 seconds Calibration mode.
- When manually pressing the trigger button, the sensor may occasionally stay on and not power off if the button is not released quickly enough. If this happens, simply press the reset button.
