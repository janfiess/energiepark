/*****************************************************************
 * Beispiel WLAN + MQTT Public / Receive
 * ESP32 needed
 * Installiere Library "MQTT" von Joel Gaehwiler
******************************************************************/


#include <WiFi.h>
#include <MQTT.h>

// WLAN und MQTT Einstellungen
const char* ssid = "FRITZ!Box 6690 TA";      // @todo: add your wifi name "FRITZ!Box 6690 TA"
const char* pass = "79854308499311013585";   // @todo: add your wifi pw, "79854308499311013585"
const char* mqtt_broker = "192.168.178.49";  // "broker.emqx.io", "192.168.0.80"
const char* mqtt_client_id = "headphone_station_audio1de";

// Topics.  -   Diese werden für Subscribing und Publishing genutzt
const char* MQTT_PUBLISH_TOPIC_AUDIO = "holz/player/audio1de";
const char* MQTT_SUBSCRIBE_TOPIC_LED = "device_audio1de/ledring";

// Array der zu abonnierenden Topics (hier nur eines, aber erweiterbar)
const char* subscribe_topics[] = { MQTT_SUBSCRIBE_TOPIC_LED };

WiFiClient wificlient;
MQTTClient mqttclient;

// Funktionsprototypen
void connectWiFi();
void connectMQTT();
void mqtt_messageReceived(String& topic, String& payload);
void sendMQTT();

long prev_timestamp = 0;  // alle paar sekunden message verschicken

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println("Kopfhörer-Station startet...");

  // WLAN verbinden
  connectWiFi();

  // --- MQTT Client initialisieren und Callback setzen ---
  // Diese Zeilen müssen hier stehen und nicht in connectMQTT()
  mqttclient.begin(mqtt_broker, wificlient);
  mqttclient.onMessage(mqtt_messageReceived);
  connectMQTT();

  Serial.println("Setup abgeschlossen.");
}

void loop() {
  mqttclient.loop();  // MQTT Loop regelmässig aufrufen, um die Verbindung am Leben zu halten und Nachrichten zu empfangen

  // Überprüfen, ob die MQTT-Verbindung aktiv ist. Wenn nicht, versuchen wir uns neu zu verbinden.
  if (!mqttclient.connected()) {
    Serial.println("MQTT Verbindung verloren. Versuche Neuverbindung...");
    connectMQTT();
  }

  // Sende Nachricht alle 10 Sekunden
  if (millis() - prev_timestamp >= 10000) {
    prev_timestamp = millis();
    Serial.println("Sending MQTT message...");
    sendMQTT();
  }

  delay(50);  // Eine kleine Verzögerung ist gut, um den Core nicht zu überlasten
}

// --- WLAN Funktionen ---

void connectWiFi() {
  Serial.printf("Verbinde mit WLAN %s", ssid);  // ssid ist const char*, kein String(ssid) nötig
  WiFi.begin(ssid, pass);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {  // Max 20 Versuche (10 Sekunden)
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nWiFi verbunden: SSID: %s, IP-Adresse: %s\n", ssid, WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\nWiFi Verbindung fehlgeschlagen!");
  }
}

// --- MQTT Funktionen ---

void connectMQTT() {  // Diese Funktion kümmert sich nur noch um den eigentlichen Verbindungsaufbau und das Abonnement.
  Serial.printf("Verbinde mit MQTT Broker: %s\n", mqtt_broker);

  // Versuche, eine Verbindung herzustellen, bis sie erfolgreich ist
  while (!mqttclient.connect(mqtt_client_id)) {  // Optional: Mit Benutzername/Passwort: mqttclient.connect(mqtt_client_id, "username", "password"). ->  -> username und passwort sind oft jeweils "public"
    Serial.println("Fehler beim Verbinden mit MQTT-Broker. Neuer Versuch in 2 Sekunden...");
    delay(2000);
  }

  Serial.println("Verbunden mit MQTT-Broker");

  int numTopics = sizeof(subscribe_topics) / sizeof(subscribe_topics[0]);
  for (int i = 0; i < numTopics; i++) {
    mqttclient.subscribe(subscribe_topics[i]);
    Serial.printf("Abonniert Topic: %s\n", subscribe_topics[i]);  // Bestätigung des Abonnements
  }
}

void sendMQTT() {
  const char* publishTopic = MQTT_PUBLISH_TOPIC_AUDIO;
  const char* publishPayload = "huhu";

  Serial.printf("Publishing: Topic: %s, Payload: %s\n", publishTopic, publishPayload);
  mqttclient.publish(publishTopic, publishPayload);
}

// --- MQTT Callback für eingehende Nachrichten ---
void mqtt_messageReceived(String& topic, String& payload) {
  Serial.println("MQTT Nachricht empfangen:");
  Serial.printf("  Topic: %s, Payload: %s\n", topic.c_str(), payload.c_str());  // .c_str() verwenden!

  if (topic.equals(MQTT_SUBSCRIBE_TOPIC_LED)) {
    int numLedsToLight = payload.toInt();
    Serial.printf("Empfangen: %d LEDs\n", numLedsToLight);  // %d für Integer
  }
}