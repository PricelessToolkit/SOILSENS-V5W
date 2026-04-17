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
#include "espnow_encryption.h"

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
bool espnow_encryption_enabled = false;
String espnow_encryption_key;

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
void blinkLED(int times, int interval);
void toggleDonePin();
void FailSafe();
void modeESPNOW();
void mqttautodiscovery();
void publishmqtt();
float readAHT20Temperature();
float readAHT20Humidity();
String htmlEscape(const String &value);

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

String htmlEscape(const String &value) {
  String escaped = value;
  escaped.replace("&", "&amp;");
  escaped.replace("\"", "&quot;");
  escaped.replace("<", "&lt;");
  escaped.replace(">", "&gt;");
  return escaped;
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
  preferences.end();
  preferences.begin("soilCalib", true);
  drySoilValue = preferences.getUInt("drySoilValue", 0);
  wetSoilValue = preferences.getUInt("wetSoilValue", 0);
  preferences.end();
  preferences.begin("wifi-config", false);

  WebServer server(80);

  String htmlForm = R"rawliteral(
<!DOCTYPE html>
<html lang="en" data-theme="dark">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>SoilSens-V5W Configuration</title>
  <style>
    :root{--bg-primary:#0a0e14;--bg-secondary:#111821;--bg-tertiary:#1a232f;--bg-card:#151d28;--bg-card-hover:#1a2636;--border-primary:#2a3744;--border-accent:#3d4f5f;--text-primary:#e6edf3;--text-secondary:#8b949e;--text-muted:#6e7681;--accent-primary:#49A6FD;--accent-secondary:#2c83d3;--accent-glow:rgba(73,166,253,0.16);--success:#3fb950;--warning:#d29922;--danger:#f85149;--shadow-md:0 24px 60px rgba(0,0,0,0.45)}
    *{margin:0;padding:0;box-sizing:border-box}
    body{font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;background:
      radial-gradient(circle at top left,rgba(73,166,253,0.16),transparent 28%),
      radial-gradient(circle at top right,rgba(63,185,80,0.08),transparent 24%),
      linear-gradient(180deg,#0a0e14 0%,#101722 100%);color:var(--text-primary);min-height:100vh;line-height:1.5}
    .shell{max-width:1160px;margin:0 auto;padding:28px 18px 40px}
    .hero{display:block;margin-bottom:18px}
    .hero-card,.panel{background:rgba(17,24,33,0.92);border:1px solid var(--border-primary);border-radius:20px;box-shadow:var(--shadow-md);backdrop-filter:blur(6px)}
    .hero-card{padding:24px}
    .hero-title{margin:0;font-size:clamp(28px,4vw,44px);line-height:1.05}
    .hero-subtitle{display:block;margin-top:6px;font-size:clamp(12px,2vw,16px);line-height:1.2;color:var(--text-secondary);font-weight:600;letter-spacing:.04em}
    .eyebrow{display:inline-flex;align-items:center;gap:8px;padding:6px 10px;border-radius:999px;background:var(--accent-glow);color:var(--accent-primary);font-size:12px;font-weight:700;letter-spacing:.08em;text-transform:uppercase}
    .mode-grid,.field-grid{display:grid;gap:16px}
    .mode-grid{grid-template-columns:repeat(3,minmax(0,1fr))}
    .field-grid.two{grid-template-columns:repeat(2,minmax(0,1fr))}
    .section{background:var(--bg-card);border:1px solid var(--border-primary);border-radius:18px;padding:18px;margin-bottom:16px}
    .section-head{display:flex;justify-content:space-between;gap:12px;align-items:flex-start;margin-bottom:14px}
    .section-title{font-size:16px;font-weight:700}
    .section-copy{color:var(--text-secondary);font-size:13px;max-width:520px}
    .mode-card{position:relative}
    .mode-card input{position:absolute;opacity:0;pointer-events:none}
    .mode-option{display:block;height:100%;padding:16px;border-radius:16px;background:var(--bg-tertiary);border:1px solid var(--border-primary);cursor:pointer;transition:transform .2s,border-color .2s,background .2s}
    .mode-option strong{display:block;font-size:15px;margin-bottom:6px}
    .mode-option span{display:block;color:var(--text-secondary);font-size:13px}
    .mode-card input:checked + .mode-option{border-color:var(--accent-primary);background:var(--accent-glow);transform:translateY(-2px)}
    .form-group{margin-bottom:14px}
    .form-label{display:block;font-size:12px;font-weight:700;color:var(--text-secondary);margin-bottom:6px;text-transform:uppercase;letter-spacing:.06em}
    .form-input{width:100%;padding:12px 14px;border-radius:12px;border:1px solid var(--border-primary);background:var(--bg-tertiary);color:var(--text-primary);font-size:14px;transition:border-color .2s,box-shadow .2s}
    .form-input:focus{outline:none;border-color:var(--accent-primary);box-shadow:0 0 0 4px rgba(73,166,253,0.12)}
    .toggle-row{display:flex;align-items:center;justify-content:space-between;gap:14px;padding:14px 16px;border-radius:14px;background:var(--bg-tertiary);border:1px solid var(--border-primary);margin-bottom:14px}
    .toggle-copy strong{display:block;font-size:14px;margin-bottom:4px}
    .toggle-copy span{display:block;color:var(--text-secondary);font-size:12px}
    .toggle-input{width:22px;height:22px;accent-color:var(--accent-primary);flex-shrink:0}
    .form-hint{font-size:12px;color:var(--text-muted);margin-top:6px}
    .pill{display:inline-flex;align-items:center;gap:8px;padding:8px 12px;border-radius:999px;background:rgba(63,185,80,0.12);color:var(--success);font-size:12px;font-weight:700}
    .pill::before{content:"";width:8px;height:8px;border-radius:999px;background:currentColor}
    .actions{display:flex;gap:12px;flex-wrap:wrap;align-items:center}
    .settings-actions{flex-wrap:nowrap;margin-bottom:12px}
    .calibration-actions{margin-bottom:16px;flex-wrap:nowrap}
    .btn{appearance:none;border:none;border-radius:12px;padding:13px 18px;font-size:14px;font-weight:700;cursor:pointer;transition:transform .2s,opacity .2s}
    .btn-primary{background:linear-gradient(135deg,var(--accent-primary),var(--accent-secondary));color:#fff}
    .btn-secondary{background:var(--bg-tertiary);color:var(--text-primary);border:1px solid var(--border-primary)}
    .btn-danger{background:linear-gradient(135deg,#f85149,#cf222e);color:#fff}
    .btn-half{flex:1 1 50%;width:50%}
    .btn-full{width:100%;justify-content:center}
    .btn-calibration{flex:1 1 50%;width:50%;color:#fff}
    .btn-dry{background:linear-gradient(135deg,#8a5a2b,#6f451f)}
    .btn-wet{background:linear-gradient(135deg,var(--accent-primary),var(--accent-secondary))}
    .status-row{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:12px;margin-bottom:16px}
    .status-row.two-col{display:grid;grid-template-columns:minmax(0,1fr) minmax(0,1fr) !important;align-items:stretch}
    .status-chip{padding:14px;border-radius:14px;background:var(--bg-tertiary);border:1px solid var(--border-primary)}
    .status-chip strong{display:block;font-size:12px;color:var(--text-muted);text-transform:uppercase;letter-spacing:.06em;margin-bottom:6px}
    .status-chip span{display:block;font-size:16px;font-weight:700;word-break:break-word}
    .status-chip.live-chip{border-color:rgba(63,185,80,.35);background:rgba(63,185,80,.10)}
    .status-chip.live-chip span{color:var(--success)}
    .status-chip.dry-chip{border-color:rgba(138,90,43,.38);background:rgba(138,90,43,.16)}
    .status-chip.dry-chip span{color:#c79057}
    .status-chip.moist-chip{border-color:rgba(73,166,253,.35);background:rgba(73,166,253,.12)}
    .status-chip.moist-chip span{color:var(--accent-primary)}
    .btn:hover{transform:translateY(-1px)}
    .footer-note{color:var(--text-muted);font-size:12px}
    .conditional{display:none}
    .conditional.show{display:block}
    @media (max-width:900px){.mode-grid,.field-grid.two,.status-row{grid-template-columns:1fr}}
  </style>
</head>
<body>
  <div class="shell">
    <section class="hero">
      <div class="hero-card">
        <h1 class="hero-title">SoilSense-V5W<span class="hero-subtitle">by PricelessToolkit</span></h1>
      </div>
    </section>

    <form action="/submit" method="post">
      <section class="section">
        <div class="section-head">
          <div>
            <div class="section-title">Connection Mode</div>
            <div class="section-copy">Choose the transport mode that matches your deployment.</div>
          </div>
        </div>
        <div class="mode-grid">
          <label class="mode-card">
            <input type="radio" id="mode1" name="mode" value="1" __MODE_1__>
            <span class="mode-option"><strong>ESP-NOW</strong><span>Direct peer-to-peer sending. No WiFi or MQTT fields required.</span></span>
          </label>
          <label class="mode-card">
            <input type="radio" id="mode0" name="mode" value="0" __MODE_0__>
            <span class="mode-option"><strong>Fast-WIFI</strong><span>Locks onto a known access point with fixed network details for faster reconnects.</span></span>
          </label>
          <label class="mode-card">
            <input type="radio" id="mode2" name="mode" value="2" __MODE_2__>
            <span class="mode-option"><strong>Basic-WIFI</strong><span>Standard WiFi client mode with DHCP-style simplicity.</span></span>
          </label>
        </div>
      </section>

      <section class="section">
        <div class="section-head">
          <div>
            <div class="section-title">Identity</div>
            <div class="section-copy">Set the sensor name that will appear in Home Assistant under MQTT.</div>
          </div>
        </div>
        <div class="field-grid two">
          <div class="form-group">
            <label class="form-label" for="nodeName">Node Name</label>
            <input class="form-input" type="text" id="nodeName" name="nodeName" value="__NODE_NAME__">
          </div>
          <div class="form-group conditional" id="gatewayKeyField">
            <label class="form-label" for="gatewayKey">Gateway Key</label>
            <input class="form-input" type="text" id="gatewayKey" name="gatewayKey" value="__GATEWAY_KEY__">
          </div>
        </div>
        <div class="conditional" id="espNowEncryptionBlock">
          <div class="toggle-row">
            <div class="toggle-copy">
              <strong>Encryption</strong>
              <span>Enable XOR encryption for ESP-NOW packets.</span>
            </div>
            <input class="toggle-input" type="checkbox" id="espnowEncryptionEnabled" name="espnowEncryptionEnabled" __ESPNOW_ENCRYPTION_ENABLED__>
          </div>
          <div class="field-grid two conditional" id="espNowEncryptionFields">
            <div class="form-group">
              <label class="form-label" for="espnowEncryptionKey">Encryption Key</label>
              <input class="form-input" type="text" id="espnowEncryptionKey" name="espnowEncryptionKey" value="__ESPNOW_ENCRYPTION_KEY__">
              <div class="form-hint">Use gateway format like ["0x4B","0xA3","0x3F","0x9C"]</div>
            </div>
          </div>
        </div>
      </section>

      <section class="section conditional" id="commonWifiFields">
        <div class="section-head">
          <div>
            <div class="section-title">WiFi Access</div>
            <div class="section-copy">Credentials shared by Fast-WIFI and Basic-WIFI.</div>
          </div>
        </div>
        <div class="field-grid two">
          <div class="form-group">
            <label class="form-label" for="ssid">WiFi SSID</label>
            <input class="form-input" type="text" id="ssid" name="ssid" value="__SSID__">
          </div>
          <div class="form-group">
            <label class="form-label" for="password">WiFi Password</label>
            <input class="form-input" type="password" id="password" name="password" value="__PASSWORD__">
          </div>
        </div>
      </section>

      <section class="section conditional" id="wifiAdvancedFields">
        <div class="section-head">
          <div>
            <div class="section-title">Fast-WIFI Network Details</div>
            <div class="section-copy">These values are only used in Fast-WIFI mode.</div>
          </div>
        </div>
        <div class="field-grid two">
          <div class="form-group">
            <label class="form-label" for="macAddress">AP MAC Address</label>
            <input class="form-input" type="text" id="macAddress" name="macAddress" value="__MAC_ADDRESS__">
            <div class="form-hint">Use the radio interface MAC address of your access point.</div>
          </div>
          <div class="form-group">
            <label class="form-label" for="wifiChannel">WiFi Channel</label>
            <input class="form-input" type="text" id="wifiChannel" name="wifiChannel" value="__WIFI_CHANNEL__">
            <div class="form-hint">The access point must use a fixed WiFi channel.</div>
          </div>
          <div class="form-group">
            <label class="form-label" for="localIP">Local IP</label>
            <input class="form-input" type="text" id="localIP" name="localIP" value="__LOCAL_IP__">
            <div class="form-hint">Use a free IP address from your local range and reserve it for this sensor.</div>
          </div>
          <div class="form-group">
            <label class="form-label" for="gateway">Gateway</label>
            <input class="form-input" type="text" id="gateway" name="gateway" value="__GATEWAY__">
          </div>
          <div class="form-group">
            <label class="form-label" for="subnet">Subnet</label>
            <input class="form-input" type="text" id="subnet" name="subnet" value="__SUBNET__">
          </div>
        </div>
      </section>

      <section class="section conditional" id="mqttFields">
        <div class="section-head">
          <div>
            <div class="section-title">MQTT Broker</div>
            <div class="section-copy">These fields are only needed for WiFi-based modes.</div>
          </div>
        </div>
        <div class="field-grid two">
          <div class="form-group">
            <label class="form-label" for="mqttserver">MQTT Server</label>
            <input class="form-input" type="text" id="mqttserver" name="mqttserver" value="__MQTT_SERVER__">
          </div>
          <div class="form-group">
            <label class="form-label" for="mqttport">MQTT Port</label>
            <input class="form-input" type="text" id="mqttport" name="mqttport" value="__MQTT_PORT__">
          </div>
          <div class="form-group">
            <label class="form-label" for="mqttusername">MQTT Username</label>
            <input class="form-input" type="text" id="mqttusername" name="mqttusername" value="__MQTT_USERNAME__">
          </div>
          <div class="form-group">
            <label class="form-label" for="mqttpassword">MQTT Password</label>
            <input class="form-input" type="password" id="mqttpassword" name="mqttpassword" value="__MQTT_PASSWORD__">
          </div>
        </div>
      </section>

      <section class="section">
        <div class="section-head">
          <div>
            <div class="section-title">Calibration</div>
            <div class="section-copy">Capture the current raw soil moisture reading and save it as your dry or moist calibration point.</div>
          </div>
        </div>
        <div class="status-row">
          <div class="status-chip live-chip">
            <strong>SOIL SENSOR RAW VALUE "LIVE"</strong>
            <span id="calibrationRawValue">__CALIBRATION_RAW__</span>
          </div>
        </div>
        <div class="status-row two-col">
          <div class="status-chip dry-chip">
            <strong>Dry Soil</strong>
            <span id="dryCalibrationValue">__DRY_SOIL_VALUE__</span>
          </div>
          <div class="status-chip moist-chip">
            <strong>WET SOIL</strong>
            <span id="wetCalibrationValue">__WET_SOIL_VALUE__</span>
          </div>
        </div>
        <div class="actions calibration-actions">
          <button class="btn btn-secondary btn-calibration btn-dry" type="button" id="captureDrySoil">SET DRY SOIL</button>
          <button class="btn btn-secondary btn-calibration btn-wet" type="button" id="captureWetSoil">SET WET SOIL</button>
        </div>
        <input type="hidden" id="drySoilCalibration" name="drySoilCalibration" value="__DRY_SOIL_VALUE_RAW__">
        <input type="hidden" id="wetSoilCalibration" name="wetSoilCalibration" value="__WET_SOIL_VALUE_RAW__">
        <div class="footer-note">Place the sensor in the target condition first, then capture the raw value and save configuration.</div>
      </section>

      <section class="section">
        <div class="actions settings-actions">
          <button class="btn btn-primary btn-half" type="submit">Save Configuration</button>
          <button class="btn btn-secondary btn-half" type="button" onclick="window.location.reload()">Reload Values</button>
        </div>
        <button class="btn btn-danger btn-full" type="submit" formaction="/reset-defaults" formmethod="post">Reset to Default</button>
        <div class="footer-note">Save stores the current settings. Reset to Default clears saved settings and loads the firmware defaults after restart.</div>
      </section>
    </form>
  </div>

  <script>
    let calibrationPollHandle = null;

    async function fetchCalibrationRawValue() {
      const response = await fetch('/calibration-raw', { cache: 'no-store' });
      if (!response.ok) throw new Error('Failed to read soil calibration value');
      return response.json();
    }

    async function refreshCalibrationRawValue() {
      try {
        const payload = await fetchCalibrationRawValue();
        document.getElementById('calibrationRawValue').textContent = String(payload.rawValue);
      } catch (error) {
      }
    }

    async function captureCalibration(target) {
      try {
        const payload = await fetchCalibrationRawValue();
        const rawValue = String(payload.rawValue);
        document.getElementById('calibrationRawValue').textContent = rawValue;
        if (target === 'dry') {
          document.getElementById('dryCalibrationValue').textContent = rawValue;
          document.getElementById('drySoilCalibration').value = rawValue;
        } else {
          document.getElementById('wetCalibrationValue').textContent = rawValue;
          document.getElementById('wetSoilCalibration').value = rawValue;
        }
      } catch (error) {
        alert(error.message);
      }
    }

    function toggleFields() {
      var selected = document.querySelector('input[name="mode"]:checked');
      var mode = selected ? selected.value : '0';
      var encryptionToggle = document.getElementById('espnowEncryptionEnabled');
      var encryptionEnabled = encryptionToggle ? encryptionToggle.checked : false;
      document.getElementById('gatewayKeyField').classList.toggle('show', mode === '1');
      document.getElementById('espNowEncryptionBlock').classList.toggle('show', mode === '1');
      document.getElementById('espNowEncryptionFields').classList.toggle('show', mode === '1' && encryptionEnabled);
      document.getElementById('commonWifiFields').classList.toggle('show', mode !== '1');
      document.getElementById('wifiAdvancedFields').classList.toggle('show', mode === '0');
      document.getElementById('mqttFields').classList.toggle('show', mode !== '1');
    }

    document.querySelectorAll('input[name="mode"]').forEach(function(input) {
      input.addEventListener('change', toggleFields);
    });
    document.getElementById('espnowEncryptionEnabled').addEventListener('change', toggleFields);
    document.getElementById('captureDrySoil').addEventListener('click', function() { captureCalibration('dry'); });
    document.getElementById('captureWetSoil').addEventListener('click', function() { captureCalibration('wet'); });
    window.addEventListener('load', function() {
      toggleFields();
      refreshCalibrationRawValue();
      calibrationPollHandle = window.setInterval(refreshCalibrationRawValue, 1000);
    });
  </script>
</body>
</html>
)rawliteral";

  htmlForm.replace("__NODE_NAME__", htmlEscape(node_name));
  htmlForm.replace("__GATEWAY_KEY__", htmlEscape(gateway_key));
  htmlForm.replace("__SSID__", htmlEscape(wifi_ssid));
  htmlForm.replace("__PASSWORD__", htmlEscape(wifi_password));
  htmlForm.replace("__MQTT_SERVER__", htmlEscape(mqtt_server));
  htmlForm.replace("__MQTT_PORT__", String(mqtt_port));
  htmlForm.replace("__MQTT_USERNAME__", htmlEscape(mqtt_username));
  htmlForm.replace("__MQTT_PASSWORD__", htmlEscape(mqtt_password));
  htmlForm.replace("__LOCAL_IP__", htmlEscape(wifi_local_ip));
  htmlForm.replace("__GATEWAY__", htmlEscape(gateway));
  htmlForm.replace("__SUBNET__", htmlEscape(subnet));
  htmlForm.replace("__MAC_ADDRESS__", htmlEscape(mac_address));
  htmlForm.replace("__WIFI_CHANNEL__", String(wifi_channel));
  htmlForm.replace("__ESPNOW_ENCRYPTION_ENABLED__", espnow_encryption_enabled ? "checked" : "");
  htmlForm.replace("__ESPNOW_ENCRYPTION_KEY__", htmlEscape(espnow_encryption_key));
  htmlForm.replace("__CALIBRATION_RAW__", String(getAverageSoilMoisture()));
  htmlForm.replace("__DRY_SOIL_VALUE__", drySoilValue ? String(drySoilValue) : String("Not set"));
  htmlForm.replace("__WET_SOIL_VALUE__", wetSoilValue ? String(wetSoilValue) : String("Not set"));
  htmlForm.replace("__DRY_SOIL_VALUE_RAW__", drySoilValue ? String(drySoilValue) : "");
  htmlForm.replace("__WET_SOIL_VALUE_RAW__", wetSoilValue ? String(wetSoilValue) : "");
  htmlForm.replace("__MODE_0__", config_mode == 0 ? "checked" : "");
  htmlForm.replace("__MODE_1__", config_mode == 1 ? "checked" : "");
  htmlForm.replace("__MODE_2__", config_mode == 2 ? "checked" : "");

  server.on("/", [&]() {
    server.send(200, "text/html", htmlForm);
  });

  server.on("/calibration-raw", HTTP_GET, [&]() {
    String json = String("{\"rawValue\":") + String(getAverageSoilMoisture()) + "}";
    server.send(200, "application/json", json);
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
      bool espnowEncryptionEnabledArg = server.hasArg("espnowEncryptionEnabled");
      String espnowEncryptionKeyArg = server.arg("espnowEncryptionKey");
      String drySoilCalibrationArg = server.arg("drySoilCalibration");
      String wetSoilCalibrationArg = server.arg("wetSoilCalibration");

      if (espnowEncryptionKeyArg.length() == 0) {
        espnowEncryptionKeyArg = defaultEspNowEncryptionKeyString();
      }

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
      preferences.putBool("espEncOn", espnowEncryptionEnabledArg);
      preferences.putString("espEncKey", espnowEncryptionKeyArg);
      preferences.end();

      preferences.begin("soilCalib", false);
      if (drySoilCalibrationArg.length()) {
        drySoilValue = drySoilCalibrationArg.toInt();
        preferences.putUInt("drySoilValue", drySoilValue);
      }
      if (wetSoilCalibrationArg.length()) {
        wetSoilValue = wetSoilCalibrationArg.toInt();
        preferences.putUInt("wetSoilValue", wetSoilValue);
      }
      preferences.end();

      String modeLabel = mode == "1" ? "ESP-NOW" : (mode == "0" ? "Fast-WIFI" : "Basic-WIFI");
      String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Configuration Saved</title>
  <style>
    body{margin:0;min-height:100vh;display:grid;place-items:center;font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;background:linear-gradient(180deg,#0a0e14 0%,#101722 100%);color:#e6edf3;padding:20px}
    .card{max-width:720px;width:100%;background:#151d28;border:1px solid #2a3744;border-radius:20px;padding:28px;box-shadow:0 24px 60px rgba(0,0,0,.45)}
    h1{margin:0 0 10px;font-size:30px}
    p{color:#8b949e}
    table{width:100%;border-collapse:collapse;margin-top:18px}
    th,td{padding:10px 12px;border-bottom:1px solid #2a3744;text-align:left;vertical-align:top;word-break:break-word}
    th{color:#8b949e;font-size:12px;text-transform:uppercase;letter-spacing:.08em}
    .pill{display:inline-flex;padding:8px 12px;border-radius:999px;background:rgba(63,185,80,.14);color:#3fb950;font-weight:700;font-size:12px}
  </style>
</head>
<body>
  <div class="card">
    <span class="pill">Saved successfully</span>
    <h1>Configuration Saved</h1>
    <p>Your settings were stored and the device will restart in a moment.</p>
    <table>
      <tr><th>Field</th><th>Value</th></tr>
      <tr><td>Node Name</td><td>__NODE_NAME__</td></tr>
      <tr><td>Gateway Key</td><td>__GATEWAY_KEY__</td></tr>
      <tr><td>Mode</td><td>__MODE__</td></tr>
      <tr><td>ESP-NOW Encryption</td><td>__ESPNOW_ENCRYPTION__</td></tr>
      <tr><td>ESP-NOW Key</td><td>__ESPNOW_ENCRYPTION_KEY__</td></tr>
      <tr><td>Dry Soil Calibration</td><td>__DRY_SOIL_VALUE__</td></tr>
      <tr><td>Moist Soil Calibration</td><td>__WET_SOIL_VALUE__</td></tr>
      <tr><td>WiFi SSID</td><td>__SSID__</td></tr>
      <tr><td>WiFi Password</td><td>__PASSWORD__</td></tr>
      <tr><td>MQTT Server</td><td>__MQTT_SERVER__</td></tr>
      <tr><td>MQTT Port</td><td>__MQTT_PORT__</td></tr>
      <tr><td>MQTT Username</td><td>__MQTT_USERNAME__</td></tr>
      <tr><td>MQTT Password</td><td>__MQTT_PASSWORD__</td></tr>
      <tr><td>Local IP</td><td>__LOCAL_IP__</td></tr>
      <tr><td>Gateway</td><td>__GATEWAY__</td></tr>
      <tr><td>Subnet</td><td>__SUBNET__</td></tr>
      <tr><td>MAC Address</td><td>__MAC_ADDRESS__</td></tr>
      <tr><td>WiFi Channel</td><td>__WIFI_CHANNEL__</td></tr>
    </table>
  </div>
</body>
</html>
)rawliteral";
      html.replace("__NODE_NAME__", htmlEscape(nodeName));
      html.replace("__GATEWAY_KEY__", htmlEscape(gatewayKey));
      html.replace("__MODE__", htmlEscape(modeLabel));
      html.replace("__ESPNOW_ENCRYPTION__", htmlEscape(espnowEncryptionEnabledArg ? "On" : "Off"));
      html.replace("__ESPNOW_ENCRYPTION_KEY__", htmlEscape(espnowEncryptionKeyArg));
      html.replace("__DRY_SOIL_VALUE__", drySoilCalibrationArg.length() ? htmlEscape(drySoilCalibrationArg) : String("Unchanged"));
      html.replace("__WET_SOIL_VALUE__", wetSoilCalibrationArg.length() ? htmlEscape(wetSoilCalibrationArg) : String("Unchanged"));
      html.replace("__SSID__", htmlEscape(ssid));
      html.replace("__PASSWORD__", htmlEscape(password));
      html.replace("__MQTT_SERVER__", htmlEscape(mqttserver));
      html.replace("__MQTT_PORT__", htmlEscape(mqttport));
      html.replace("__MQTT_USERNAME__", htmlEscape(mqttusername));
      html.replace("__MQTT_PASSWORD__", htmlEscape(mqttpassword));
      html.replace("__LOCAL_IP__", htmlEscape(localIP));
      html.replace("__GATEWAY__", htmlEscape(gatewayArg));
      html.replace("__SUBNET__", htmlEscape(subnetArg));
      html.replace("__MAC_ADDRESS__", htmlEscape(macAddress));
      html.replace("__WIFI_CHANNEL__", htmlEscape(wifiChannel));

      server.send(200, "text/html", html);
      delay(500);
      ESP.restart();
    } else {
      server.send(405, "text/html", "<html><body><h1>Method Not Allowed</h1></body></html>");
      delay(500);
      ESP.restart();
    }
  });

  server.on("/reset-defaults", HTTP_POST, [&]() {
    preferences.begin("wifi-config", false);
    preferences.clear();
    preferences.end();

    preferences.begin("soilCalib", false);
    preferences.clear();
    preferences.end();

    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Defaults Restored</title>
  <style>
    body{margin:0;min-height:100vh;display:grid;place-items:center;font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;background:linear-gradient(180deg,#0a0e14 0%,#101722 100%);color:#e6edf3;padding:20px}
    .card{max-width:640px;width:100%;background:#151d28;border:1px solid #2a3744;border-radius:20px;padding:28px;box-shadow:0 24px 60px rgba(0,0,0,.45)}
    h1{margin:0 0 10px;font-size:30px}
    p{color:#8b949e;line-height:1.6}
    .pill{display:inline-flex;padding:8px 12px;border-radius:999px;background:rgba(248,81,73,.14);color:#f85149;font-weight:700;font-size:12px;margin-bottom:12px}
  </style>
</head>
<body>
  <div class="card">
    <span class="pill">Defaults restored</span>
    <h1>Configuration Reset</h1>
    <p>Saved settings were cleared. The sensor will restart and load the firmware default values.</p>
  </div>
</body>
</html>
)rawliteral";

    server.send(200, "text/html", html);
    delay(500);
    ESP.restart();
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
  espnow_encryption_enabled = preferences.getBool("espEncOn", false);
  espnow_encryption_key = preferences.getString("espEncKey", defaultEspNowEncryptionKeyString());
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
    unsigned long buttonPressTime = millis();
    while (digitalRead(calibrationButtonPin) == LOW) {
      delay(1);
    }

    if (digitalRead(calibrationButtonPin) == HIGH) {
      unsigned long pressDuration = millis() - buttonPressTime;

      if (pressDuration >= maxPressTime) {
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
  if (espnow_encryption_enabled) {
    uint8_t key[MAX_ESPNOW_ENCRYPTION_KEY_LENGTH];
    size_t keyLength = 0;
    if (parseEspNowEncryptionKey(espnow_encryption_key, key, keyLength)) {
      xorInPlace(reinterpret_cast<uint8_t *>(myData.json), jsonSize, key, keyLength);
    } else {
      Serial.println("Invalid ESP-NOW encryption key configuration, sending plaintext payload");
    }
  }
  esp_now_send(receiverAddress, reinterpret_cast<uint8_t *>(myData.json), jsonSize);
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
