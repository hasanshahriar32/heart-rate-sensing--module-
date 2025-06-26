#ifndef MQTT_PUBLISH_H
#define MQTT_PUBLISH_H

#include <PubSubClient.h>
#include <ESP8266WiFi.h>

// HiveMQ public broker details
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic = "mrhasan/heart"; // Unique topic for your project

extern int heartRate;
extern int signalValue;

WiFiClient espMqttClient;
PubSubClient mqttClient(espMqttClient);

void mqttReconnect() {
  while (!mqttClient.connected()) {
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("[MQTT] Connected to HiveMQ");
    } else {
      Serial.print("[MQTT] Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
  }
}

void mqttSetup() {
  mqttClient.setServer(mqtt_server, mqtt_port);
}

void mqttLoopAndPublish() {
  if (!mqttClient.connected()) {
    mqttReconnect();
  }
  mqttClient.loop();
  static unsigned long lastMqtt = 0;
  if (millis() - lastMqtt > 1000) {
    lastMqtt = millis();
    char payload[64];
    snprintf(payload, sizeof(payload), "{\"bpm\":%d,\"signal\":%d}", heartRate, signalValue);
    mqttClient.publish(mqtt_topic, payload);
    Serial.print("[MQTT] Published: ");
    Serial.println(payload);
  }
}

#endif // MQTT_PUBLISH_H
