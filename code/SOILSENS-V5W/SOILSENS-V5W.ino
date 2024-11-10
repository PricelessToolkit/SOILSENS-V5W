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

#define DEFAULT_VREF 1100
#define MQTT_RETAIN true
#define SENSOR_TOPIC "homeassistant/sensor/"
#define SWITCH_TOPIC "homeassistant/switch/"
uint8_t receiverAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // ESP-NOW Receiver MAC "BROADCAST"


AHT20 aht20;
TMP102 sensor0;
Preferences preferences;

const int calibrationButtonPin = 7;
const int blueLEDPin = 2;
const int soilMoisturePin = 3;

unsigned long buttonPressTime = 0;
const unsigned long minPressTime = 500;
const unsigned long maxPressTime = 2000;

unsigned int drySoilValue;
unsigned int wetSoilValue;

int light;
int batteryPercentage = 0;
int soilMoistureRow = 0;
int soilMpercent = 0;
String node_name;
String gateway_key;
String wifi_ssid;
String wifi_password;
String mqtt_server;
unsigned int mqtt_port;
String mqtt_username;
String mqtt_password;
String local_ip;
String gateway;
String subnet;
String mac_address;
unsigned int wifi_channel;
unsigned int config_mode; 

StaticJsonDocument<256> doc;

typedef struct struct_message {
    char json[256];
} struct_message;

struct_message myData;

WiFiClient espClient;
PubSubClient client(espClient);




////////////// HOTSPOT ////////////

