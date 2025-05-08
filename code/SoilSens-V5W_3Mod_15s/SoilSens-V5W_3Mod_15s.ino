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
uint8_t receiverAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // ESP-NOW Receiver MAC "BROADCAST"

AHT20 aht20;
TMP102 sensor0;
Preferences preferences;

const int calibrationButtonPin = 7;
const int blueLEDPin = 2;
const int donePin = 4; // Pin controlled by toggleDonePin

unsigned long buttonPressTime = 0;
const unsigned long minPressTime = 500;
const unsigned long maxPressTime = 2000;

// --- Failsafe Timer ---
unsigned long setupStartTime;
const unsigned long failsafeTimeoutDuration = 15000; // 15 seconds
bool failsafeTriggered = false; // Flag to ensure failsafe runs only once

// --- Global Variables (rest) ---
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

StaticJsonDocument<256> doc;      // For ESP-NOW
StaticJsonDocument<512> mqttDoc;  // For MQTT state

typedef struct struct_message {
    char json[256];
} struct_message;

struct_message myData;

WiFiClient espClient;
PubSubClient client(espClient);

// --- Forward Declarations ---
void toggleDonePin();
void blinkLED(int times, int interval);
int getAverageSoilMoisture();
void setupSensorConfig();
void calibrateSoilMoistureSensor();
void mqttautodiscovery();
void publishmqtt();
void reconnect();
void fast_setup_wifi();
void basic_setup_wifi();
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);


