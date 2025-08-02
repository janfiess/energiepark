/*****************************************************************
 * Beispiel WLAN + MQTT Public / Receive
 * ESP32 needed
 * Installiere Library "MQTT" von Joel Gaehwiler
******************************************************************/

/******************************************************************
 * Read NFC tags and RFID cards (I2C mode)
 * Script erkennt, wenn ein NFC Tag in die Nähe kommt bzw entfernt wird
 * und sendet das Signal per MQTT über WLAN an den MQTT Broker, wo TouchDesigner abonniert ist
 * turn on I2C mode by switching physical switches on the PN532 to 1 / 0 (I2C)
 * Anschluss:
 * PN532: SDA <-> ESP32-C6: GPIO 6
 * PN532: SCL <-> ESP32-C6: GPIO 7
 * PN532: Vcc <-> ESP32-C6: 3.3V
 * PN532: GND <-> ESP32-C6: GND
 * Installiere Library "Adafruit_PN532" von Adafruit
********************************************************************/


#include <WiFi.h>
#include <MQTT.h>
#include <Wire.h>                               // für I2C -> z.B. PN532 NFC Reader
#include <Adafruit_PN532.h>                     // NFC Reader

// WLAN und MQTT Einstellungen
const char* ssid = "FRITZ!Box 6690 TA";         // @todo: add your wifi name "FRITZ!Box 6690 TA"
const char* pass = "79854308499311013585";      // @todo: add your wifi pw, "79854308499311013585"
const char* mqtt_broker = "192.168.178.49";     // "broker.emqx.io", "192.168.0.80"     
const char* mqtt_client_id = "headphone_station_audio1de";

// Topics.  -   Diese werden für Subscribing und Publishing genutzt
const char* PUBLISH_TOPIC_AUDIO = "holz/player/audio1de";
const char* SUBSCRIBE_TOPIC_LED = "device_audio1de/ledring";

// Array der zu abonnierenden Topics (hier nur eines, aber erweiterbar)
const char* subscribe_topics[] = {SUBSCRIBE_TOPIC_LED};

WiFiClient wificlient;
MQTTClient mqttclient;

// I2C Pins definieren
#define SDA_PIN 6
#define SCL_PIN 7

long prev_timestamp = 0;    // alle paar sekunden message verschicken

// IRQ und RESET Pins definieren – werden vom PN532-Modul NICHT verwendet bei I2C, aber Bibliothek erwartet sie
#define PN532_IRQ   (2)
#define PN532_RESET (3)

// Konstruktor mit IRQ, RESET und Wire
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET, &Wire);

// Status merken, ob aktuell ein NFC-Tag erkannt wird
bool tagDetected = false;

// Funktionsprototypen
void connectWiFi();
void connectMQTT();
void messageReceived(String &topic, String &payload);
void sendMQTT();


void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println("Kopfhörer-Station startet...");

  // WLAN verbinden
  connectWiFi();

  // --- MQTT Client initialisieren und Callback setzen ---
  // Diese Zeilen müssen hier stehen und nicht in connectMQTT()
  mqttclient.begin(mqtt_broker, wificlient);
  mqttclient.onMessage(messageReceived); 
  connectMQTT();

  
  Wire.begin(SDA_PIN, SCL_PIN);                        // Wire starten mit den benutzerdefinierten I2C Pins
  nfc.begin();                                         // PN532 starten

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Kein PN532 gefunden – Verbindung prüfen.");
    while (1); // bleibt hängen, wenn nichts gefunden wird
  }

  // Chip-Daten des NFC-Readers anzeigen
  Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware Version: "); Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);

  // Konfiguriere das Modul für RFID-Lesen
  nfc.SAMConfig();
  Serial.println("Warte auf ein RFID/NFC Tag...");


  Serial.println("Setup abgeschlossen.");
}