void setupSensorConfig() {

  const char *ssidAP = "SOILSENS-V5W";
  const char *passwordAP = "password";
    // Set up the access point
  WiFi.softAP(ssidAP, passwordAP);
  Serial.println("Access Point started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());


  preferences.begin("wifi-config", false);
  // Remove a specific key
  preferences.clear();

  /*
  preferences.remove("ssid");
  preferences.remove("password");
  preferences.remove("nodeName");
  preferences.remove("gatewayKey");
  preferences.remove("mode");
  preferences.remove("mqttserver");
  preferences.remove("mqttport");
  preferences.remove("mqttusername");
  preferences.remove("mqttpassword");
  preferences.remove("localIP");
  preferences.remove("gateway");
  preferences.remove("subnet");
  preferences.remove("macAddress");
  preferences.remove("wifiChannel");
  */



  // Web server running on port 80
  WebServer server(80);

const char *htmlForm = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>SoilSensor Setup</title>
    <style>
        body { 
            font-family: Arial, sans-serif;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            margin: 0;
        }
        .container {
            text-align: center;
        }
        .hidden {
            display: none;
        }
    </style>
    <script>
        function toggleFields() {
            var mode = document.querySelector('input[name="mode"]:checked').value;
            var wifiFields = document.getElementById('wifiFields');
            wifiFields.style.display = (mode === '0') ? 'block' : 'none';
        }

        window.onload = function() {
            toggleFields();
        };
    </script>
</head>
<body>
    <div class="container">
        <h1>SOILSENS-V5W Configuration</h1>
        <form action="/submit" method="post">
            <label for="nodeName">Node Name:</label><br>
            <input type="text" id="nodeName" name="nodeName"><br><br>
            <label for="gatewayKey">Gateway Key:</label><br>
            <input type="text" id="gatewayKey" name="gatewayKey"><br><br>
            <label for="mode">Mode:</label><br>
            <input type="radio" id="mode1" name="mode" value="1" onclick="toggleFields()" checked>
            <label for="mode1">1 (ESP-NOW)</label><br>
            <input type="radio" id="mode0" name="mode" value="0" onclick="toggleFields()">
            <label for="mode0">0 (WiFi)</label><br><br>
            <div id="wifiFields" class="hidden">
                <label for="ssid">WiFi SSID:</label><br>
                <input type="text" id="ssid" name="ssid"><br><br>
                <label for="password">Password:</label><br>
                <input type="password" id="password" name="password"><br><br>
                <label for="macAddress">MAC Address:</label><br>
                <input type="text" id="macAddress" name="macAddress"><br><br>
                <label for="wifiChannel">WiFi Channel:</label><br>
                <input type="text" id="wifiChannel" name="wifiChannel"><br><br>
                <label for="localIP">Local IP:</label><br>
                <input type="text" id="localIP" name="localIP"><br><br>
                <label for="gateway">Gateway:</label><br>
                <input type="text" id="gateway" name="gateway"><br><br>
                <label for="subnet">Subnet:</label><br>
                <input type="text" id="subnet" name="subnet"><br><br>
                <label for="mqttserver">MQTT Server:</label><br>
                <input type="text" id="mqttserver" name="mqttserver"><br><br>
                <label for="mqttport">MQTT Port:</label><br>
                <input type="text" id="mqttport" name="mqttport"><br><br>
                <label for="mqttusername">MQTT Username:</label><br>
                <input type="text" id="mqttusername" name="mqttusername"><br><br>
                <label for="mqttpassword">MQTT Password:</label><br>
                <input type="password" id="mqttpassword" name="mqttpassword"><br><br>
            </div>
            <input type="submit" value="Submit">
        </form>
        <br>
        <p>Thank you for your Support!</p>
    </div>
</body>
</html>
)rawliteral";

// Handle root path
server.on("/", [&]() {
    server.send(200, "text/html", htmlForm);
});


  // Handle form submission
  server.on("/submit", HTTP_POST, [&]() {
    if (server.method() == HTTP_POST) {
      String ssid = server.arg("ssid");
      String password = server.arg("password");
      String nodeName = server.arg("nodeName");
      String gatewayKey = server.arg("gatewayKey");
      String mode = server.arg("mode");
      String mqttserver = server.arg("mqttserver");
      String mqttport = server.arg("mqttport");
      String mqttusername = server.arg("mqttusername");
      String mqttpassword = server.arg("mqttpassword");
      String localIP = server.arg("localIP");
      String gateway = server.arg("gateway");
      String subnet = server.arg("subnet");
      String macAddress = server.arg("macAddress");
      String wifiChannel = server.arg("wifiChannel");

      // Save the configuration using Preferences
      // Write the new data
      preferences.putString("ssid", ssid);
      preferences.putString("password", password);
      preferences.putString("nodeName", nodeName);
      preferences.putString("gatewayKey", gatewayKey);
      preferences.putUInt("mode", mode.toInt());
      preferences.putString("mqttserver", mqttserver);
      preferences.putString("mqttport", mqttport);
      preferences.putString("mqttusername", mqttusername);
      preferences.putString("mqttpassword", mqttpassword);
      preferences.putString("localIP", localIP);
      preferences.putString("gateway", gateway);
      preferences.putString("subnet", subnet);
      preferences.putString("macAddress", macAddress);
      preferences.putUInt("wifiChannel", wifiChannel.toInt());
      preferences.end();

      // Build the HTML response with all submitted data in a table
      String html = "<html><body><h1>Configuration Saved</h1>";
      html += "<table border='1' cellpadding='5' cellspacing='0'>";
      html += "<tr><th>Field</th><th>Value</th></tr>";
      html += "<tr><td>SSID</td><td>" + ssid + "</td></tr>";
      html += "<tr><td>Password</td><td>" + password + "</td></tr>";
      html += "<tr><td>Node Name</td><td>" + nodeName + "</td></tr>";
      html += "<tr><td>Gateway Key</td><td>" + gatewayKey + "</td></tr>";
      html += "<tr><td>Mode</td><td>" + mode + "</td></tr>";
      html += "<tr><td>MQTT Server</td><td>" + mqttserver + "</td></tr>";
      html += "<tr><td>MQTT Port</td><td>" + mqttport + "</td></tr>";
      html += "<tr><td>MQTT Username</td><td>" + mqttusername + "</td></tr>";
      html += "<tr><td>MQTT Password</td><td>" + mqttpassword + "</td></tr>";
      html += "<tr><td>Local IP</td><td>" + localIP + "</td></tr>";
      html += "<tr><td>Gateway</td><td>" + gateway + "</td></tr>";
      html += "<tr><td>Subnet</td><td>" + subnet + "</td></tr>";
      html += "<tr><td>MAC Address</td><td>" + macAddress + "</td></tr>";
      html += "<tr><td>WiFi Channel</td><td>" + wifiChannel + "</td></tr>";
      html += "</table>";
      html += "</body></html>";

      // Send the HTML response
      server.send(200, "text/html", html);

      delay(500);
      ESP.restart();
    } else {
      server.send(405, "text/html", "<html><body><h1>Method Not Allowed</h1></body></html>");
      delay(500);
      ESP.restart();
    }
  });


  // Start the web server
  server.begin();
  Serial.println("Web server started");

  while (true) {
    server.handleClient();
  }

}



