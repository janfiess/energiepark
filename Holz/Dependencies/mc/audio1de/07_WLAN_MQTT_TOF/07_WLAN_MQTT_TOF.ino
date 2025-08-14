/*****************************************************************
 * Beispiel: WLAN + MQTT + TOF-Schalter: Publish / Receive
 * Read TOF sensor (TOF050 VL6180X) via I2C and publish to MQTT broker whether an object is close to the Reader or not.
 * In der Konsole wird nur geprintet, wann die "Session" beginnt und wann sie endet.
 * Für etwas Toleranz wird der Wert mit den letzten Werten verglichen. Um Fehlimpulse zu vermeiden, wird die Console erst beschrieben,
 * sobald die aktuelle Messung mit den letzten paar Messungen übereinstimmt.
 *
 * turn on I2C mode by switching physical switches on the PN532 to 1 / 0 (I2C)
 * Anschluss:
 * PN532: SDA <-> ESP32-C6: GPIO 6
 * PN532: SCL <-> ESP32-C6: GPIO 7
 * PN532: Vcc <-> ESP32-C6: 3.3V
 * PN532: GND <-> ESP32-C6: GND
 *
 * Installiere Library "Adafruit_PN532" von Adafruit
 * Installiere Library "MQTT" von Joel Gaehwiler
 * Installiere Adafruit_VL6180X Library von Adafruit.
******************************************************************/


#include <WiFi.h>
#include <MQTT.h>
#include <Wire.h>                                                      // I2C
#include "Adafruit_VL6180X.h"


// WLAN und MQTT Einstellungen
const char* ssid = "FRITZ!Box 6690 TA";                                // @todo: add your wifi name "FRITZ!Box 6690 TA"
const char* pass = "79854308499311013585";                             // @todo: add your wifi pw, "79854308499311013585"
const char* mqtt_broker = "192.168.178.49";                            // "broker.emqx.io", "192.168.0.80"
const char* mqtt_client_id = "headphone_station_audio1de";

const char* MQTT_PUBLISH_TOPIC_AUDIO = "holz/player/audio1de";         // Topics.  -   Diese werden für Subscribing und Publishing genutzt
const char* MQTT_SUBSCRIBE_TOPIC_LED = "device_audio1de/ledring";
const char* subscribe_topics[] = { MQTT_SUBSCRIBE_TOPIC_LED };         // Array der zu abonnierenden Topics (hier nur eines, aber erweiterbar)

WiFiClient wificlient;
MQTTClient mqttclient;




// I2C Pins definieren
#define SDA_PIN 6
#define SCL_PIN 7



// TOF Distanzsensor

Adafruit_VL6180X tof = Adafruit_VL6180X();
int tof_maxdistance = 20;                                                // 20 ~ 30mm -> Sensor misst ab ca 10mm

// --- Globale Variablen für die Zustandsverwaltung ---
bool tof_objectDetected_temp = false;                                    // Aktueller Zustand (ohne Entprellung)
bool tof_objectDetected = false;                                         // Endgültiger Zustand (nach Entprellung) -> Wert gilt als endgültig, sobald mehrmals derselbe Wert erkannt wurde

// Array zum Speichern der letzten Zustände für die Entprellung
const int tof_toleranz_entprellung = 5;
bool tof_prevStates[tof_toleranz_entprellung]; 
int tof_state_index = 0;                                                 // Index für das Array
int tof_timestamp_prev_detected = 0;











// Funktionsprototypen
void connectWiFi();
void connectMQTT();
void mqtt_messageReceived(String& topic, String& payload);


void setup() {
  Serial.begin(115200);
  delay(1000);                                                           // warten, sonst werden die Serial.print in setup() nicht auf die Komsole gedruckt
  Serial.println("Kopfhörer-Station startet...");

  // WLAN verbinden
  connectWiFi();

  // --- MQTT Client initialisieren und Callback setzen ---
  // Diese Zeilen müssen hier stehen und nicht in connectMQTT()
  mqttclient.begin(mqtt_broker, wificlient);
  mqttclient.onMessage(mqtt_messageReceived);
  connectMQTT();

  // I2C
  Wire.begin(SDA_PIN, SCL_PIN);                                         // Wire starten mit den benutzerdefinierten I2C Pins



  // TOF Sensor
  if (! tof.begin()) {
    Serial.println("Failed to find TOF sensor");
    while (1);
  }
  Serial.println("TOF Sensor found!");


  for(int i=0; i < tof_toleranz_entprellung; i++){
    tof_prevStates[i] = false;
  }






  Serial.println("Setup abgeschlossen.");
}

