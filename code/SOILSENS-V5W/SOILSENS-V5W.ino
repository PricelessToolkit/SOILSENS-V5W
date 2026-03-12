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
#include <math.h>

#define DEFAULT_VREF 1100
#define MQTT_RETAIN true
#define SENSOR_TOPIC "homeassistant/sensor/"

uint8_t receiverAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // ESP-NOW Receiver MAC "BROADCAST"

AHT20 aht20;
TMP102 sensor0;
Preferences preferences;

bool inCalibrationMode = false;
const int calibrationButtonPin = 7;
const int blueLEDPin = 2;
const int soilMoisturePin = 3;

unsigned long BootTime;
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
String wifi_local_ip;
String gateway;
String subnet;
String mac_address;
unsigned int mqtt_port;
String mqtt_username;
String mqtt_password;
unsigned int wifi_channel;
unsigned int config_mode; // 1: ESP-NOW, 0: Fast-WIFI, 2: Basic-WIFI

StaticJsonDocument<256> doc;

typedef struct struct_message {
  char json[256];
} struct_message;

struct_message myData;

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long toggleStartTime = 0;
bool mqttPublished = false;

// Forward declarations
void setupSensorConfig();
void fast_setup_wifi();
void basic_setup_wifi();
void reconnect();
int getAverageSoilMoisture();
int getAverageVoltage();
void calibrateSoilMoistureSensor();
void blinkLED(int times, int interval);
void toggleDonePin();
void FailSafe();
void modeESPNOW();
void mqttautodiscovery();
void publishmqtt();
float readAHT20Temperature();
float readAHT20Humidity();

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  toggleDonePin();
}
#else
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  toggleDonePin();
}
#endif

float readAHT20Temperature() {
  return aht20.getTemperature();
}

float readAHT20Humidity() {
  return aht20.getHumidity();
}

////////////// HOTSPOT CONFIGURATION ////////////

void setupSensorConfig() {
  const char *ssidAP = "SoilSens-V5W";
  const char *passwordAP = "password";

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssidAP, passwordAP);

  Serial.println("Access Point started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  preferences.begin("wifi-config", false);
  preferences.clear();

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
            if (mode === '1') {
                document.getElementById('commonWifiFields').style.display = 'none';
                document.getElementById('wifiAdvancedFields').style.display = 'none';
                document.getElementById('mqttFields').style.display = 'none';
            } else if (mode === '0') {
                document.getElementById('commonWifiFields').style.display = 'block';
                document.getElementById('wifiAdvancedFields').style.display = 'block';
                document.getElementById('mqttFields').style.display = 'block';
            } else if (mode === '2') {
                document.getElementById('commonWifiFields').style.display = 'block';
                document.getElementById('wifiAdvancedFields').style.display = 'none';
                document.getElementById('mqttFields').style.display = 'block';
            }
        }
        window.onload = function() {
            toggleFields();
        };
    </script>
