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
#include <Adafruit_PN532.h>                                            // NFC Reader
#include "esp_log.h" 


// WLAN und MQTT Einstellungen
const char* ssid = "Energiepark Technik";                                // @todo: add your wifi name "FRITZ!Box 6690 TA"
// const char* ssid = "LinusFetzMusikGast";                                // @todo: add your wifi name "FRITZ!Box 6690 TA"
const char* pass = "Energiepark2025.8";                             // @todo: add your wifi pw, "79854308499311013585"
// const char* pass = "linusfetzgast";                             // @todo: add your wifi pw, "79854308499311013585"
const char* mqtt_broker = "192.168.178.24";                            // "broker.emqx.io", "192.168.0.80"
// const char* mqtt_broker = "broker.hivemq.com";                            // "broker.emqx.io", "192.168.0.80", "test.mosquitto.org", "broker.hivemq.com"
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
int tof_maxdistance = 30;                                                // 30 ~ 40mm -> Sensor misst ab ca 10mm

// --- Globale Variablen für die Zustandsverwaltung ---
bool tof_objectDetected_temp = false;                                    // Aktueller Zustand (ohne Entprellung)
bool tof_objectDetected = false;                                         // Endgültiger Zustand (nach Entprellung) -> Wert gilt als endgültig, sobald mehrmals derselbe Wert erkannt wurde

// Array zum Speichern der letzten Zustände für die Entprellung
const int tof_toleranz_entprellung = 5;
bool tof_prevStates[tof_toleranz_entprellung]; 
int tof_state_index = 0;                                                 // Index für das Array
int tof_timestamp_prev_detected = 0;



// NFC Reader

// IRQ und RESET Pins definieren – werden vom PN532-Modul NICHT verwendet bei I2C, aber Bibliothek erwartet sie
#define PN532_IRQ   (2)
#define PN532_RESET (3)

// Konstruktor mit IRQ, RESET und Wire
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET, &Wire);

// Ziel-UID definieren (7 Bytes)
// const uint8_t TARGET_UID[] = { 0xFF, 0x0F, 0x17, 0xE9, 0x3F, 0x00, 0x00 };
String target_id = "14AA77D6";
const uint8_t NFC_TARGET_UID_LENGTH = 7;

// --- Statusvariablen für die verbesserte Erkennung ---
bool nfc_tagCurrentlyPresent = false;                                  // Merkt sich, ob der TARGET_UID-Tag aktuell als "anwesend" gilt // Wert ist bereits entprellt
int nfc_presentConfirmCounter = 0;                                     // Zählt aufeinanderfolgende erfolgreiche Lesungen des TARGET_UID
int nfc_absentConfirmCounter = 0;                                      // Zählt aufeinanderfolgende fehlgeschlagene Lesungen (Tag nicht da)

const int NFC_CONFIRM_PRESENT_THRESHOLD = 3;                           // Der Tag muss X-mal hintereinander korrekt gelesen werden, um als "anwesend" zu gelten.
const int NFC_CONFIRM_ABSENT_THRESHOLD = 5;                            // Der Tag muss X-mal hintereinander NICHT gelesen werden, um als "entfernt" zu gelten.

uint8_t nfc_uid[7];                                                    // Buffer zum Speichern der UID
uint8_t nfc_uidLength;                                                 // Länge der UID
bool nfc_tag_erkannt;                                                  // in dieser Variable wird gespeichert, ob irgendein NFC Tag erkannt wurde
bool nfc_isTargetTag;                                                  // handelt es sich bei dem erkannten NFC Tag wirklich um den gewünschten? 




// algemein

bool is_object_detected;                                               // diese Variable is nur dann true oder false, wenn alle Sensoren den Wert true bzw. false ausgeben
bool prev_is_object_detected;