void setup_wifi() {
  // Convert IP addresses from strings to IPAddress objects
  IPAddress localIP, gatewayIP, subnetMask;
  if (!localIP.fromString(local_ip) ||
      !gatewayIP.fromString(gateway) ||
      !subnetMask.fromString(subnet)) {
    Serial.println("Invalid IP configuration");
    return;
  }

  // Configure static IP
  if (!WiFi.config(localIP, gatewayIP, subnetMask)) {
    Serial.println("Failed to configure static IP");
    return;
  }

  // Set Wi-Fi mode to station
  WiFi.mode(WIFI_STA);

  // Disconnect from any previous Wi-Fi connections
  //WiFi.disconnect(true);

  // Convert MAC address string to uint8_t array
  uint8_t bssid[6];
  if (sscanf(mac_address.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
             &bssid[0], &bssid[1], &bssid[2],
             &bssid[3], &bssid[4], &bssid[5]) != 6) {
    Serial.println("Invalid MAC address format");
    return;
  }

  // Begin Wi-Fi connection with BSSID and channel
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str(), wifi_channel, bssid, true);

  // Wait for connection with a timeout
  uint32_t timeout = millis() + 5000; // 5 seconds timeout
  while (WiFi.status() != WL_CONNECTED && millis() < timeout) {
    delay(100);
    Serial.print(".");
  }

  // Check connection status
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to Wi-Fi");
  }
}



void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Buffer needs to be increased to accomodate the config payloads
    if (client.setBufferSize(1024)) {
      Serial.println("Buffer Size increased to 1024 byte");
    } else {
      Serial.println("Failed to allocate larger buffer");
    }

    String client_id = node_name;

    if (client.connect(client_id.c_str(), mqtt_username.c_str(), mqtt_password.c_str())) {
      Serial.println("MQTT connected");

      Serial.println("Buffersize: " + client.getBufferSize());

      mqttautodiscovery();

    } else {
      Serial.print("MQTT failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 1 seconds");
      delay(1000);
    }
  }
}


int getAverageSoilMoisture() {
    int total = 0;
    for (int i = 0; i < 5; i++) {
        int soilMoisture = adc1_get_raw(ADC1_CHANNEL_3);
        total += soilMoisture;
    }
    int average = total / 5;
    return average;
}


int getAverageVoltage() {
    int total = 0;
    for (int i = 0; i < 5; i++) {
        int adcRow = adc1_get_raw(ADC1_CHANNEL_0);
        total += adcRow;
    }
    int averageVolts = total / 5;
    return averageVolts;
}

void setup() {
    Serial.begin(115200);
    Wire.begin(6, 5);
    // Retrieve the saved data from Preferences
    preferences.begin("wifi-config", true);
    wifi_ssid = preferences.getString("ssid", "ssid");
    wifi_password = preferences.getString("password", "password");
    node_name = preferences.getString("nodeName", "SOILSENS-V5W");
    gateway_key = preferences.getString("gatewayKey", "ab");
    config_mode = preferences.getUInt("mode", 1);
    mqtt_server = preferences.getString("mqttserver", "192.168.1.5");
    mqtt_port = preferences.getUInt("mqttport", 1883);
    mqtt_username = preferences.getString("mqttusername", "mqtt");
    mqtt_password = preferences.getString("mqttpassword", "Mqttpass");
    local_ip = preferences.getString("localIP", "192.168.1.100");
    gateway = preferences.getString("gateway", "192.168.1.1");
    subnet = preferences.getString("subnet", "255.255.255.0");
    mac_address = preferences.getString("macAddress", "94:a6:7e:99:99:99");
    wifi_channel = preferences.getUInt("wifiChannel", 6);
    preferences.end();


    pinMode(calibrationButtonPin, INPUT);
    pinMode(blueLEDPin, OUTPUT);
    digitalWrite(blueLEDPin, HIGH);
    pinMode(4, OUTPUT);
    digitalWrite(4, LOW);

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_11);

    if (digitalRead(calibrationButtonPin) == LOW) {
        buttonPressTime = millis();
        while (digitalRead(calibrationButtonPin) == LOW) {}
        if (digitalRead(calibrationButtonPin) == HIGH) {
            unsigned long pressDuration = millis() - buttonPressTime;
            if (pressDuration > minPressTime && pressDuration < maxPressTime) {
                calibrateSoilMoistureSensor();
            } else if (pressDuration >= maxPressTime) {
                blinkLED(6, 100);
                setupSensorConfig();
            }
        }
    }

    // Check mode ESP-NOW or WIFI
    if (config_mode == 1) {
        WiFi.mode(WIFI_STA);
        if (esp_now_init() != ESP_OK) {
            Serial.println("Error initializing ESP-NOW");
            return;
        }

        esp_now_peer_info_t peerInfo;
        memcpy(peerInfo.peer_addr, receiverAddress, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;

        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            Serial.println("Failed to add peer");
            return;
        }

        esp_now_register_send_cb(OnDataSent);
    } else {
        // Code to execute if config_mode is not equal to 1
        setup_wifi();
        client.setServer(mqtt_server.c_str(), mqtt_port);
        reconnect();
    }


    if (!sensor0.begin()) {
        Serial.println("TMP102 not detected.");
        while (1);
    }

    if (aht20.begin() == false) {
        Serial.println("AHT20 not detected.");
        pinMode(3, OUTPUT);
        digitalWrite(3, LOW);
        while (1);
    }

    preferences.begin("soilCalib", true); // Read-only mode
    drySoilValue = preferences.getUInt("drySoilValue", 0);
    wetSoilValue = preferences.getUInt("wetSoilValue", 0);
    preferences.end();

    // Battery Voltage
    int averageVolts = getAverageVoltage();
    int bpercentage = map(averageVolts, 2058, 2730, 0, 100);
    batteryPercentage = constrain(bpercentage, 0, 100);

    // Light Sensor
    light = adc1_get_raw(ADC1_CHANNEL_1);

    // Soile Moisture
    soilMoistureRow = getAverageSoilMoisture();
    int mpercent = map(soilMoistureRow, drySoilValue, wetSoilValue, 0, 100);
    soilMpercent = constrain(mpercent, 0, 100);


}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    toggleDonePin();
}