////////////// HOTSPOT CONFIGURATION ////////////
void setupSensorConfig() {
    // This function now runs independently of the failsafe timer in loop()
    // because it ends with ESP.restart(), resetting the timer.
    Serial.println("Entering Configuration Mode...");
    digitalWrite(blueLEDPin, LOW); // Turn on LED during config mode

    const char *ssidAP = "SoilSens-V5W";
    const char *passwordAP = "password";
    WiFi.softAP(ssidAP, passwordAP);
    Serial.println("Access Point started");
    Serial.print("IP address: "); Serial.println(WiFi.softAPIP());

    preferences.begin("wifi-config", false);
    preferences.clear();
    preferences.end();

    WebServer server(80);

    // (HTML Form - same as previous versions)
    const char *htmlForm = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>SoilSensor Setup</title>
    <style> /* Styles from previous version */
        body{font-family:Arial,sans-serif;display:flex;justify-content:center;align-items:center;min-height:100vh;margin:0;background-color:#f4f4f4}.container{background-color:#fff;padding:30px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,.1);text-align:center;width:80%;max-width:500px}input[type=text],input[type=password],input[type=number]{width:95%;padding:10px;margin:8px 0;display:inline-block;border:1px solid #ccc;border-radius:4px;box-sizing:border-box}input[type=submit]{width:95%;background-color:#4CAF50;color:#fff;padding:14px 20px;margin:15px 0;border:none;border-radius:4px;cursor:pointer;font-size:16px}input[type=submit]:hover{background-color:#45a049}.hidden{display:none}label{display:block;margin-top:15px;text-align:left;margin-left:2.5%;font-weight:700}.radio-group label{display:inline-block;margin-right:15px;text-align:center;margin-left:0;font-weight:400}.radio-group input[type=radio]{margin-right:5px;vertical-align:middle}.radio-group{margin-bottom:15px;text-align:left;margin-left:2.5%}h1{color:#333;margin-bottom:20px}p{color:#666}#wifiAdvancedFields label,#mqttFields label,#commonWifiFields label{margin-top:10px}
    </style>
    <script> /* Script from previous version */
        function toggleFields(){var e=document.querySelector('input[name="mode"]:checked').value,o=document.getElementById("commonWifiFields"),d=document.getElementById("wifiAdvancedFields"),l=document.getElementById("mqttFields"),t=document.getElementById("gatewayKeyField");o.style.display="none",d.style.display="none",l.style.display="none",t.style.display="none","1"===e?t.style.display="block":"0"===e?(o.style.display="block",d.style.display="block",l.style.display="block"):"2"===e&&(o.style.display="block",l.style.display="block")}window.onload=function(){toggleFields()};
    </script>
</head>
<body>
    <div class="container">
        <h1>Soil Sensor Configuration</h1>
        <form action="/submit" method="post">
            <label for="nodeName">Node Name:</label>
            <input type="text" id="nodeName" name="nodeName" required placeholder="e.g., LivingRoomPlant"><br>
            <div id="gatewayKeyField">
                <label for="gatewayKey">Gateway Key (ESP-NOW only):</label>
                <input type="text" id="gatewayKey" name="gatewayKey" placeholder="e.g., MySecretKey"><br>
            </div>
             <label>Mode:</label>
             <div class="radio-group">
                <input type="radio" id="mode1" name="mode" value="1" onclick="toggleFields()"> <label for="mode1">ESP-NOW</label>
                <input type="radio" id="mode0" name="mode" value="0" onclick="toggleFields()" checked> <label for="mode0">Fast-WIFI</label>
                <input type="radio" id="mode2" name="mode" value="2" onclick="toggleFields()"> <label for="mode2">Basic-WIFI</label>
             </div>
            <div id="commonWifiFields">
                <label for="ssid">WiFi SSID:</label> <input type="text" id="ssid" name="ssid"><br>
                <label for="password">WiFi Password:</label> <input type="password" id="password" name="password"><br>
            </div>
            <div id="wifiAdvancedFields">
                <label for="macAddress">Router MAC (BSSID, Optional):</label> <input type="text" id="macAddress" name="macAddress" placeholder="AA:BB:CC:11:22:33"><br>
                <label for="wifiChannel">WiFi Channel (Optional):</label> <input type="number" id="wifiChannel" name="wifiChannel" placeholder="1-13" min="1" max="13"><br>
                <label for="localIP">Static Local IP:</label> <input type="text" id="localIP" name="localIP" placeholder="192.168.1.150"><br>
                <label for="gateway">Gateway:</label> <input type="text" id="gateway" name="gateway" placeholder="192.168.1.1"><br>
                <label for="subnet">Subnet:</label> <input type="text" id="subnet" name="subnet" placeholder="255.255.255.0"><br>
            </div>
            <div id="mqttFields">
                <label for="mqttserver">MQTT Server IP/Host:</label> <input type="text" id="mqttserver" name="mqttserver"><br>
                <label for="mqttport">MQTT Port:</label> <input type="number" id="mqttport" name="mqttport" value="1883" placeholder="1883"><br>
                <label for="mqttusername">MQTT Username (Optional):</label> <input type="text" id="mqttusername" name="mqttusername"><br>
                <label for="mqttpassword">MQTT Password (Optional):</label> <input type="password" id="mqttpassword" name="mqttpassword"><br>
            </div>
            <input type="submit" value="Save Configuration & Restart">
        </form><br><p>Ensure all required fields for the selected mode are filled.</p>
    </div>
</body>
</html>
)rawliteral";


    server.on("/", HTTP_GET, [&]() { server.send(200, "text/html", htmlForm); });

    server.on("/submit", HTTP_POST, [&]() {
        // (Submit logic - same as previous version, ending with ESP.restart())
        Serial.println("Submit request received.");
        preferences.begin("wifi-config", false);
        preferences.putString("nodeName", server.arg("nodeName"));
        int mode = server.arg("mode").toInt();
        preferences.putUInt("mode", mode);
        if (mode == 1) { /* Save ESP-NOW, clear others */ preferences.putString("gatewayKey", server.arg("gatewayKey")); preferences.remove("ssid"); preferences.remove("password"); preferences.remove("mqttserver"); preferences.remove("mqttport"); preferences.remove("mqttusername"); preferences.remove("mqttpassword"); preferences.remove("localIP"); preferences.remove("gateway"); preferences.remove("subnet"); preferences.remove("macAddress"); preferences.remove("wifiChannel"); }
        else { /* Save WiFi/MQTT */ preferences.putString("ssid", server.arg("ssid")); preferences.putString("password", server.arg("password")); preferences.putString("mqttserver", server.arg("mqttserver")); preferences.putUInt("mqttport", server.arg("mqttport").toInt()); preferences.putString("mqttusername", server.arg("mqttusername")); preferences.putString("mqttpassword", server.arg("mqttpassword")); preferences.remove("gatewayKey"); if (mode == 0) { /* Save Fast WiFi */ preferences.putString("localIP", server.arg("localIP")); preferences.putString("gateway", server.arg("gateway")); preferences.putString("subnet", server.arg("subnet")); preferences.putString("macAddress", server.arg("macAddress")); preferences.putUInt("wifiChannel", server.arg("wifiChannel").toInt()); } else { /* Clear Fast WiFi for Basic*/ preferences.remove("localIP"); preferences.remove("gateway"); preferences.remove("subnet"); preferences.remove("macAddress"); preferences.remove("wifiChannel"); } }
        preferences.end();
        Serial.println("Preferences saved.");
        String html = "<html><head><title>Config Saved</title><style>table,th,td{border:1px solid #000;border-collapse:collapse;padding:5px}</style></head><body><h1>Configuration Saved</h1><table><tr><th>Field</th><th>Value</th></tr>";
        html+="<tr><td>Node Name</td><td>"+server.arg("nodeName")+"</td></tr>"; html+="<tr><td>Mode</td><td>"+server.arg("mode")+"</td></tr>"; if(mode==1){html+="<tr><td>Gateway Key</td><td>"+server.arg("gatewayKey")+"</td></tr>";}else{html+="<tr><td>SSID</td><td>"+server.arg("ssid")+"</td></tr>";html+="<tr><td>Password</td><td>***</td></tr>";html+="<tr><td>MQTT Server</td><td>"+server.arg("mqttserver")+"</td></tr>";html+="<tr><td>MQTT Port</td><td>"+server.arg("mqttport")+"</td></tr>";html+="<tr><td>MQTT Username</td><td>"+server.arg("mqttusername")+"</td></tr>";html+="<tr><td>MQTT Password</td><td>***</td></tr>";if(mode==0){html+="<tr><td>Local IP</td><td>"+server.arg("localIP")+"</td></tr>";html+="<tr><td>Gateway</td><td>"+server.arg("gateway")+"</td></tr>";html+="<tr><td>Subnet</td><td>"+server.arg("subnet")+"</td></tr>";html+="<tr><td>MAC Address</td><td>"+server.arg("macAddress")+"</td></tr>";html+="<tr><td>WiFi Channel</td><td>"+server.arg("wifiChannel")+"</td></tr>";}} html+="</table><p>Device will restart shortly.</p></body></html>";
        server.send(200, "text/html", html);
        Serial.println("Restarting device...");
        delay(2000);
        ESP.restart();
    });

    server.begin();
    Serial.println("Web server started for configuration.");
    while (true) {
        server.handleClient();
        delay(1);
    }
}