void loop() {
  mqttclient.loop();                           // MQTT Loop regelmässig aufrufen, um die Verbindung am Leben zu halten und Nachrichten zu empfangen

  // Überprüfen, ob die MQTT-Verbindung aktiv ist. Wenn nicht, versuchen wir uns neu zu verbinden.
  if (!mqttclient.connected()) {
    Serial.println("MQTT Verbindung verloren. Versuche Neuverbindung...");
    connectMQTT();
  }





  // NFC Reader

  uint8_t uid[7];
  uint8_t uidLength;

  // Tag auslesen (mit Timeout)
  bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100);

  if (success) {
    if (!tagDetected) {
      tagDetected = true;


      for (uint8_t i = 0; i < uidLength; i++) {
        Serial.print(uid[i], HEX); Serial.print(" ");
      }
      Serial.println();

      char* publish_payload_audio = "pause";
      Serial.printf("Kopfhörer eingehängt - Publishing: Topic: %s, Payload: %s\n", PUBLISH_TOPIC_AUDIO, publish_payload_audio);
      mqttclient.publish(PUBLISH_TOPIC_AUDIO, publish_payload_audio);



      
    }
  } else {
    if (tagDetected) {
      tagDetected = false;
      char* publish_payload_audio = "play";
      Serial.printf("Kopfhörer entfernt - Publishing: Topic: %s, Payload: %s\n", PUBLISH_TOPIC_AUDIO, publish_payload_audio);
      mqttclient.publish(PUBLISH_TOPIC_AUDIO, publish_payload_audio);
    }
  }

  delay(200); // Abfrageintervall





  
  // // Sende Nachricht alle 10 Sekunden
  // if(millis() - prev_timestamp >= 10000) { 
  //   prev_timestamp = millis();
  //   Serial.println("Sending MQTT message...");
  //   sendMQTT();
  // }
  
  delay(50);                                           // Eine kleine Verzögerung ist gut, um den Core nicht zu überlasten
}

// --- WLAN Funktionen ---

void connectWiFi() {
  Serial.printf("Verbinde mit WLAN %s", ssid); // ssid ist const char*, kein String(ssid) nötig
  WiFi.begin(ssid, pass);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) { // Max 20 Versuche (10 Sekunden)
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

void connectMQTT() {                                                              // Diese Funktion kümmert sich nur noch um den eigentlichen Verbindungsaufbau und das Abonnement.
  Serial.printf("Verbinde mit MQTT Broker: %s\n", mqtt_broker);

  // Versuche, eine Verbindung herzustellen, bis sie erfolgreich ist
  while (!mqttclient.connect(mqtt_client_id)) {                                   // Optional: Mit Benutzername/Passwort: mqttclient.connect(mqtt_client_id, "username", "password"). ->  -> username und passwort sind oft jeweils "public"
    Serial.println("Fehler beim Verbinden mit MQTT-Broker. Neuer Versuch in 2 Sekunden...");
    delay(2000);
  }

  Serial.println("Verbunden mit MQTT-Broker");

  int numTopics = sizeof(subscribe_topics) / sizeof(subscribe_topics[0]);
  for (int i = 0; i < numTopics; i++) {
    mqttclient.subscribe(subscribe_topics[i]);
    Serial.printf("Abonniert Topic: %s\n", subscribe_topics[i]);                  // Bestätigung des Abonnements
  }
}

void sendMQTT() {
  const char* publishTopic = PUBLISH_TOPIC_AUDIO;
  const char* publishPayload = "huhu";            

  Serial.printf("Publishing: Topic: %s, Payload: %s\n", publishTopic, publishPayload);
  mqttclient.publish(publishTopic, publishPayload);
}

// --- MQTT Callback für eingehende Nachrichten ---
void messageReceived(String &topic, String &payload) {
  Serial.println("MQTT Nachricht empfangen:");
  Serial.printf("  Topic: %s, Payload: %s\n", topic.c_str(), payload.c_str());    // .c_str() verwenden!

  if (topic.equals(SUBSCRIBE_TOPIC_LED)) { 
    int numLedsToLight = payload.toInt();
    Serial.printf("Empfangen: %d LEDs\n", numLedsToLight);                        // %d für Integer
  }
}