void calibrateSoilMoistureSensor() {
    preferences.begin("soilCalib", false);
    preferences.clear(); // Clear all previous values

    blinkLED(2, 500); // Blink LED 2 times to indicate dry soil calibration is starting
    delay(1000);
    drySoilValue = getAverageSoilMoisture(); // Read and calculate the average dry soil value
    preferences.putUInt("drySoilValue", drySoilValue);
    blinkLED(3, 500); // Blink LED 3 times to indicate dry soil calibration is complete

    delay(60000); // Wait 60 seconds for the user to wather the plant.

    blinkLED(2, 500); // Blink LED 2 times to indicate wet soil calibration is starting
    delay(1000);
    wetSoilValue = getAverageSoilMoisture(); // Read and calculate the average wet soil value
    preferences.putUInt("wetSoilValue", wetSoilValue);
    blinkLED(3, 500); // Blink LED 3 times to indicate wet soil calibration is complete

    preferences.end();
    ESP.restart();
}

void blinkLED(int times, int interval) {
    for (int i = 0; i < times; i++) {
        digitalWrite(blueLEDPin, LOW);
        delay(interval);
        digitalWrite(blueLEDPin, HIGH);
        delay(interval);
    }
}

void toggleDonePin() {
    static unsigned long lastToggleTime = 0;
    static bool isPinHigh = false;
    unsigned long currentTime = millis();
    if (isPinHigh && (currentTime - lastToggleTime >= 50)) {
        digitalWrite(4, LOW);
        lastToggleTime = currentTime;
        isPinHigh = false;
    } else if (!isPinHigh && (currentTime - lastToggleTime >= 10)) {
        digitalWrite(4, HIGH);
        lastToggleTime = currentTime;
        isPinHigh = true;
    }
}






////////////////////////// HOTSPOT END ///////////////////////////////////

void modeESPNOW() {

    doc.clear();
    doc["k"] = gateway_key;
    doc["id"] = node_name;
    doc["b"] = int(batteryPercentage);
    doc["l"] = int(light);
    doc["mo"] = int(soilMpercent);
    doc["t"] = int(aht20.getTemperature());
    doc["hu"] = int(aht20.getHumidity());
    doc["t2"] = int(sensor0.readTempC());
    doc["rw"] = int(getAverageSoilMoisture()); // Averaged Soil moisture sensor "ROW ADC Value".
    size_t jsonSize = serializeJson(doc, myData.json, sizeof(myData.json));
    esp_now_send(receiverAddress, (uint8_t *) &myData, jsonSize + 1);
}