////////////// WiFi Connection Functions ////////////
// (fast_setup_wifi and basic_setup_wifi - same as previous version)
void fast_setup_wifi(){Serial.println("Attempting Fast-WIFI connection...");IPAddress l,g,s;bool iv=l.fromString(wifi_local_ip),gv=g.fromString(gateway),sv=s.fromString(subnet);if(!iv||!gv||!sv||wifi_local_ip==""||gateway==""||subnet==""){Serial.println("Invalid static IP config. Falling back to Basic.");basic_setup_wifi();return;}if(!WiFi.config(l,g,s)){Serial.println("STA Failed to config static IP! Falling back to Basic.");basic_setup_wifi();return;}WiFi.mode(WIFI_STA);uint8_t b[6];bool bv=false;if(mac_address.length()==17)bv=(sscanf(mac_address.c_str(),"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",&b[0],&b[1],&b[2],&b[3],&b[4],&b[5])==6);if(wifi_channel>0&&wifi_channel<=13&&bv){Serial.printf("Connecting to SSID: %s on Ch: %d BSSID: %s\n",wifi_ssid.c_str(),wifi_channel,mac_address.c_str());WiFi.begin(wifi_ssid.c_str(),wifi_password.c_str(),wifi_channel,b,true);}else{Serial.printf("Connecting to SSID: %s (no opt)\n",wifi_ssid.c_str());WiFi.begin(wifi_ssid.c_str(),wifi_password.c_str());}uint32_t t=millis()+10000;while(WiFi.status()!=WL_CONNECTED&&millis()<t){delay(500);Serial.print(".");}if(WiFi.status()==WL_CONNECTED){Serial.println("\nWi-Fi connected (Fast-WIFI)");Serial.print("IP: ");Serial.println(WiFi.localIP());Serial.print("RSSI: ");Serial.println(WiFi.RSSI());}else{Serial.println("\nFailed Fast-WIFI connect.");WiFi.disconnect(true);WiFi.mode(WIFI_OFF);}}
void basic_setup_wifi(){Serial.println("Attempting Basic-WIFI connection...");WiFi.mode(WIFI_STA);WiFi.config(INADDR_NONE,INADDR_NONE,INADDR_NONE);WiFi.begin(wifi_ssid.c_str(),wifi_password.c_str());uint32_t t=millis()+10000;while(WiFi.status()!=WL_CONNECTED&&millis()<t){delay(500);Serial.print(".");}if(WiFi.status()==WL_CONNECTED){Serial.println("\nWi-Fi connected (Basic-WIFI)");Serial.print("IP: ");Serial.println(WiFi.localIP());Serial.print("RSSI: ");Serial.println(WiFi.RSSI());}else{Serial.println("\nFailed Basic-WIFI connect.");WiFi.disconnect(true);WiFi.mode(WIFI_OFF);}}

