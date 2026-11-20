#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>

// Pin
#define DHT1_PIN 15
#define DHT2_PIN 18
#define LED_PIN 2

// DHT
DHTesp dht1;
DHTesp dht2;

// WiFi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// MQTT
const char* mqtt_server = "broker.emqx.io";

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
String ledJson = "OFF";

// WiFi
void setup_wifi() {
  delay(10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// MQTT Callback
void callback(char* topic, byte* payload, unsigned int length) {

  String message = "";

  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("MQTT Message: ");
  Serial.println(message);

  if (message == "ON") {
    digitalWrite(LED_PIN, HIGH);
    ledJson = "ON";
  }

  if (message == "OFF") {
    digitalWrite(LED_PIN, LOW);
    ledJson = "OFF";
  }
}

// MQTT Reconnect
void reconnect() {

  while (!client.connected()) {

    Serial.print("Attempting MQTT Connection...");

    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {

      Serial.println(" Connected");

      client.subscribe("/cyberacademy/led");

    } else {

      Serial.print(" Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retry 5 seconds");

      delay(5000);
    }
  }
}

void setup() {

  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);

  setup_wifi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  dht1.setup(DHT1_PIN, DHTesp::DHT22);
  dht2.setup(DHT2_PIN, DHTesp::DHT22);

  Serial.println("System Ready");
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  unsigned long now = millis();

  if (now - lastMsg > 2000) {

    lastMsg = now;

    // Baca Sensor 1
    TempAndHumidity data1 = dht1.getTempAndHumidity();

    // Baca Sensor 2
    TempAndHumidity data2 = dht2.getTempAndHumidity();

    // Publish DHT22 #1
    client.publish("/cyberacademy/temp1",
                   String(data1.temperature, 2).c_str());

    client.publish("/cyberacademy/hum1",
                   String(data1.humidity, 2).c_str());

    // Publish DHT22 #2
    client.publish("/cyberacademy/temp2",
                   String(data2.temperature, 2).c_str());

    client.publish("/cyberacademy/hum2",
                   String(data2.humidity, 2).c_str());

    // Kontrol LED otomatis
    if (data1.temperature > 30 || data2.temperature > 30) {
      digitalWrite(LED_PIN, HIGH);
      ledJson = "ON";
    } else {
      digitalWrite(LED_PIN, LOW);
      ledJson = "OFF";
    }

    // Publish status LED
    client.publish("/cyberacademy/ledJson",
                   ledJson.c_str());

    // Serial Monitor
    Serial.println("\n========== DATA SENSOR ==========");

    Serial.print("Temp 1 : ");
    Serial.print(data1.temperature);
    Serial.println(" °C");

    Serial.print("Hum 1  : ");
    Serial.print(data1.humidity);
    Serial.println(" %");

    Serial.print("Temp 2 : ");
    Serial.print(data2.temperature);
    Serial.println(" °C");

    Serial.print("Hum 2  : ");
    Serial.print(data2.humidity);
    Serial.println(" %");

    Serial.print("LED    : ");
    Serial.println(ledJson);

    Serial.println("=================================");
  }
}