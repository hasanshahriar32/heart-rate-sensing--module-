#ifndef MQTT_PUBLISH_H
#define MQTT_PUBLISH_H

#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

// HiveMQ Cloud broker details (update username/password below)
const char* mqtt_server = "38f07a1ee3754972a26af0f040402fde.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_topic = "mrhasan/heart"; // Unique topic for your project
const char *mqtt_username = "Paradox";    // <-- Set your HiveMQ Cloud username
const char *mqtt_password = "Paradox1";    // <-- Set your HiveMQ Cloud password

extern int heartRate;
extern int signalValue;

WiFiClientSecure espMqttClient;
PubSubClient mqttClient(espMqttClient);

void mqttReconnect() {
  while (!mqttClient.connected()) {
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("[MQTT] Connected to HiveMQ Cloud");
    } else {
      Serial.print("[MQTT] Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
  }
}

void mqttSetup() {
  espMqttClient.setInsecure(); // For testing only. For production, use a proper root CA cert.
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
    char payload[256];
    // Generate ISO8601 timestamp (placeholder, replace with RTC or NTP if available)
    char timestamp[32];
    snprintf(timestamp, sizeof(timestamp), "2025-01-28T10:43:51.123Z"); // TODO: Replace with real time if available
    snprintf(payload, sizeof(payload),
             "{\"userId\":\"BW8NUP21AWMkI0xrrI2nxBP6Xd92\",\"dataType\":\"heartRate\",\"bpm\":%d,\"signal\":%d,\"timestamp\":\"%s\",\"deviceId\":\"ESP8266_001\"}",
             heartRate, signalValue, timestamp);
    mqttClient.publish(mqtt_topic, payload);
    Serial.print("[MQTT] Published: ");
    Serial.println(payload);
  }
}

#endif // MQTT_PUBLISH_H