</head>
<body>
    <div class="container">
        <h1>SoilSens-V5W Configuration</h1>
        <form action="/submit" method="post">
            <label for="nodeName">Node Name:</label><br>
            <input type="text" id="nodeName" name="nodeName"><br><br>
            <label for="gatewayKey">Gateway Key:</label><br>
            <input type="text" id="gatewayKey" name="gatewayKey"><br><br>
            <label for="mode">Mode:</label><br>
            <input type="radio" id="mode1" name="mode" value="1" onclick="toggleFields()">
            <label for="mode1">ESP-NOW</label><br>
            <input type="radio" id="mode0" name="mode" value="0" onclick="toggleFields()" checked>
            <label for="mode0">Fast-WIFI</label><br>
            <input type="radio" id="mode2" name="mode" value="2" onclick="toggleFields()">
            <label for="mode2">Basic-WIFI</label><br><br>
            <div id="commonWifiFields">
                <label for="ssid">WiFi SSID:</label><br>
                <input type="text" id="ssid" name="ssid"><br><br>
                <label for="password">WiFi Password:</label><br>
                <input type="password" id="password" name="password"><br><br>
            </div>
            <div id="wifiAdvancedFields">
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
            </div>
            <div id="mqttFields">
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

  server.on("/", [&]() {
    server.send(200, "text/html", htmlForm);
  });

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
      String gatewayArg = server.arg("gateway");
      String subnetArg = server.arg("subnet");
      String macAddress = server.arg("macAddress");
      String wifiChannel = server.arg("wifiChannel");

      preferences.putString("ssid", ssid);
      preferences.putString("password", password);
      preferences.putString("nodeName", nodeName);
      preferences.putString("gatewayKey", gatewayKey);
      preferences.putUInt("mode", mode.toInt());
      preferences.putString("mqttserver", mqttserver);
      preferences.putUInt("mqttport", mqttport.toInt());
      preferences.putString("mqttusername", mqttusername);
      preferences.putString("mqttpassword", mqttpassword);
      preferences.putString("localIP", localIP);
      preferences.putString("gateway", gatewayArg);
      preferences.putString("subnet", subnetArg);
      preferences.putString("macAddress", macAddress);
      preferences.putUInt("wifiChannel", wifiChannel.toInt());
      preferences.end();

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
      html += "<tr><td>Gateway</td><td>" + gatewayArg + "</td></tr>";
      html += "<tr><td>Subnet</td><td>" + subnetArg + "</td></tr>";
      html += "<tr><td>MAC Address</td><td>" + macAddress + "</td></tr>";
      html += "<tr><td>WiFi Channel</td><td>" + wifiChannel + "</td></tr>";
      html += "</table>";
      html += "</body></html>";

      server.send(200, "text/html", html);
      delay(500);
      ESP.restart();
    } else {
      server.send(405, "text/html", "<html><body><h1>Method Not Allowed</h1></body></html>");
      delay(500);
      ESP.restart();
    }
  });

  server.begin();
  Serial.println("Web server started");

  while (true) {
    server.handleClient();
    delay(1);
  }
}

////////////// WiFi Connection Functions ////////////

void fast_setup_wifi() {
  IPAddress localIP, gatewayIP, subnetMask;

  if (!localIP.fromString(wifi_local_ip) ||
      !gatewayIP.fromString(gateway) ||
      !subnetMask.fromString(subnet)) {
    Serial.println("Invalid IP configuration");
    return;
  }

  WiFi.mode(WIFI_STA);

  if (!WiFi.config(localIP, gatewayIP, subnetMask)) {
    Serial.println("Failed to configure static IP");
    return;
  }

  uint8_t bssid[6];
  if (sscanf(mac_address.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
             &bssid[0], &bssid[1], &bssid[2],
             &bssid[3], &bssid[4], &bssid[5]) != 6) {
    Serial.println("Invalid MAC address format");
    return;
  }

  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str(), wifi_channel, bssid, true);

  uint32_t timeout = millis() + 5000;
  while (WiFi.status() != WL_CONNECTED && millis() < timeout) {
    delay(100);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi connected (Fast-WIFI)");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to Fast-WIFI");
  }
}

void basic_setup_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());

  uint32_t timeout = millis() + 5000;
  while (WiFi.status() != WL_CONNECTED && millis() < timeout) {
    delay(100);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi connected (Basic-WIFI)");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to Basic-WIFI");
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    if (client.setBufferSize(1024)) {
      Serial.println("Buffer Size increased to 1024 byte");
    } else {
      Serial.println("Failed to allocate larger buffer");
    }

    String client_id = node_name;

    if (client.connect(client_id.c_str(), mqtt_username.c_str(), mqtt_password.c_str())) {
      Serial.println("MQTT connected");
      Serial.print("Buffersize: ");
      Serial.println(client.getBufferSize());
      mqttautodiscovery();
    } else {
      Serial.print("MQTT failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 1 seconds");
      FailSafe();
      delay(1000);
    }
  }
}

int getAverageSoilMoisture() {
  int total = 0;
  for (int i = 0; i < 5; i++) {
    total += adc1_get_raw(ADC1_CHANNEL_3);
  }
  return total / 5;
}