void loop() {
  // MQTT
  mqttclient.loop();                                                    // MQTT Loop regelmässig aufrufen, um die Verbindung am Leben zu halten und Nachrichten zu empfangen  
  if (!mqttclient.connected()) {                                        // Überprüfen, ob die MQTT-Verbindung aktiv ist. Wenn nicht, versuchen wir uns neu zu verbinden.
    Serial.println("MQTT Verbindung verloren. Versuche Neuverbindung...");
    connectMQTT();
  }


  

  // TOF

  uint8_t tof_range = tof.readRange();
  uint8_t tof_status = tof.readRangeStatus();

  if (tof_status == VL6180X_ERROR_NONE) {


    // 1.: Aktuellen Zustand (Objekt erkannt oder nicht) bestimmen -> Wird ein Objekt erkannt innerhalb des festgelegten Bereichs?
          
    if (tof_range < tof_maxdistance) {
      tof_objectDetected_temp = true;
      // Serial.println("Object detected");
    } else {
      tof_objectDetected_temp = false;
    }


    // 2.: Zustand für Entprellung speichern -> Mehrere Werte aufnehmen und miteinander vergleichen

    tof_prevStates[tof_state_index] = tof_objectDetected_temp;
    tof_state_index = (tof_state_index + 1) % tof_toleranz_entprellung;    // Index zyklisch erhöhen (0, 1, 2, 3, 4, 0, ...)

    // 3. Entprellte Logik prüfen und Ausgabe steuern
    bool tof_all_detected = true;
    bool tof_none_detected = true;

    for (int i = 0; i < tof_toleranz_entprellung; i++) {
      if (!tof_prevStates[i]) {
        tof_all_detected = false;
      }
      if (tof_prevStates[i]) {
        tof_none_detected = false;
      }
    }

    if (tof_all_detected && !tof_objectDetected) {                          // Wenn alle der letzten Werte gleich sind und der endgültige Zustand sich ändert
      tof_objectDetected = true;
      // Serial.println("Objekt in der Nähe");

      // Signal versenden per MQTT
      String publishPayload = "pause";
      Serial.printf("Publishing: Topic: %s, Payload: %s\n", MQTT_PUBLISH_TOPIC_AUDIO, publishPayload);
      mqttclient.publish(MQTT_PUBLISH_TOPIC_AUDIO, publishPayload);



    } else if (tof_none_detected && tof_objectDetected) {
      tof_objectDetected = false;
      // Serial.println("Kein Objekt in der Nähe");

       // Signal versenden per MQTT
      String publishPayload = "play";
      Serial.printf("Publishing: Topic: %s, Payload: %s\n", MQTT_PUBLISH_TOPIC_AUDIO, publishPayload);
      mqttclient.publish(MQTT_PUBLISH_TOPIC_AUDIO, publishPayload);
    }
  }

  delay(100);                                                             // kurze Pause, entlastet den Prozessor
}

// --- WLAN Funktionen ---

void connectWiFi() {
  Serial.printf("Verbinde mit WLAN %s", ssid);                            // ssid ist const char*, kein String(ssid) nötig
  WiFi.begin(ssid, pass);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {                // Max 20 Versuche (10 Sekunden)
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

void connectMQTT() {                                                      // Diese Funktion kümmert sich nur noch um den eigentlichen Verbindungsaufbau und das Abonnement.
  Serial.printf("Verbinde mit MQTT Broker: %s\n", mqtt_broker);

  // Versuche, eine Verbindung herzustellen, bis sie erfolgreich ist
  while (!mqttclient.connect(mqtt_client_id)) {                           // Optional: Mit Benutzername/Passwort: mqttclient.connect(mqtt_client_id, "username", "password"). ->  -> username und passwort sind oft jeweils "public"
    Serial.println("Fehler beim Verbinden mit MQTT-Broker. Neuer Versuch in 2 Sekunden...");
    delay(2000);
  }

  Serial.println("Verbunden mit MQTT-Broker");

  int numTopics = sizeof(subscribe_topics) / sizeof(subscribe_topics[0]);
  for (int i = 0; i < numTopics; i++) {
    mqttclient.subscribe(subscribe_topics[i]);
    Serial.printf("Abonniert Topic: %s\n", subscribe_topics[i]);          // Bestätigung des Abonnements
  }
}


// --- MQTT Callback für eingehende Nachrichten ---
void mqtt_messageReceived(String& topic, String& payload) {
  Serial.println("MQTT Nachricht empfangen:");
  Serial.printf("  Topic: %s, Payload: %s\n", topic.c_str(), payload.c_str());  // .c_str() verwenden!

  if (topic.equals(MQTT_SUBSCRIBE_TOPIC_LED)) {
    int numLedsToLight = payload.toInt();
    Serial.printf("Empfangen: %d LEDs\n", numLedsToLight);                // %d für Integer
  }
}