// (reconnect - same as previous version)
void reconnect(){uint8_t r=0;const uint8_t mr=3;while(!client.connected()&&r<mr){r++;Serial.printf("Attempting MQTT connection (%d/%d)...\n",r,mr);String cid=node_name+"-"+WiFi.macAddress();cid.replace(":","");Serial.print(" ClientID: ");Serial.println(cid);Serial.print(" Server: ");Serial.print(mqtt_server);Serial.print(":");Serial.println(mqtt_port);bool c=false;if(mqtt_username.length()>0){Serial.printf(" User: %s\n",mqtt_username.c_str());c=client.connect(cid.c_str(),mqtt_username.c_str(),mqtt_password.c_str());}else{Serial.println(" Anon connection.");c=client.connect(cid.c_str());}if(c){Serial.println("MQTT connected!");Serial.print(" Buffer: ");Serial.println(client.getBufferSize());mqttautodiscovery();}else{Serial.print("MQTT failed, rc=");Serial.print(client.state());switch(client.state()){case -4:Serial.println(" (Timeout)");break;case -3:Serial.println(" (Conn Lost)");break;case -2:Serial.println(" (Conn Failed)");break;case -1:Serial.println(" (Disconnected)");break;case 1:Serial.println(" (Bad Protocol)");break;case 2:Serial.println(" (Bad Client ID)");break;case 3:Serial.println(" (Unavailable)");break;case 4:Serial.println(" (Bad Credentials)");break;case 5:Serial.println(" (Unauthorized)");break;default:Serial.println(" (Unknown)");break;}Serial.println(" Retry in 3s...");delay(3000);}}if(!client.connected())Serial.println("MQTT failed after retries.");}

// (getAverageSoilMoisture, getAverageVoltage - same as previous version)
int getAverageSoilMoisture(){int t=0;for(int i=0;i<5;i++){t+=adc1_get_raw(ADC1_CHANNEL_2);delay(10);}return t/5;} // VERIFY CHANNEL 2 FOR GPIO 3
int getAverageVoltage(){int t=0;for(int i=0;i<5;i++){t+=adc1_get_raw(ADC1_CHANNEL_0);delay(10);}return t/5;} // VERIFY CHANNEL 0 FOR GPIO 1

// --- Simple Done Pin Toggle ---
void toggleDonePin() {
    // Just pulse the pin - called by normal operation OR failsafe
    Serial.println(">>> Toggling DONE pin (GPIO 4) <<<");
    digitalWrite(donePin, HIGH);
    delay(20); // Pulse duration
    digitalWrite(donePin, LOW);
}