// autodiscovery
void mqttautodiscovery() {

    // Moisture
    client.publish(
        (String(SENSOR_TOPIC) + node_name + "/moisture/config").c_str(),
        (String("{"
        "\"name\":\"Soil Moisture\","
        "\"device_class\":\"Moisture\","
        "\"unit_of_measurement\":\"%\","
        "\"icon\":\"mdi:water-percent\","
        "\"state_topic\":\"") + String(SENSOR_TOPIC) + node_name + "/moisture" + "\","
        "\"unique_id\":\"" + node_name + "_mo" +"\","
        "\"device\":{\"identifiers\":[\"" + node_name + "\"],"
        "\"name\":\"" + node_name +"\","
        "\"mdl\":\"" + node_name +
        "\",\"mf\":\"PricelessToolkit\"}}").c_str(),
        MQTT_RETAIN);

    // Humidity
    client.publish(
        (String(SENSOR_TOPIC) + node_name + "/humidity/config").c_str(),
        (String("{"
        "\"name\":\"Air Humidity\","
        "\"device_class\":\"humidity\","
        "\"unit_of_measurement\":\"%\","
        "\"icon\":\"mdi:water-percent\","
        "\"state_topic\":\"") + String(SENSOR_TOPIC) + node_name + "/humidity" + "\","
        "\"unique_id\":\"" + node_name + "_hu" +"\","
        "\"device\":{\"identifiers\":[\"" + node_name + "\"],"
        "\"name\":\"" + node_name +"\","
        "\"mdl\":\"" + node_name +
        "\",\"mf\":\"PricelessToolkit\"}}").c_str(),
        MQTT_RETAIN);

    // Temperature
    client.publish(
        (String(SENSOR_TOPIC) + node_name + "/soiltmp/config").c_str(),
        (String("{"
        "\"name\":\"Soil TMP\","
        "\"device_class\":\"temperature\","
        "\"unit_of_measurement\":\"°C\","
        "\"icon\":\"mdi:thermometer\","
        "\"state_topic\":\"") + String(SENSOR_TOPIC) + node_name + "/soiltmp" + "\","
        "\"unique_id\":\"" + node_name + "_soiltmp" +"\","
        "\"device\":{\"identifiers\":[\"" + node_name + "\"],"
        "\"name\":\"" + node_name +"\","
        "\"mdl\":\"" + node_name +
        "\",\"mf\":\"PricelessToolkit\"}}").c_str(),
        MQTT_RETAIN);

    // Temperature2
    client.publish(
        (String(SENSOR_TOPIC) + node_name + "/airtmp/config").c_str(),
        (String("{"
        "\"name\":\"Air TMP\","
        "\"device_class\":\"temperature\","
        "\"unit_of_measurement\":\"°C\","
        "\"icon\":\"mdi:thermometer\","
        "\"state_topic\":\"") + String(SENSOR_TOPIC) + node_name + "/airtmp" + "\","
        "\"unique_id\":\"" + node_name + "_airtmp" +"\","
        "\"device\":{\"identifiers\":[\"" + node_name + "\"],"
        "\"name\":\"" + node_name +"\","
        "\"mdl\":\"" + node_name +
        "\",\"mf\":\"PricelessToolkit\"}}").c_str(),
        MQTT_RETAIN);

    //Lux
    client.publish(
        (String(SENSOR_TOPIC) + node_name + "/lx/config").c_str(),
        (String("{"
        "\"name\":\"Lux\","
        "\"device_class\":\"illuminance\","
        "\"unit_of_measurement\":\"lx\","
        "\"icon\":\"mdi:brightness-1\","
        "\"state_topic\":\"") + String(SENSOR_TOPIC) + node_name + "/lx" + "\","
        "\"unique_id\":\"" + node_name + "_lx" +"\","
        "\"device\":{\"identifiers\":[\"" + node_name + "\"],"
        "\"name\":\"" + node_name +"\","
        "\"mdl\":\"" + node_name +
        "\",\"mf\":\"PricelessToolkit\"}}").c_str(),
        MQTT_RETAIN);

    //Battery
    client.publish(
        (String(SENSOR_TOPIC) + node_name + "/batt/config").c_str(),
        (String("{"
        "\"name\":\"Battery\","
        "\"device_class\":\"battery\","
        "\"unit_of_measurement\":\"%\","
        "\"icon\":\"mdi:battery\","
        "\"entity_category\":\"diagnostic\","
        "\"state_topic\":\"") + String(SENSOR_TOPIC) + node_name + "/batt" + "\","
        "\"unique_id\":\"" + node_name + "_batt" +"\","
        "\"device\":{\"identifiers\":[\"" + node_name + "\"],"
        "\"name\":\"" + node_name +"\","
        "\"mdl\":\"" + node_name +
        "\",\"mf\":\"PricelessToolkit\"}}").c_str(),
        MQTT_RETAIN);
    
    //ROW Moisture ADC
    client.publish(
      (String(SENSOR_TOPIC) + node_name + "/madcrow/config").c_str(),
      (String("{"
        "\"name\":\"Moisture ADC\","
        "\"icon\":\"mdi:text\","
        "\"state_topic\":\"") + String(SENSOR_TOPIC) + node_name +  "/madcrow" +"\","
        "\"unique_id\":\"" + node_name + "_madcrow" +"\","
        "\"force_update\": true,"
        "\"device\":{\"identifiers\":[\"" + node_name + "\"],"
        "\"name\":\"" + node_name +"\","
        "\"mdl\":\"" + node_name +
        "\",\"mf\":\"PricelessToolkit\"}}").c_str(),
      MQTT_RETAIN);

    //RSSI
    client.publish(
      (String(SENSOR_TOPIC) + node_name + "/rssi/config").c_str(),
      (String("{"
        "\"name\":\"RSSI\","
        "\"icon\":\"mdi:signal\","
        "\"unit_of_measurement\":\"dBm\","
        "\"device_class\":\"signal_strength\","
        "\"entity_category\":\"diagnostic\","
        "\"state_topic\":\"") + String(SENSOR_TOPIC) + node_name +  "/rssi" +"\","
        "\"unique_id\":\"" + node_name + "_rssi" +"\","
        "\"device\":{\"identifiers\":[\"" + node_name + "\"],"
        "\"name\":\"" + node_name +"\","
        "\"mdl\":\"" + node_name +
        "\",\"mf\":\"PricelessToolkit\"}}").c_str(),
      MQTT_RETAIN);




}

