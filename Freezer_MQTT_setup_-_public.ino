#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into pin D2 on the Arduino
#define ONE_WIRE_BUS 4 //D2

// Setup a oneWire instance to communicate with any OneWire devices
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

// Update these with values suitable for your network.
const char* ssid = "wifiSSID";
const char* password = "password";
const char* mqtt_server = "123.123.123.123";
const int SwitchOutput = 5; //D1

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
long lastOff = 0;
float temp = 0;
//int inPin = 5;

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("Ferm1_temperature_sensor")) {
      Serial.println("connected");
      client.loop();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup(){
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  pinMode(SwitchOutput, OUTPUT);
  digitalWrite(SwitchOutput, LOW);
  sensors.begin();
}

void callback(char* topic, byte* payload, unsigned int length) {
  String recMessage = "";
  for (int i = 0; i < length; i++) {
    recMessage += (char)payload[i];
  }
  long now = millis();
  if (recMessage == "ON") {
    Serial.println("MSG: ON/1");
    if (now - lastOff > 300000) {
      digitalWrite(SwitchOutput, HIGH);
    } else {
      Serial.println("Too soon to turn back on.");
      client.publish("PID/Ferm1/Msg", "Too soon to turn back on.");
    }
  } else if (recMessage == "OFF") {
    Serial.println("MSG: OFF/0");
    digitalWrite(SwitchOutput, LOW);
    lastOff = now;
  }
}

void loop(){
  if (!client.connected()) {
    reconnect();
    client.publish("PID/Ferm1/Msg", "Reconnected.");
    Serial.print("After Reconnect Subscribe:");
    Serial.println(client.subscribe("PID/Ferm1/State"));
  }
  client.loop();
  long now = millis();
  if (now - lastMsg > 30000) {
    lastMsg = now;
    sensors.setResolution(12);
    sensors.requestTemperatures(); // Send the command to get temperatures
    temp = sensors.getTempFByIndex(0);

    if ((temp > -50) && (temp < 130))
    {
      Serial.println(temp);
      client.publish("PID/Ferm1/Temp", String(temp).c_str());
    }
  }
}
