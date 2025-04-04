#include <MKRNB.h>
// sim card pin if needed
const char PINNUMBER[] = "";
const char* mqtt_server = "broker.mqtt.cool";
const int mqtt_port = 1883;
const char* clientID = "MKRNB1500";
const char* topic = "test/mkrnb";
const char* payload = nullptr;
// Button
const int buttonPin = 14;
// NB/MQTT Objects
NBClient client;
GPRS gprs;
NB nbAccess;
void connectToNB_IoT() {
  Serial.println("Connecting to NB-IoT Network...");
  while (nbAccess.begin(PINNUMBER) != NB_READY || gprs.attachGPRS() != GPRS_READY) {
    Serial.println("Not connected. Retrying...");
    delay(2000);
  }
  Serial.println("Connected to NB-IoT!");
}
bool connectToMQTT() {
  Serial.println("Connecting to MQTT...");
  client.stop();
  delay(1000);
  if (!client.connect(mqtt_server, mqtt_port)) {
    Serial.println("MQTT connect failed!");
    return false;
  }
  Serial.println("Connected to broker");
  uint8_t connectPacket[14 + strlen(clientID)] = {
    0x10, (uint8_t)(12 + strlen(clientID)),
    0x00, 0x04, 'M', 'Q', 'T', 'T',
    0x04,
    0x02,
    0x00, 0x3C,
    0x00, (uint8_t)strlen(clientID)
  };
  memcpy(connectPacket + 14, clientID, strlen(clientID));
  client.write(connectPacket, sizeof(connectPacket));
  Serial.println("MQTT CONNECT sent, waiting for CONNACK...");
  unsigned long timeout = millis();
  while (!client.available()) {
    if (millis() - timeout > 3000) {
      Serial.println("Timeout waiting for CONNACK");
      return false;
    }
  }
  uint8_t response[4];
  int i = 0;
  while (client.available() && i < 4) {
    response[i++] = client.read();
  }
  if (response[0] == 0x20 && response[1] == 0x02 && response[3] == 0x00) {
    Serial.println("MQTT connection successful");
    return true;
  }
  Serial.println("MQTT CONNACK error");
  return false;
}
void publishMQTTMessage() {
  Serial.println("Publishing MQTT Message...");
  uint8_t publishPacket[4 + strlen(topic) + strlen(payload)] = {
    0x30, (uint8_t)(2 + strlen(topic) + strlen(payload)),
    0x00, (uint8_t)strlen(topic)
  };
  memcpy(publishPacket + 4, topic, strlen(topic));
  memcpy(publishPacket + 4 + strlen(topic), payload, strlen(payload));
  client.write(publishPacket, sizeof(publishPacket));
  Serial.println("Message sent!");
}
void setup() {
  Serial.begin(9600);
  pinMode(buttonPin, INPUT_PULLUP);
  Serial.println("Ready. Hold button to idle. Release to trigger.");
}
void loop() {
  if (digitalRead(buttonPin) == LOW) {
    Serial.println("Button held - idle mode");
    // Set payload to "white" before sending
    payload = "white";
    connectToNB_IoT();
    if (connectToMQTT()) {
      publishMQTTMessage();
    } else {
      Serial.println("MQTT send failed (white)");
    }
    delay(200);
    return;
  }
  Serial.println("Button released - waiting 5s before sending...");
  unsigned long start = millis();
  while (millis() - start < 5000) {
    if (digitalRead(buttonPin) == LOW) {
      Serial.println("Button pressed again - aborting");
      return;
    }
    delay(100);
  }
  // Still released after 5s → send "red"
  Serial.println("Triggering MQTT send");
  payload = "red";
  connectToNB_IoT();
  if (connectToMQTT()) {
    publishMQTTMessage();
  } else {
    Serial.println("MQTT send failed (red)");
  }
  delay(5000);
}