int getAverageVoltage() {
  int total = 0;
  for (int i = 0; i < 5; i++) {
    total += adc1_get_raw(ADC1_CHANNEL_0);
  }
  return total / 5;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(6, 5);
  BootTime = millis();

  preferences.begin("wifi-config", true);
  wifi_ssid = preferences.getString("ssid", "ssid");
  wifi_password = preferences.getString("password", "password");
  node_name = preferences.getString("nodeName", "SoilSens-V5W");
  gateway_key = preferences.getString("gatewayKey", "xy");
  config_mode = preferences.getUInt("mode", 1);
  mqtt_server = preferences.getString("mqttserver", "192.168.1.5");
  mqtt_port = preferences.getUInt("mqttport", 1883);
  mqtt_username = preferences.getString("mqttusername", "mqtt");
  mqtt_password = preferences.getString("mqttpassword", "Mqttpass");
  wifi_local_ip = preferences.getString("localIP", "192.168.1.100");
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
    while (digitalRead(calibrationButtonPin) == LOW) {
      delay(1);
    }

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

  if (config_mode == 1) {
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
      Serial.println("Error initializing ESP-NOW");
      return;
    }

    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, receiverAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Failed to add peer");
      return;
    }

    esp_now_register_send_cb(OnDataSent);
  } else if (config_mode == 0) {
    fast_setup_wifi();
    client.setServer(mqtt_server.c_str(), mqtt_port);
    reconnect();
  } else if (config_mode == 2) {
    basic_setup_wifi();
    client.setServer(mqtt_server.c_str(), mqtt_port);
    reconnect();
  }

  if (!sensor0.begin()) {
    Serial.println("TMP102 not detected.");
    while (1) {
      delay(1);
    }
  }

  aht20.begin();

  preferences.begin("soilCalib", true);
  drySoilValue = preferences.getUInt("drySoilValue", 0);
  wetSoilValue = preferences.getUInt("wetSoilValue", 0);
  preferences.end();

  int averageVolts = getAverageVoltage();
  int bpercentage = map(averageVolts, 2058, 2730, 0, 100);
  batteryPercentage = constrain(bpercentage, 0, 100);

  light = adc1_get_raw(ADC1_CHANNEL_1);
  soilMoistureRow = getAverageSoilMoisture();

  int mpercent;
  if (drySoilValue == wetSoilValue) {
    mpercent = 0;
  } else {
    mpercent = map(soilMoistureRow, drySoilValue, wetSoilValue, 0, 100);
  }
  soilMpercent = constrain(mpercent, 0, 100);
}

