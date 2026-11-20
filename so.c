#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

// ====== WiFi Credentials ======
const char* ssid = "SSID";           // Replace with your WiFi SSID
const char* password = "Passwd";          // Replace with your WiFi password

// ====== MQTT Broker Settings ======
const char* mqtt_server = "broker.emqx.io";
const int mqttPort = 1883;
const char* mqtt_User = "admin";
const char* mqtt_Password = "public";

const char* pubTempTopic = "Team/sensor/temperature";
const char* pubHumidTopic = "team/sensor/humidity";
const char* subtopic = "Tushar/LED";

// ====== GPIO Pin Definitions ======
const int redLED = 26;
const int yellowLED = 25;
const int DHTPIN = 33;
#define DHTTYPE DHT11

// ====== Global Variables ======
WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);

unsigned long lastSensorRead = 0;
unsigned long lastStatusPrint = 0;
const long sensorInterval = 5000;
const long statusPrintInterval = 10000; // Show WiFi/MQTT status every 10 sec

bool wifiConnected = false;
bool mqttConnected = false;

// ====== WiFi Connection ======
void setup_wifi() {
  Serial.println("\n[INFO] Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 20) {
    delay(500);
    Serial.print(".");
    attempt++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ [SUCCESS] WiFi Connected!");
    Serial.print("[INFO] IP Address: ");
    Serial.println(WiFi.localIP());
    wifiConnected = true;
  } else {
    Serial.println("\n❌ [ERROR] WiFi Connection Failed! Check Credentials.");
    wifiConnected = false;
  }
}

// ====== MQTT Reconnection Handling ======
void reconnect() {
  if (!client.connected()) {
    Serial.println("\n[INFO] Connecting to MQTT...");

            // Generate a unique client ID using MAC address
    String clientId = "ESP32Client-";
    clientId += WiFi.macAddress(); // e.g., ESP32Client-AA:BB:CC:DD:EE:FF

    if (client.connect(clientId.c_str(), mqtt_User, mqtt_Password)) {
      Serial.println("✅ [SUCCESS] Connected to MQTT Broker!");
      client.subscribe(subtopic);
      Serial.print("[INFO] Subscribed to Topic: ");
      Serial.println(subtopic);
      mqttConnected = true;
    } else {
      Serial.print("❌ [ERROR] MQTT Connection Failed, Code: ");
      Serial.println(client.state());
      Serial.println("[INFO] Retrying in 2 seconds...");
      mqttConnected = false;
      delay(2000);
    }
  }
}

// ====== LED Control Function ======
void controlLED(String msg) {
  if (msg == "on1") {
    digitalWrite(redLED, HIGH);
    Serial.println("[ACTION] Red LED Turned ON ✅");
  } else if (msg == "off1") {
    digitalWrite(redLED, LOW);
    Serial.println("[ACTION] Red LED Turned OFF ❌");
  } else if (msg == "on2") {
    digitalWrite(yellowLED, HIGH);
    Serial.println("[ACTION] Yellow LED Turned ON ✅");
  } else if (msg == "off2") {
    digitalWrite(yellowLED, LOW);
    Serial.println("[ACTION] Yellow LED Turned OFF ❌");
  } else {
    Serial.println("[WARNING] Unknown Command Received! ❗");
  }
}

// ====== MQTT Callback Function ======
void callback(char* topic, byte* payload, unsigned int length) {
  String msgString;
  for (int i = 0; i < length; i++) {
    msgString += (char)payload[i];
  }

  Serial.print("\n📩 [RECEIVED] Message from ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(msgString);

  controlLED(msgString);
}

// ====== Publish Sensor Data (Non-Blocking) ======
void publishSensorData() {
  if (millis() - lastSensorRead >= sensorInterval) {
    lastSensorRead = millis();
    
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (!isnan(temperature) && !isnan(humidity)) {
      char tempString[10], humidString[10];
      dtostrf(temperature, 6, 2, tempString);
      dtostrf(humidity, 6, 2, humidString);

      client.publish(pubTempTopic, tempString);
      Serial.print("📤 [PUBLISH] Temperature Sent to ");
      Serial.print(pubTempTopic);
      Serial.print(": ");
      Serial.println(tempString);

      client.publish(pubHumidTopic, humidString);
      Serial.print("📤 [PUBLISH] Humidity Sent to ");
      Serial.print(pubHumidTopic);
      Serial.print(": ");
      Serial.println(humidString);
    } else {
      Serial.println("❌ [ERROR] Failed to Read Temperature or Humidity!");
    }
  }
}

// ====== Print WiFi & MQTT Status Every 10 sec ======
void printStatus() {
  if (millis() - lastStatusPrint >= statusPrintInterval) {
    lastStatusPrint = millis();

    Serial.println("\n===== 📡 SYSTEM STATUS =====");
    Serial.print("🌐 WiFi: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "Connected ✅" : "Disconnected ❌");

    Serial.print("🔌 MQTT: ");
    Serial.println(client.connected() ? "Connected ✅" : "Disconnected ❌");
    Serial.println("============================\n");
  }
}

// ====== Arduino Setup ======
void setup() {
  Serial.begin(115200);
  Serial.println("\n===== 🚀 ESP32 MQTT Sensor Node Booting... =====");

  setup_wifi();
  
  client.setServer(mqtt_server, mqttPort);
  client.setCallback(callback);

  pinMode(redLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);
  dht.begin();
}

// ====== Arduino Loop ======
void loop() {
  if (!wifiConnected || WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ [WARNING] WiFi Disconnected! Reconnecting...");
    setup_wifi();
  }

  if (!client.connected()) {
    Serial.println("⚠️ [WARNING] MQTT Disconnected! Reconnecting...");
    reconnect();
  }

  client.loop();  
  publishSensorData();  
  printStatus(); // Show system status every 10 sec
}