void publishmqtt() {
    //Publish each value to its respective MQTT topic
    client.publish((String(SENSOR_TOPIC) + node_name + "/madcrow").c_str(), String(soilMoistureRow).c_str(), MQTT_RETAIN);
    client.publish((String(SENSOR_TOPIC) + node_name + "/batt").c_str(), String(batteryPercentage).c_str(), MQTT_RETAIN);
    client.publish((String(SENSOR_TOPIC) + node_name + "/lx").c_str(), String(light).c_str(), MQTT_RETAIN);
    client.publish((String(SENSOR_TOPIC) + node_name + "/moisture").c_str(), String(soilMpercent).c_str(), MQTT_RETAIN);
    client.publish((String(SENSOR_TOPIC) + node_name + "/airtmp").c_str(), String(aht20.getTemperature()).c_str(), MQTT_RETAIN);
    client.publish((String(SENSOR_TOPIC) + node_name + "/soiltmp").c_str(), String(sensor0.readTempC()).c_str(), MQTT_RETAIN);
    client.publish((String(SENSOR_TOPIC) + node_name + "/humidity").c_str(), String(aht20.getHumidity()).c_str(), MQTT_RETAIN);
    client.publish((String(SENSOR_TOPIC) + node_name + "/rssi").c_str(), String(WiFi.RSSI()).c_str(), MQTT_RETAIN);

}

unsigned long toggleStartTime = 0; // Declare without initialization
bool mqttPublished = false;        // Flag to track if MQTT payload has been published

void loop() {
    if (config_mode == 1) {
        // ESP-NOW mode
        modeESPNOW();
    } else {
        // WiFi and MQTT mode
        if (WiFi.status() != WL_CONNECTED) {
            setup_wifi();
        }
        
        // Check connection only when publishing
        if (client.connected() && !mqttPublished) { 
            //mqttautodiscovery();
            publishmqtt();       
            mqttPublished = true;      // Set flag to true after publishing
            toggleStartTime = millis(); // Record the start time once
        } else if (!client.connected()) {
            reconnect();  // Try reconnecting if client is disconnected
        }

        
        client.loop();   
        
        // Check if the delay period has elapsed for power-off
        if (millis() - toggleStartTime >= 500 && mqttPublished) {
            toggleDonePin(); // Power off the device after 500 ms
        }
    }
    // Call client.loop() unconditionally to maintain MQTT communication
    client.loop();
}