void calibrateSoilMoistureSensor() {
  inCalibrationMode = true;
  preferences.begin("soilCalib", false);
  preferences.clear();

  blinkLED(2, 500);
  delay(1000);

  drySoilValue = getAverageSoilMoisture();
  preferences.putUInt("drySoilValue", drySoilValue);

  blinkLED(3, 500);
  delay(60000);

  blinkLED(2, 500);
  delay(1000);

  wetSoilValue = getAverageSoilMoisture();
  preferences.putUInt("wetSoilValue", wetSoilValue);

  blinkLED(3, 500);
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

void FailSafe() {
  if (inCalibrationMode) return;

  if (config_mode == 0 || config_mode == 2) {
    if (millis() - BootTime >= 15000) {
      toggleDonePin();
    }
  }
}

void modeESPNOW() {
  float airTemp = readAHT20Temperature();
  float airHumidity = readAHT20Humidity();

  doc.clear();
  doc["k"] = gateway_key;
  doc["id"] = node_name;
  doc["b"] = int(batteryPercentage);
  doc["l"] = int(light);
  doc["mo"] = int(soilMpercent);
  doc["t"] = isnan(airTemp) ? 0 : int(airTemp);
  doc["hu"] = isnan(airHumidity) ? 0 : int(airHumidity);
  doc["t2"] = int(sensor0.readTempC());
  doc["rw"] = int(getAverageSoilMoisture());

  size_t jsonSize = serializeJson(doc, myData.json, sizeof(myData.json));
  esp_now_send(receiverAddress, (uint8_t *)&myData, jsonSize + 1);
}

void mqttautodiscovery() {
  client.publish(
    (String(SENSOR_TOPIC) + node_name + "/moisture/config").c_str(),
    (String("{"
      "\"name\":\"Soil Moisture\","
      "\"device_class\":\"Moisture\","
      "\"unit_of_measurement\":\"%\","
      "\"icon\":\"mdi:water-percent\","
      "\"state_topic\":\"") + String(SENSOR_TOPIC) + node_name + "/moisture" + "\","
      "\"unique_id\":\"" + node_name + "_mo" + "\","
      "\"device\":{\"identifiers\":[\"" + node_name + "\"],"
      "\"name\":\"" + node_name + "\","
      "\"mdl\":\"" + node_name + "\","
      "\"mf\":\"PricelessToolkit\"}}").c_str(),
    MQTT_RETAIN);

  client.publish(
    (String(SENSOR_TOPIC) + node_name + "/humidity/config").c_str(),
    (String("{"
      "\"name\":\"Air Humidity\","
      "\"device_class\":\"humidity\","
      "\"unit_of_measurement\":\"%\","
      "\"icon\":\"mdi:water-percent\","
      "\"state_topic\":\"") + String(SENSOR_TOPIC) + node_name + "/humidity" + "\","
      "\"unique_id\":\"" + node_name + "_hu" + "\","
      "\"device\":{\"identifiers\":[\"" + node_name + "\"],"
      "\"name\":\"" + node_name + "\","
      "\"mdl\":\"" + node_name + "\","
      "\"mf\":\"PricelessToolkit\"}}").c_str(),
    MQTT_RETAIN);

  client.publish(
    (String(SENSOR_TOPIC) + node_name + "/soiltmp/config").c_str(),
    (String("{"
      "\"name\":\"Soil TMP\","
      "\"device_class\":\"temperature\","
      "\"unit_of_measurement\":\"°C\","
      "\"icon\":\"mdi:thermometer\","
      "\"state_topic\":\"") + String(SENSOR_TOPIC) + node_name + "/soiltmp" + "\","
      "\"unique_id\":\"" + node_name + "_soiltmp" + "\","
      "\"device\":{\"identifiers\":[\"" + node_name + "\"],"
      "\"name\":\"" + node_name + "\","
      "\"mdl\":\"" + node_name + "\","
      "\"mf\":\"PricelessToolkit\"}}").c_str(),
    MQTT_RETAIN);

  client.publish(
    (String(SENSOR_TOPIC) + node_name + "/airtmp/config").c_str(),
    (String("{"
      "\"name\":\"Air TMP\","
      "\"device_class\":\"temperature\","
      "\"unit_of_measurement\":\"°C\","
      "\"icon\":\"mdi:thermometer\","
      "\"state_topic\":\"") + String(SENSOR_TOPIC) + node_name + "/airtmp" + "\","
      "\"unique_id\":\"" + node_name + "_airtmp" + "\","
      "\"device\":{\"identifiers\":[\"" + node_name + "\"],"
      "\"name\":\"" + node_name + "\","
      "\"mdl\":\"" + node_name + "\","
      "\"mf\":\"PricelessToolkit\"}}").c_str(),
    MQTT_RETAIN);

  client.publish(
    (String(SENSOR_TOPIC) + node_name + "/lx/config").c_str(),
    (String("{"
      "\"name\":\"Lux\","
      "\"device_class\":\"illuminance\","
      "\"unit_of_measurement\":\"lx\","
      "\"icon\":\"mdi:brightness-1\","
      "\"state_topic\":\"") + String(SENSOR_TOPIC) + node_name + "/lx" + "\","
      "\"unique_id\":\"" + node_name + "_lx" + "\","
      "\"device\":{\"identifiers\":[\"" + node_name + "\"],"
      "\"name\":\"" + node_name + "\","
      "\"mdl\":\"" + node_name + "\","
      "\"mf\":\"PricelessToolkit\"}}").c_str(),
    MQTT_RETAIN);

  client.publish(
    (String(SENSOR_TOPIC) + node_name + "/batt/config").c_str(),
    (String("{"
      "\"name\":\"Battery\","
      "\"device_class\":\"battery\","
      "\"unit_of_measurement\":\"%\","
      "\"icon\":\"mdi:battery\","
      "\"entity_category\":\"diagnostic\","
      "\"state_topic\":\"") + String(SENSOR_TOPIC) + node_name + "/batt" + "\","
      "\"unique_id\":\"" + node_name + "_batt" + "\","
      "\"device\":{\"identifiers\":[\"" + node_name + "\"],"
      "\"name\":\"" + node_name + "\","
      "\"mdl\":\"" + node_name + "\","
      "\"mf\":\"PricelessToolkit\"}}").c_str(),
    MQTT_RETAIN);

  client.publish(
    (String(SENSOR_TOPIC) + node_name + "/madcrow/config").c_str(),
    (String("{"
      "\"name\":\"Moisture ADC\","
      "\"icon\":\"mdi:text\","
      "\"state_topic\":\"") + String(SENSOR_TOPIC) + node_name + "/madcrow" + "\","
      "\"unique_id\":\"" + node_name + "_madcrow" + "\","
      "\"force_update\": true,"
      "\"device\":{\"identifiers\":[\"" + node_name + "\"],"
      "\"name\":\"" + node_name + "\","
      "\"mdl\":\"" + node_name + "\","
      "\"mf\":\"PricelessToolkit\"}}").c_str(),
    MQTT_RETAIN);

  client.publish(
    (String(SENSOR_TOPIC) + node_name + "/rssi/config").c_str(),
    (String("{"
      "\"name\":\"RSSI\","
      "\"icon\":\"mdi:signal\","
      "\"unit_of_measurement\":\"dBm\","
      "\"device_class\":\"signal_strength\","
      "\"entity_category\":\"diagnostic\","
      "\"state_topic\":\"") + String(SENSOR_TOPIC) + node_name + "/rssi" + "\","
      "\"unique_id\":\"" + node_name + "_rssi" + "\","
      "\"device\":{\"identifiers\":[\"" + node_name + "\"],"
      "\"name\":\"" + node_name + "\","
      "\"mdl\":\"" + node_name + "\","
      "\"mf\":\"PricelessToolkit\"}}").c_str(),
    MQTT_RETAIN);
}

void publishmqtt() {
  float airTemp = readAHT20Temperature();
  float airHumidity = readAHT20Humidity();

  client.publish((String(SENSOR_TOPIC) + node_name + "/madcrow").c_str(), String(soilMoistureRow).c_str(), MQTT_RETAIN);
  client.publish((String(SENSOR_TOPIC) + node_name + "/batt").c_str(), String(batteryPercentage).c_str(), MQTT_RETAIN);
  client.publish((String(SENSOR_TOPIC) + node_name + "/lx").c_str(), String(light).c_str(), MQTT_RETAIN);
  client.publish((String(SENSOR_TOPIC) + node_name + "/moisture").c_str(), String(soilMpercent).c_str(), MQTT_RETAIN);
  client.publish((String(SENSOR_TOPIC) + node_name + "/airtmp").c_str(), String(airTemp).c_str(), MQTT_RETAIN);
  client.publish((String(SENSOR_TOPIC) + node_name + "/soiltmp").c_str(), String(sensor0.readTempC()).c_str(), MQTT_RETAIN);
  client.publish((String(SENSOR_TOPIC) + node_name + "/humidity").c_str(), String(airHumidity).c_str(), MQTT_RETAIN);
  client.publish((String(SENSOR_TOPIC) + node_name + "/rssi").c_str(), String(WiFi.RSSI()).c_str(), MQTT_RETAIN);
}

void loop() {
  if (config_mode == 1) {
    modeESPNOW();
  } else {
    if (WiFi.status() != WL_CONNECTED) {
      if (config_mode == 0) {
        fast_setup_wifi();
      } else if (config_mode == 2) {
        basic_setup_wifi();
      }
    }

    if (client.connected() && !mqttPublished) {
      publishmqtt();
      mqttPublished = true;
      toggleStartTime = millis();
    } else if (!client.connected()) {
      reconnect();
    }

    client.loop();

    if (millis() - toggleStartTime >= 500 && mqttPublished) {
      toggleDonePin();
    }
  }

  FailSafe();
  client.loop();
}