void setup() {
    // --- Start the timer IMMEDIATELY ---
    setupStartTime = millis();

    Serial.begin(115200);
    Serial.println("\n\n--- Soil Sensor Booting ---");
    Serial.printf("Failsafe timeout set to %lu ms for WiFi modes\n", failsafeTimeoutDuration);

    pinMode(calibrationButtonPin, INPUT_PULLUP);
    pinMode(blueLEDPin, OUTPUT);
    pinMode(donePin, OUTPUT);
    digitalWrite(blueLEDPin, HIGH); // LED OFF
    digitalWrite(donePin, LOW);   // Ensure done pin is LOW

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11); // Batt
    adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_11); // Light
    adc1_config_channel_atten(ADC1_CHANNEL_2, ADC_ATTEN_DB_11); // Soil - VERIFY!

    // --- Check Button Press ---
    delay(50);
    if (digitalRead(calibrationButtonPin) == LOW) {
        Serial.println("Button pressed at boot.");
        buttonPressTime = millis();
        digitalWrite(blueLEDPin, LOW); // LED ON
        while (digitalRead(calibrationButtonPin) == LOW) { delay(10); }
        digitalWrite(blueLEDPin, HIGH); // LED OFF
        unsigned long pressDuration = millis() - buttonPressTime;
        Serial.printf("Button held for %lu ms.\n", pressDuration);

        if (pressDuration > maxPressTime) {
            blinkLED(6, 100);
            setupSensorConfig(); // This restarts, resetting the failsafe timer
        } else if (pressDuration > minPressTime) {
            blinkLED(3, 200);
            calibrateSoilMoistureSensor(); // This restarts, resetting the failsafe timer
        } else {
             Serial.println("Button press too short, continuing normal boot.");
        }
    }

    // --- Proceed with normal boot if button didn't cause restart ---
    preferences.begin("wifi-config", true);
    // (Load config variables - same as before)
    wifi_ssid = preferences.getString("ssid", ""); wifi_password = preferences.getString("password", ""); node_name = preferences.getString("nodeName", "SoilSensor"); gateway_key = preferences.getString("gatewayKey", "defaultKey"); config_mode = preferences.getUInt("mode", 1); mqtt_server = preferences.getString("mqttserver", ""); mqtt_port = preferences.getUInt("mqttport", 1883); mqtt_username = preferences.getString("mqttusername", ""); mqtt_password = preferences.getString("mqttpassword", ""); wifi_local_ip = preferences.getString("localIP", ""); gateway = preferences.getString("gateway", ""); subnet = preferences.getString("subnet", ""); mac_address = preferences.getString("macAddress", ""); wifi_channel = preferences.getUInt("wifiChannel", 0);
    preferences.end();
    Serial.printf("Loaded Config - Mode: %d, Node: %s\n", config_mode, node_name.c_str());

    Serial.println("Initializing I2C and sensors...");
    Wire.begin(6, 5); // VERIFY I2C PINS!

    if (!sensor0.begin()) Serial.println("WARN: TMP102 not detected.");
    else Serial.println("TMP102 Initialized.");
    if (!aht20.begin()) Serial.println("WARN: AHT20 not detected.");
    else Serial.println("AHT20 Initialized.");

    preferences.begin("soilCalib", true);
    drySoilValue = preferences.getUInt("drySoilValue", 1000);
    wetSoilValue = preferences.getUInt("wetSoilValue", 3000);
    preferences.end();
    Serial.printf("Soil Calib: Dry=%u, Wet=%u\n", drySoilValue, wetSoilValue);
    if (drySoilValue >= wetSoilValue) { drySoilValue = 1000; wetSoilValue = 3000; Serial.println("WARN: Invalid calib, using defaults."); }

    Serial.println("Reading sensor values...");
    int avgBattRaw = getAverageVoltage();
    batteryPercentage = constrain(map(avgBattRaw, 2058, 2730, 0, 100), 0, 100); // CALIBRATE RAW VALUES
    light = adc1_get_raw(ADC1_CHANNEL_1);
    soilMoistureRow = getAverageSoilMoisture();
    soilMpercent = constrain(map(soilMoistureRow, drySoilValue, wetSoilValue, 100, 0), 0, 100);
    Serial.printf(" Readings: BattRaw=%d (%d%%), LightRaw=%d, SoilRaw=%d (%d%%), AirT=%.1fC, AirH=%.1f%%, SoilT=%.1fC\n",
                  avgBattRaw, batteryPercentage, light, soilMoistureRow, soilMpercent, aht20.getTemperature(), aht20.getHumidity(), sensor0.readTempC());

    // --- Setup Network ---
    if (config_mode == 1) { // ESP-NOW
        Serial.println("Configuring ESP-NOW...");
        WiFi.mode(WIFI_STA);
        if (esp_now_init() != ESP_OK) Serial.println("ERROR: ESP-NOW init failed!");
        else {
            esp_now_peer_info_t peerInfo = {}; memcpy(peerInfo.peer_addr, receiverAddress, 6); peerInfo.channel = 0; peerInfo.encrypt = false; peerInfo.ifidx = WIFI_IF_STA;
            if (esp_now_add_peer(&peerInfo) != ESP_OK) Serial.println("ERROR: Failed add ESP-NOW peer!");
            else { esp_now_register_send_cb(OnDataSent); Serial.println("ESP-NOW Initialized."); }
        }
    } else if (config_mode == 0) { // Fast-WIFI
        fast_setup_wifi();
        if (WiFi.status() == WL_CONNECTED) { client.setServer(mqtt_server.c_str(), mqtt_port); client.setBufferSize(1024); reconnect(); }
        else Serial.println("WARN: Fast-WIFI connect failed in setup.");
    } else if (config_mode == 2) { // Basic-WIFI
        basic_setup_wifi();
        if (WiFi.status() == WL_CONNECTED) { client.setServer(mqtt_server.c_str(), mqtt_port); client.setBufferSize(1024); reconnect(); }
        else Serial.println("WARN: Basic-WIFI connect failed in setup.");
    }

    Serial.println("--- Setup Complete ---");
    digitalWrite(blueLEDPin, HIGH); // LED OFF
}