// Funktionsprototypen
void connectWiFi();
void connectMQTT();
void mqtt_messageReceived(String& topic, String& payload);
void init_tof();
void read_tof();
void init_nfc();
void read_nfc();


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

  // Harte Methode: ESP32-I2C-Logger komplett ausschalten, sonst wird Konsole zugemüllt
  esp_log_set_vprintf([](const char *fmt, va_list args) -> int {
    (void)fmt;   // ignore
    (void)args;  // ignore
    return 0;    // nichts ausgeben
  });



  Wire.begin(SDA_PIN, SCL_PIN);                                         // Wire starten mit den benutzerdefinierten I2C Pins


  init_tof();                                                           // TOF Sensor
  init_nfc();                                                           // NFC Sensor

  Serial.println("Setup abgeschlossen.");
}

void loop() {
  // MQTT
  mqttclient.loop();                                                    // MQTT Loop regelmässig aufrufen, um die Verbindung am Leben zu halten und Nachrichten zu empfangen  
  if (!mqttclient.connected()) {                                        // Überprüfen, ob die MQTT-Verbindung aktiv ist. Wenn nicht, versuchen wir uns neu zu verbinden.
    Serial.println("MQTT Verbindung verloren. Versuche Neuverbindung...");
    connectMQTT();
  }

  read_nfc();                                                            // get NFC "button" state

  // delay(200);


  read_tof();                                                            // get TOF "button" state
  // return;








  if(nfc_tagCurrentlyPresent == true && tof_objectDetected == true){
    is_object_detected = true;
  }
  else if(nfc_tagCurrentlyPresent == false && tof_objectDetected == false){
    is_object_detected = false;
  }
  else return;


  if(is_object_detected == prev_is_object_detected) return;
  if(is_object_detected == true){
    prev_is_object_detected = true;

    String publishPayload = "pause";
    Serial.printf("--> Publishing: Topic: %s, Payload: %s\n", MQTT_PUBLISH_TOPIC_AUDIO, publishPayload);
    mqttclient.publish(MQTT_PUBLISH_TOPIC_AUDIO, publishPayload);
  }
  else if(is_object_detected == false){
    prev_is_object_detected = false;

    String publishPayload = "play";
    Serial.printf("--> Publishing: Topic: %s, Payload: %s\n", MQTT_PUBLISH_TOPIC_AUDIO, publishPayload);
    mqttclient.publish(MQTT_PUBLISH_TOPIC_AUDIO, publishPayload);
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


void init_tof(){                                                          // wird in start() aufgerufen
  if (! tof.begin()) {
    Serial.println("Failed to find TOF sensor");
    while (1);
  }
  Serial.println("TOF Sensor found!");


  for(int i=0; i < tof_toleranz_entprellung; i++){
    tof_prevStates[i] = false;
  }
}




void read_tof(){
  uint8_t tof_range = tof.readRange();
  uint8_t tof_status = tof.readRangeStatus();

  // Serial.printf("tof_range: %d\n", tof_range);

  // if (tof_status == VL6180X_ERROR_NONE) {


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
      Serial.printf("Objekt in der Nähe (Distanz: %d)\n", tof_range);

      // Signal versenden per MQTT
      // String publishPayload = "pause";
      // Serial.printf("Publishing (TOF only): Topic: %s, Payload: %s\n", MQTT_PUBLISH_TOPIC_AUDIO, publishPayload);
      // mqttclient.publish(MQTT_PUBLISH_TOPIC_AUDIO, publishPayload);



    } else if (tof_none_detected && tof_objectDetected) {
      tof_objectDetected = false;
      Serial.printf("kein Objekt in der Nähe (Distanz: %d)\n", tof_range);

       // Signal versenden per MQTT
      // String publishPayload = "play";
      // Serial.printf("Publishing (TOF only): Topic: %s, Payload: %s\n", MQTT_PUBLISH_TOPIC_AUDIO, publishPayload);
      // mqttclient.publish(MQTT_PUBLISH_TOPIC_AUDIO, publishPayload);
    }
  // }
}







void init_nfc(){                                                          // wird in start() aufgerufen
  nfc.begin();                                                          // NFC Reader PN532 starten
  uint32_t nfc_versiondata = nfc.getFirmwareVersion();                  // Firmware-Version abfragen
  if (!nfc_versiondata) {
    Serial.println("Kein PN532 gefunden – Verbindung prüfen.");
    while (1);                                                          // bleibt hängen, wenn nichts gefunden wird
  }

  // Chip-Daten anzeigen
  Serial.printf("Found chip PN5%X\n", (nfc_versiondata >> 24) & 0xFF);
  Serial.printf("Firmware Version: %d.%d\n", (nfc_versiondata >> 16) & 0xFF, (nfc_versiondata >> 8) & 0xFF);

  // Konfiguriere das Modul für RFID-Lesen
  nfc.SAMConfig();
  Serial.println("Warte auf ein RFID/NFC Tag...");
}



void read_nfc(){
  // NFC
  nfc_tag_erkannt = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, nfc_uid, &nfc_uidLength, 100);   // Versuche für 100ms, einen Tag zu erkennen. Der Timeout sollte nicht zu kurz sein, da der PN532 Zeit benötigt..
  nfc_isTargetTag = false;  

  




  if (nfc_tag_erkannt) {  
    // UID in Hex-String konvertieren
    char nfc_payload[3 * sizeof(nfc_uid) + 1]; 
    nfc_payload[0] = '\0'; 

    for (uint8_t i = 0; i < nfc_uidLength; i++) {
      char byteStr[4];
      sprintf(byteStr, "%02X", nfc_uid[i]);      
      strcat(nfc_payload, byteStr);              
    }
    String current_NFC_string = nfc_payload; 
    // Serial.println(current_NFC_string);

    if(current_NFC_string == target_id){
      nfc_isTargetTag = true;
      // Serial.println("der gezeigte ID ist der Gesuchte");
    }
  }




  // --- Logik für die Bestätigung von Anwesenheit/Abwesenheit ---
  if (nfc_isTargetTag) {
    nfc_absentConfirmCounter = 0;                                        // Reset des Abwesenheitszählers
    nfc_presentConfirmCounter++;                                         // Inkrementiere Anwesenheitszähler

    if (nfc_presentConfirmCounter >= NFC_CONFIRM_PRESENT_THRESHOLD) {    // Wenn der Tag oft genug hintereinander bestätigt wurde   
      nfc_presentConfirmCounter = NFC_CONFIRM_PRESENT_THRESHOLD;         // Zähler auf Max setzen, um Überlauf zu vermeiden
      if (!nfc_tagCurrentlyPresent) {  

        Serial.printf("Ziel-Tag erkannt: %s\n", target_id);              // current_NFC_string == target_id 

        // Signal versenden per MQTT
        // String publishPayload = "pause (nfc)";
        // Serial.printf("Publishing (NFC only): Topic: %s, Payload: %s\n", MQTT_PUBLISH_TOPIC_AUDIO, publishPayload);
        // mqttclient.publish(MQTT_PUBLISH_TOPIC_AUDIO, publishPayload);
        

        nfc_tagCurrentlyPresent = true; // Status auf "anwesend" setzen
      }
    }
  } else {                                                                // Wenn der Ziel-Tag NICHT gelesen wurde (entweder kein Tag oder falscher Tag)
    nfc_presentConfirmCounter = 0;                                        // Reset des Anwesenheitszählers
    nfc_absentConfirmCounter++;                                           // Inkrementiere Abwesenheitszähler

    if (nfc_absentConfirmCounter >= NFC_CONFIRM_ABSENT_THRESHOLD) {
      nfc_absentConfirmCounter = NFC_CONFIRM_ABSENT_THRESHOLD;            // Wenn der Tag oft genug hintereinander NICHT gelesen wurde -> Zähler auf Max setzen
      if (nfc_tagCurrentlyPresent) {                                      // Wenn der Tag vorher als anwesend galt, jetzt aber nicht mehr      
        Serial.println("Ziel-Tag entfernt (stabilisiert).");

        // Signal versenden per MQTT
        // String publishPayload = "play (nfc)";
        // Serial.printf("Publishing (NFC only): Topic: %s, Payload: %s\n", MQTT_PUBLISH_TOPIC_AUDIO, publishPayload);
        // mqttclient.publish(MQTT_PUBLISH_TOPIC_AUDIO, publishPayload);

        nfc_tagCurrentlyPresent = false;                                  // Status auf "nicht anwesend" setzen
      }
    }
  }
}