// Callback when data is sent via ESP-NOW
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("ESP-NOW Send Status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
    if (status == ESP_NOW_SEND_SUCCESS) {
         // Normal success signal for ESP-NOW
         toggleDonePin();
    }
    // Optional: Deep sleep after ESP-NOW attempt
    // Serial.println("Entering deep sleep after ESP-NOW attempt.");
    // esp_deep_sleep_start();
}

void calibrateSoilMoistureSensor() {
    preferences.begin("soilCalib", false);
    preferences.clear(); // Clear all previous values
    blinkLED(2, 500); // Blink LED 2 times for dry soil calibration
    delay(1000);
    drySoilValue = getAverageSoilMoisture();
    preferences.putUInt("drySoilValue", drySoilValue);
    blinkLED(3, 500);
    delay(60000); // Wait 60 seconds for plant watering
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
    doc["rw"] = int(getAverageSoilMoisture());
    size_t jsonSize = serializeJson(doc, myData.json, sizeof(myData.json));
    esp_now_send(receiverAddress, (uint8_t *) &myData, jsonSize + 1);
}
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
    client.publish((String(SENSOR_TOPIC) + node_name + "/madcrow").c_str(), String(soilMoistureRow).c_str(), MQTT_RETAIN);
    client.publish((String(SENSOR_TOPIC) + node_name + "/batt").c_str(), String(batteryPercentage).c_str(), MQTT_RETAIN);
    client.publish((String(SENSOR_TOPIC) + node_name + "/lx").c_str(), String(light).c_str(), MQTT_RETAIN);
    client.publish((String(SENSOR_TOPIC) + node_name + "/moisture").c_str(), String(soilMpercent).c_str(), MQTT_RETAIN);
    client.publish((String(SENSOR_TOPIC) + node_name + "/airtmp").c_str(), String(aht20.getTemperature()).c_str(), MQTT_RETAIN);
    client.publish((String(SENSOR_TOPIC) + node_name + "/soiltmp").c_str(), String(sensor0.readTempC()).c_str(), MQTT_RETAIN);
    client.publish((String(SENSOR_TOPIC) + node_name + "/humidity").c_str(), String(aht20.getHumidity()).c_str(), MQTT_RETAIN);
    client.publish((String(SENSOR_TOPIC) + node_name + "/rssi").c_str(), String(WiFi.RSSI()).c_str(), MQTT_RETAIN);
}

unsigned long lastMqttPublishTime = 0;
const unsigned long mqttPublishInterval = 30000; // Publish MQTT every 30 seconds (example)
bool dataPublishedThisCycle = false; // Track if MQTT publish happened in the current WiFi cycle

void loop() {
    unsigned long currentTime = millis();

    // --- 15-Second Failsafe Check for WiFi Modes (runs regardless of success) ---
    if (!failsafeTriggered && (config_mode == 0 || config_mode == 2) && (currentTime - setupStartTime >= failsafeTimeoutDuration)) {
        Serial.println("!!! FAILSAFE: 15 seconds elapsed in WiFi mode. Forcing toggleDonePin() !!!");
        toggleDonePin(); // Trigger the external reset/power-off
    }

    // --- Normal Operation Logic ---
    if (config_mode == 1) { // ESP-NOW Mode
        modeESPNOW(); // Sends data. OnDataSent callback calls toggleDonePin on success.
        Serial.println("ESP-NOW cycle complete. Delaying/Sleeping...");
        // Add delay or deep sleep for ESP-NOW mode
        delay(30000); // Example: Wait 30 seconds for next send attempt
        // Or: esp_deep_sleep_start();

    } else { // WiFi Modes (Fast or Basic)

        // 1. Check WiFi Connection
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi disconnected. Attempting reconnect...");
            dataPublishedThisCycle = false; // Reset publish flag if connection lost
            if (config_mode == 0) fast_setup_wifi();
            else basic_setup_wifi(); // config_mode == 2

            if (WiFi.status() != WL_CONNECTED) {
                 Serial.println("Reconnect failed. Will retry later.");
                 delay(5000); // Wait before next loop iteration
                 return; // Skip MQTT logic for this iteration
            } else {
                 Serial.println("WiFi reconnected.");
                 // Need to reconnect MQTT if WiFi just reconnected
                 reconnect();
            }
        }

        // 2. Check MQTT Connection (only if WiFi is connected)
        if (!client.connected()) {
            Serial.println("MQTT disconnected. Reconnecting...");
            dataPublishedThisCycle = false; // Reset publish flag
            reconnect(); // Blocks until connected or retries fail
        }

        // 3. Handle MQTT Client Loop and Publishing (only if MQTT is connected)
        if (client.connected()) {
            client.loop(); // Keep MQTT connection alive

            // Publish data periodically (e.g., every 30 seconds)
            if (!dataPublishedThisCycle && (currentTime - lastMqttPublishTime >= mqttPublishInterval)) {
                 Serial.println("Publishing MQTT data...");
                 publishmqtt(); // Publish the sensor data
                 lastMqttPublishTime = currentTime;
                 dataPublishedThisCycle = true; // Mark data as published for this cycle

                 // *** Signal success for WiFi mode AFTER successful publish ***
                 toggleDonePin(); // Call done pin toggle after successful publish

                 Serial.println("WiFi/MQTT cycle complete. Delaying/Sleeping...");
                 // Optional: Add delay or deep sleep here for WiFi modes
                 delay(mqttPublishInterval); // Wait for the next interval
                 // Or: esp_deep_sleep_start();
                 dataPublishedThisCycle = false; // Reset for next cycle after delay/sleep
            }
        } else {
             Serial.println("MQTT not connected. Waiting...");
             delay(5000); // Wait before trying to reconnect MQTT again
             dataPublishedThisCycle = false; // Reset flag if MQTT wasn't connected
        }
    } // End WiFi Modes

    // Minimal delay if no other delays happened, prevents busy loop if not sleeping
    delay(10);
}