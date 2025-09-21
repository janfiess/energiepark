/*****************************************************************
 * Lese RFID / NFC Sensor (PN532) und teile TouchDesigner (über MQTT mit, ob ein Kopfhörer (mit RFID-Tags) in der Nähe ist
 * Der Code benachrichtigt nur, wann die "Session" (erkannt oder nicht erkannt) beginnt und wann sie endet.
 * Entprellung: Um Fehlimpulse zu vermeiden wird der Wert zunächst mit den letzten Werten verglichen. Wenn der Wert konstant bleibt, wird der Zustand als "geändert" anerkannt.
 * Wenn kein bekanntes Netzwerk gefunden wird, startet der ESP einen Access Point. Wenn man dieses WLAN Netzwerk am Computer / Smartphone auswählt,
 * kommt man per Captive WLAN auf eine Setup-Website (ein DNS-Server leitet alle Domains auf die ESP-IP 192.168.4.1 um). 
 * Dort können SSID und PW des WLAN-Netzwerks eingegeben werden, mit dem man sich verbinden mag.
 * Diese Setup-Page ist auch ohne AP-Mode über die IP Adresse des ESP im Browser verfügbar.
 * Die Daten aus dem Web-Formular werden im Flash Speicher des ESP gespeichert, damit er sich diese Info beim nächsten Start direkt abrufen kann 
 * und sich z. B. ohne Nachfrage mit einem Netzwerk verbinden kann.
 *
 * Anschluss:
 *
 * RFID/NFC: PN532, TOF: VL6180X ( * turn on I2C mode by switching physical switches on the PN532 to 1 / 0 (I2C))
 * RFID/NFC bzw. TOF: SDA <-> ESP32-C6: GPIO 6
 * RFID/NFC bzw. TOF: SCL <-> ESP32-C6: GPIO 7
 * RFID/NFC bzw. TOF: Vcc <-> ESP32-C6: 3.3V
 * RFID/NFC bzw. TOF: GND <-> ESP32-C6: GND

 * LED-Ring 
 * WS2812B-Ring: Data In  <->  ESP32-C6: GPIO 5
 * WS2812B-Ring: +5V/Vcc  <->  ESP32-C6: 5V
 * WS2812B-Ring: GND      <->  ESP32-C6: GND

 * Taster
 * Taster Pin 1   <->  ESP32-C6: 3.3V
 * Taster Pin 2   <->  ESP32-C6: GPIO 10
 *
 * Installiere folgende Libraries: 
 * -  "Adafruit_PN532" von Adafruit
 * -  "MQTT" von Joel Gaehwiler
 * -  "Adafruit_NeoPixel" von Adafruit
 *
******************************************************************/


#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <DNSServer.h>                                                 // für Captive Portal ("Hotel-WLAN-Start-Screen")
#include <MQTT.h>
#include <Wire.h>                                                      // I2C (so kommuniziert z. B. der NFC / RFID-Reader)
#include <Adafruit_PN532.h>                                            // NFC Reader
#include "esp_log.h"                                                   // nicht die Konsole voll schreiben, wenn es ab und zu einen I2C Nack gibt
#include <Adafruit_NeoPixel.h>                                         // LED-Ring


Preferences preferences;
String id_station;                              // id der Kopfhörerstation 1 - 6
int current_headphone_id;                       // 1 - 6


// Variablen für die Ziel-WLAN-Daten - wird auf der Setup-Website des ESP-Access Points eingegeben.
String targetSSID = "";                                                // "Energiepark Technik", "tinkergarden", "LinusFetzMusikGast"
String targetPassword = "";                                            // "Energiepark2025.8", "strenggeheim", "linusfetzgast"
WiFiClient wificlient;


// Server für Setup-Page
bool apMode = false;    
WebServer server(80);                                                  // Lokaler Webserver auf Port 80, um Setup-Seite im Browser bereitzustellen
DNSServer dnsServer;                                                   // für Captive Portal
const byte DNS_PORT = 53;                                              // für Captive Portal.  // DNS-Server: Alle Anfragen -> ESP32
  

// MQTT
String mqtt_broker = "";                                                          // "broker.emqx.io", "192.168.0.60", "test.mosquitto.org", "broker.hivemq.com"
String MQTT_PUBLISH_TOPIC_AUDIO_PRE = "holz/player/audio/";                       // Topics.  -   Diese werden für Subscribing und Publishing genutzt
String MQTT_PUBLISH_TOPIC_AUDIO;                                                  // je nach Kopfhörer kann die id wechseln, z. B.  "holz/ledring/numsections/1"
String MQTT_SUBSCRIBE_TOPIC_SECTIONS_LEDRING = "holz/ledring/numsections/#";      // Beispiel: "holz/ledring/numsections/1"   |  Payload: 0 - 12
String MQTT_SUBSCRIBE_TOPIC_TRACK_STATE = "holz/ledring/state/#";                 // message when audio track ended -> Payload: "leds_idle"
String subscribe_topics[10];                                                                       // Array der zu abonnierenden Topics (hier nur eines, aber erweiterbar)
MQTTClient mqttclient;


// I2C Pins definieren
#define SDA_PIN 6
#define SCL_PIN 7


// NFC Reader
// IRQ und RESET Pins definieren – werden vom PN532-Modul NICHT verwendet bei I2C, aber Bibliothek erwartet sie
#define PN532_IRQ   (2)
#define PN532_RESET (3)
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET, &Wire);                     // Konstruktor mit IRQ, RESET und Wire

String target_rfid_1[3];
String target_rfid_2[3];
String target_rfid_3[3];
String target_rfid_4[3];
String target_rfid_5[3];
String target_rfid_6[3];



const uint8_t NFC_TARGET_UID_LENGTH = 7;


// Statusvariablen für die verbesserte Erkennung                       // Merkt sich, ob der TARGET_UID-Tag aktuell als "anwesend" gilt // Wert ist bereits entprellt
int nfc_presentConfirmCounter = 0;                                     // Zählt aufeinanderfolgende erfolgreiche Lesungen des TARGET_UID
int nfc_absentConfirmCounter = 0;                                      // Zählt aufeinanderfolgende fehlgeschlagene Lesungen (Tag nicht da)

const int NFC_CONFIRM_PRESENT_THRESHOLD = 3;                           // Der Tag muss X-mal hintereinander korrekt gelesen werden, um als "anwesend" zu gelten.
const int NFC_CONFIRM_ABSENT_THRESHOLD = 5;                            // Der Tag muss X-mal hintereinander NICHT gelesen werden, um als "entfernt" zu gelten.

uint8_t nfc_uid[7];                                                    // Buffer zum Speichern der UID
uint8_t nfc_uidLength;                                                 // Länge der UID
bool nfc_tag_erkannt;                                                  // in dieser Variable wird gespeichert, ob irgendein NFC Tag erkannt wurde
bool nfc_isTargetTag;                                                  // handelt es sich bei dem erkannten NFC Tag wirklich um den gewünschten? 


// LED-Ring
#define LED_PIN 5
#define NUM_PIXELS 12
#define DELAYVAL 500
#define LED_BRIGHTNESS 50
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);


// Taster
#define TASTER_PIN 10
int buttonState = 0;         
int prev_buttonState = 0;


// allgemein
bool is_object_detected;                                               // diese Variable is nur dann true oder false, wenn alle Sensoren den Wert true bzw. false ausgeben
bool prev_is_object_detected;


// Funktionsprototypen
void connectWiFi();
void connectMQTT();
void start_webserver();
void start_access_point();
void load_setup_data();                                                // Gespeicherte Setup-Daten (Preferences -> persistent in Flash-Speicher) laden, falls bereits per Setup-Seite festgelegt
void mqtt_messageReceived(String& topic, String& payload);
void send_website_save();
void save_settings();
void send_website_settings();
void init_nfc();
void read_nfc();
void read_button();
void init_led_ring();











void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Kopfhörer-Station startet...");
  load_setup_data();                               // Gespeicherte Setup-Daten (Preferences -> persistent in Flash-Speicher) laden, falls bereits per Setup-Seite festgelegt
  connectWiFi();

  if (WiFi.status() == WL_CONNECTED) {             // MQTT Client initialisieren und Callback setzen 
    mqttclient.begin(mqtt_broker.c_str(), wificlient);
    connectMQTT();
    mqttclient.onMessage(mqtt_messageReceived);
  } 
  else start_access_point();                       // Falls keine Verbindung zu einem gespeicherten Netzwerk zustande kommt, starte den Access Point
    
  start_webserver();                               // Webserver starten, unabhängig von STA oder AP


  // I2C
  esp_log_set_vprintf([](const char *fmt, va_list args) -> int {
    (void)fmt;   // ignore
    (void)args;  // ignore
    return 0;    // nichts ausgeben
  });

  Wire.begin(SDA_PIN, SCL_PIN);   // Wire starten mit den benutzerdefinierten I2C Pins
  init_nfc();                     // NFC Sensor


  // Init LED-Ring
  init_led_ring();


  // Init Taster
  pinMode(TASTER_PIN, INPUT_PULLDOWN);
  Serial.println("Setup abgeschlossen."); 
}




void loop() {
  dnsServer.processNextRequest();                // @neu für Captive Portal
  server.handleClient();
 
  // Nur im STA-Modus reconnecten!
  if (!apMode && targetSSID != "" && WiFi.status() != WL_CONNECTED) {
    Serial.println("Versuche Verbindung mit gespeicherten Daten...");
    connectWiFi();
    delay(5000);
  }
  
  // MQTT-Verbindung bei Verlust wiederherstellen
  if (!apMode) {
    mqttclient.loop();                                                    // MQTT Loop regelmässig aufrufen, um die Verbindung am Leben zu halten und Nachrichten zu empfangen  
    if (!mqttclient.connected()) {                                        // Überprüfen, ob die MQTT-Verbindung aktiv ist. Wenn nicht, versuchen wir uns neu zu verbinden.
      Serial.println("MQTT Verbindung verloren. Versuche Neuverbindung...");
      connectMQTT();
    }
  }

  // RFID / NFC
  read_nfc();                                                             // get NFC "button" state


  if(is_object_detected != prev_is_object_detected) {
    prev_is_object_detected = is_object_detected;
    MQTT_PUBLISH_TOPIC_AUDIO = MQTT_PUBLISH_TOPIC_AUDIO_PRE + String(current_headphone_id);
    
    if(is_object_detected) {
      String publishPayload = "pause";
      Serial.printf("--> Publishing: Topic: %s, Payload: %s\n", MQTT_PUBLISH_TOPIC_AUDIO.c_str(), publishPayload);
      if(mqttclient.connected()) {
        mqttclient.publish(MQTT_PUBLISH_TOPIC_AUDIO.c_str(), publishPayload);
      }

      // LED Ring in Idle Mode
      strip.clear();
      for(int i=0; i<12; i++) {                                        // für jeden einzelnen Pixel - in der Schleife    
        strip.setPixelColor(i, strip.Color(20, 40, 0));                // Werte: 0 - 255
      }
      strip.show();                                                    // sende den aktualisierten Pixel an den LED-Ring
 
    } else {
      String publishPayload = "play";
      Serial.printf("--> Publishing: Topic: %s, Payload: %s\n", MQTT_PUBLISH_TOPIC_AUDIO.c_str(), publishPayload);
      if(mqttclient.connected()) {
        mqttclient.publish(MQTT_PUBLISH_TOPIC_AUDIO.c_str(), publishPayload);
      }
    }
  }
  
  read_button();                                                          // Alterative: Taster statt RFID-Erkennung
  delay(100);                                                             // kurze Pause, entlastet den Prozessor
}



/****************************************************************************************************
 * WLAN-Verbindung herstellen   // wird in setup gerufen und wenn Verbindung verloren geht
 ****************************************************************************************************/ 

void connectWiFi() {
  // Versuche, mit den gespeicherten Daten zu verbinden
  if (targetSSID != "") {
    Serial.printf("Versuche Verbindung mit gespeichertem WLAN: '%s' (PW: '%s')\n", targetSSID.c_str(), targetPassword.c_str());
    WiFi.begin(targetSSID.c_str(), targetPassword.c_str());
    unsigned long startAttemptTime = millis();

    // Versuche 10 Sekunden lang
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
      delay(500);
      Serial.print(".");
    }
    Serial.println();
  }

  // Überprüfe den WLAN-Status und handle entsprechend
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Erfolgreich verbunden: SSID: %s, IP-Adresse: %s \n", targetSSID.c_str(), WiFi.localIP().toString().c_str());
    apMode = false;
  }
}



/****************************************************************************************************
 * Gespeicherte Setup-Daten (Preferences -> persistent in Flash-Speicher) laden, falls bereits per Setup-Seite festgelegt
 ****************************************************************************************************/ 
  
void load_setup_data(){
  preferences.begin("wifi", true);
  targetSSID = preferences.getString("ssid", "");
  targetPassword = preferences.getString("password", "").c_str();
  mqtt_broker = preferences.getString("mqtt_broker", "").c_str();

  target_rfid_1[0] = preferences.getString("target_rfid_1a", "");
  target_rfid_1[1] = preferences.getString("target_rfid_1b", "");
  target_rfid_1[2] = preferences.getString("target_rfid_1c", "");

  target_rfid_2[0] = preferences.getString("target_rfid_2a", "");
  target_rfid_2[1] = preferences.getString("target_rfid_2b", "");
  target_rfid_2[2] = preferences.getString("target_rfid_2c", "");

  target_rfid_3[0] = preferences.getString("target_rfid_3a", "");
  target_rfid_3[1] = preferences.getString("target_rfid_3b", "");
  target_rfid_3[2] = preferences.getString("target_rfid_3c", "");

  target_rfid_4[0] = preferences.getString("target_rfid_4a", "");
  target_rfid_4[1] = preferences.getString("target_rfid_4b", "");
  target_rfid_4[2] = preferences.getString("target_rfid_4c", "");

  target_rfid_5[0] = preferences.getString("target_rfid_5a", "");
  target_rfid_5[1] = preferences.getString("target_rfid_5b", "");
  target_rfid_5[2] = preferences.getString("target_rfid_5c", "");

  target_rfid_6[0] = preferences.getString("target_rfid_6a", "");
  target_rfid_6[1] = preferences.getString("target_rfid_6b", "");
  target_rfid_6[2] = preferences.getString("target_rfid_6c", "");

  id_station = preferences.getString("id_station", "9");
  preferences.end();
  Serial.printf("Found in Flash: SSID: %s, Passwort: %s \n", targetSSID.c_str(), targetPassword.c_str());
}



/****************************************************************************************************
 * MQTT-Verbindung herstellen
 ****************************************************************************************************/ 

void connectMQTT() {                                                                        // Diese Funktion kümmert sich nur noch um den eigentlichen Verbindungsaufbau und das Abonnement.
  Serial.printf("Verbinde mit MQTT Broker: %s\n", mqtt_broker);
  int attempts = 0;
  // Versuche, eine Verbindung herzustellen, bis sie erfolgreich ist
  while (!mqttclient.connect(id_station.c_str())) {                                         // Optional: Mit Benutzername/Passwort: mqttclient.connect(mqtt_client_id, "username", "password"). ->  -> username und passwort sind oft jeweils "public"
    Serial.println("Fehler beim Verbinden mit MQTT-Broker. Neuer Versuch in 2 Sekunden...");
    attempts++;
    if(attempts > 10){
      Serial.println("\nMQTT Verbindung fehlgeschlagen! Neustart ...");
      ESP.restart();   // Neustart auslösen
    }
    delay(2000);
  }
  
  Serial.println("Verbunden mit MQTT-Broker");

  mqttclient.subscribe(MQTT_SUBSCRIBE_TOPIC_SECTIONS_LEDRING);
  Serial.printf("Abonniert Topic: %s\n", MQTT_SUBSCRIBE_TOPIC_SECTIONS_LEDRING.c_str());          // Bestätigung des Abonnements

  mqttclient.subscribe(MQTT_SUBSCRIBE_TOPIC_TRACK_STATE);
  Serial.printf("Abonniert Topic: %s\n", MQTT_SUBSCRIBE_TOPIC_TRACK_STATE.c_str());  
}



/****************************************
 * MQTT Callback für eingehende Nachrichten
 ****************************************/

void mqtt_messageReceived(String& topic, String& payload) {
  Serial.printf("<-- MQTT msg empfangen: Topic: %s, Payload: %s\n", topic.c_str(), payload.c_str());
  


  String id_from_incoming_topic = get_id_from_incoming_topic(topic);
  // Serial.printf("id_station: %s \n", id_station);
  if(id_from_incoming_topic != String(current_headphone_id)){
    Serial.println("Diese Nachricht betrifft nicht diese Station");
    return;
  } 



  if (topic.startsWith("holz/ledring/numsections")) {
    int numLedsToLight = payload.toInt();

    // LED-Ring bespielen
    strip.clear();
    for(int i=0; i<numLedsToLight; i++) {                   // für jeden einzelnen Pixel - in der Schleife
      strip.setPixelColor(i, strip.Color(128, 255, 0));     // Werte: 0 - 255
    }
    strip.show();                                           // sende den aktualisierten Pixel an den LED-Ring
  }






  if (topic.startsWith("holz/ledring/state")) {
    if(payload == "leds_idle"){
      strip.clear();
      for(int i=0; i<12; i++) {                             // für jeden einzelnen Pixel - in der Schleife    
        strip.setPixelColor(i, strip.Color(20, 40, 0));     // Werte: 0 - 255
      }
      strip.show();                                         // sende den aktualisierten Pixel an den LED-Ring
    }
  }
}




// called in mqtt_messageReceived()
String get_id_from_incoming_topic(String topic){
// Finde die Position des letzten Schrägstrichs
    int lastSlashIndex = topic.lastIndexOf('/');
    // Überprüfe, ob ein Schrägstrich gefunden wurde
    if (lastSlashIndex != -1) {
      // Extrahiere den Teil des Strings nach dem letzten Schrägstrich
      String deviceIdString = topic.substring(lastSlashIndex + 1);


      // Gib das Ergebnis aus
      // Wandle den extrahierten String in einen Integer um
      String id_from_incoming_message = deviceIdString;
      Serial.printf("id_from_incoming_message: %s \n", id_from_incoming_message);
      // Serial.println(id_from_incoming_message); // Ausgabe: 1
      return id_from_incoming_message;
    }
  }






/****************************************************************************************************
 * Webserver starten, unabhängig von STA oder AP
 ****************************************************************************************************/ 

void start_webserver(){
  server.on("/", send_website_settings);
  server.on("/save", save_settings);
  server.onNotFound(send_website_settings);
  server.begin();

  if (apMode) {
    Serial.println("Webserver + DNS gestartet (Captive Portal aktiv)");
  } else {
    Serial.println("Webserver gestartet (STA-Modus, erreichbar über lokale IP)");
  }
}



/****************************************************************************************************
 * Access Point (AP) starten, falls sich der ESP mit keinem WLAN Netzwerk verbinden kann
 ****************************************************************************************************/ 

void start_access_point(){
  Serial.println("Keine Verbindung, starte Access Point...");
  WiFi.softAP("Headphone Setup");  // offen, ohne Passwort

  IPAddress apIP = WiFi.softAPIP();
  Serial.print("AP gestartet, IP: ");
  Serial.println(apIP);

  // DNS-Server: Alle Domains -> ESP
  dnsServer.start(DNS_PORT, "*", apIP);

  apMode = true;   // wir sind im AP-Modus
}



/*********************************************************************
 * Websites
 *********************************************************************/

// "/" → wenn die IP Adresse des ESP im Browser aufgerufen wird bzw im AP Modus: wenn 192.168.4.1 aufgerufen wird: Formular anzeigen 
void send_website_settings() {                                          // aufgerufen in start_webserver()

  String page = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  page += "<title>Setup Kopfhörer-Station</title></head><body>";
  page += "<h2>Setup Kopfhörer-Station</h2>";
  page += "<form action='/save' method='POST'>";
  page += "SSID: <input type='text' name='ssid' value='" + targetSSID + "'><br><br>";
  page += "Passwort: <input type='password' name='password' value='" + targetPassword + "'><br><br>";
  page += "Device ID: <input type='text' name='id_station' value='" + id_station + "'><br><br>";
  page += "MQTT Broker IP-Adresse: <input type='text' name='mqtt_broker' value='" + mqtt_broker + "'><br><br>";

  page += "RFID-Tag Kopfhörer 1a: <input type='text' name='target_rfid_1a' value='" + target_rfid_1[0] + "'><br><br>";
  page += "RFID-Tag Kopfhörer 1b: <input type='text' name='target_rfid_1b' value='" + target_rfid_1[1] + "'><br><br>";
  page += "RFID-Tag Kopfhörer 1c: <input type='text' name='target_rfid_1c' value='" + target_rfid_1[2] + "'><br><br>";

  page += "RFID-Tag Kopfhörer 2a: <input type='text' name='target_rfid_2a' value='" + target_rfid_2[0] + "'><br><br>";
  page += "RFID-Tag Kopfhörer 2b: <input type='text' name='target_rfid_2b' value='" + target_rfid_2[1] + "'><br><br>";
  page += "RFID-Tag Kopfhörer 2c: <input type='text' name='target_rfid_2c' value='" + target_rfid_2[2] + "'><br><br>";

  page += "RFID-Tag Kopfhörer 3a: <input type='text' name='target_rfid_3a' value='" + target_rfid_3[0] + "'><br><br>";
  page += "RFID-Tag Kopfhörer 3b: <input type='text' name='target_rfid_3b' value='" + target_rfid_3[1] + "'><br><br>";
  page += "RFID-Tag Kopfhörer 3c: <input type='text' name='target_rfid_3c' value='" + target_rfid_3[2] + "'><br><br>";

  page += "RFID-Tag Kopfhörer 4a: <input type='text' name='target_rfid_4a' value='" + target_rfid_4[0] + "'><br><br>";
  page += "RFID-Tag Kopfhörer 4b: <input type='text' name='target_rfid_4b' value='" + target_rfid_4[1] + "'><br><br>";
  page += "RFID-Tag Kopfhörer 4c: <input type='text' name='target_rfid_4c' value='" + target_rfid_4[2] + "'><br><br>";

  page += "RFID-Tag Kopfhörer 5a: <input type='text' name='target_rfid_5a' value='" + target_rfid_5[0] + "'><br><br>";
  page += "RFID-Tag Kopfhörer 5b: <input type='text' name='target_rfid_5b' value='" + target_rfid_5[1] + "'><br><br>";
  page += "RFID-Tag Kopfhörer 5c: <input type='text' name='target_rfid_5c' value='" + target_rfid_5[2] + "'><br><br>";

  page += "RFID-Tag Kopfhörer 6a: <input type='text' name='target_rfid_6a' value='" + target_rfid_6[0] + "'><br><br>";
  page += "RFID-Tag Kopfhörer 6b: <input type='text' name='target_rfid_6b' value='" + target_rfid_6[1] + "'><br><br>";
  page += "RFID-Tag Kopfhörer 6c: <input type='text' name='target_rfid_6c' value='" + target_rfid_6[2] + "'><br><br>";

  page += "<input type='submit' value='Speichern & Verbinden'>";
  page += "</form>";
  page += "</body></html>";

  server.send(200, "text/html", page);
}


void send_website_save() {

  String page = "<!DOCTYPE html><html><body>";
  page += "<h2>Daten gespeichert!</h2>";
  page += "<p>SSID: '" + targetSSID + "'</p>";
  page += "<p>Passwort: '" + targetPassword + "'</p>";
  page += "<p>ESP versucht, sich zu verbinden...</p>";
  page += "</body></html>";
  
  server.send(200, "text/html", page);
}



// "/save" → wenn <IP-Adresse>/save bzw bei AP-Mode 192.168.4.1/save aufgerufen wurd, also wenn Formular abgeschickt wurde: Daten speichern und WLAN verbinden (dieser Funktionsaufruf ist in setup() hinterlegt)
void save_settings() {                                            // aufgerufen in start_webserver()
  if (server.method() == HTTP_POST) {
    if (server.hasArg("ssid")){
      targetSSID = server.arg("ssid");
      targetSSID.trim();                                          // Führende und nachfolgende Leerzeichen entfernen, um Verbindungsfehler zu vermeiden
    }
    if (server.hasArg("password")){
      targetPassword = server.arg("password");
      targetPassword.trim();
    }
    if (server.hasArg("id_station")){
      id_station = server.arg("id_station");
      id_station.trim();
    }
    if (server.hasArg("mqtt_broker")) {
      mqtt_broker = server.arg("mqtt_broker");
      mqtt_broker.trim();
    }
    if (server.hasArg("target_rfid_1a")){
      target_rfid_1[0] = server.arg("target_rfid_1a");
      target_rfid_1[0].trim();
    }
    if (server.hasArg("target_rfid_1b")){
      target_rfid_1[1] = server.arg("target_rfid_1b");
      target_rfid_1[1].trim();
    }
    if (server.hasArg("target_rfid_1c")){
      target_rfid_1[2] = server.arg("target_rfid_1c");
      target_rfid_1[2].trim();
    }

    if (server.hasArg("target_rfid_2a")){
      target_rfid_2[0] = server.arg("target_rfid_2a");
      target_rfid_2[0].trim();
    }
    if (server.hasArg("target_rfid_2b")){
      target_rfid_2[1] = server.arg("target_rfid_2b");
      target_rfid_2[1].trim();
    }
    if (server.hasArg("target_rfid_2c")){
      target_rfid_2[2] = server.arg("target_rfid_2c");
      target_rfid_2[2].trim();
    }

    if (server.hasArg("target_rfid_3a")){
      target_rfid_3[0] = server.arg("target_rfid_3a");
      target_rfid_3[0].trim();
    }
    if (server.hasArg("target_rfid_3b")){
      target_rfid_3[1] = server.arg("target_rfid_3b");
      target_rfid_3[1].trim();
    }
    if (server.hasArg("target_rfid_3c")){
      target_rfid_3[2] = server.arg("target_rfid_3c");
      target_rfid_3[2].trim();
    }

    if (server.hasArg("target_rfid_4a")){
      target_rfid_4[0] = server.arg("target_rfid_4a");
      target_rfid_4[0].trim();
    }
    if (server.hasArg("target_rfid_4b")){
      target_rfid_4[1] = server.arg("target_rfid_4b");
      target_rfid_4[1].trim();
    }
    if (server.hasArg("target_rfid_4c")){
      target_rfid_4[2] = server.arg("target_rfid_4c");
      target_rfid_4[2].trim();
    }

    if (server.hasArg("target_rfid_5a")){
      target_rfid_5[0] = server.arg("target_rfid_5a");
      target_rfid_5[0].trim();
    }
    if (server.hasArg("target_rfid_5b")){
      target_rfid_5[1] = server.arg("target_rfid_5b");
      target_rfid_5[1].trim();
    }
    if (server.hasArg("target_rfid_5c")){
      target_rfid_5[2] = server.arg("target_rfid_5c");
      target_rfid_5[2].trim();
    }

    if (server.hasArg("target_rfid_6a")){
      target_rfid_6[0] = server.arg("target_rfid_6a");
      target_rfid_6[0].trim();
    }
    if (server.hasArg("target_rfid_6b")){
      target_rfid_6[1] = server.arg("target_rfid_6b");
      target_rfid_6[1].trim();
    }
    if (server.hasArg("target_rfid_6c")){
      target_rfid_6[2] = server.arg("target_rfid_6c");
      target_rfid_6[2].trim();
    }

    // Im Flash speichern
    preferences.begin("wifi", false);
    preferences.putString("ssid", targetSSID);
    preferences.putString("password", targetPassword);
    preferences.putString("id_station", id_station);
    preferences.putString("mqtt_broker", mqtt_broker);

    preferences.putString("target_rfid_1a", target_rfid_1[0]);
    preferences.putString("target_rfid_1b", target_rfid_1[1]);
    preferences.putString("target_rfid_1c", target_rfid_1[2]);

    preferences.putString("target_rfid_2a", target_rfid_2[0]);
    preferences.putString("target_rfid_2b", target_rfid_2[1]);
    preferences.putString("target_rfid_2c", target_rfid_2[2]);

    preferences.putString("target_rfid_3a", target_rfid_3[0]);
    preferences.putString("target_rfid_3b", target_rfid_3[1]);
    preferences.putString("target_rfid_3c", target_rfid_3[2]);

    preferences.putString("target_rfid_4a", target_rfid_4[0]);
    preferences.putString("target_rfid_4b", target_rfid_4[1]);
    preferences.putString("target_rfid_4c", target_rfid_4[2]);

    preferences.putString("target_rfid_5a", target_rfid_5[0]);
    preferences.putString("target_rfid_5b", target_rfid_5[1]);
    preferences.putString("target_rfid_5c", target_rfid_5[2]);

    preferences.putString("target_rfid_6a", target_rfid_6[0]);
    preferences.putString("target_rfid_6b", target_rfid_6[1]);
    preferences.putString("target_rfid_6c", target_rfid_6[2]);

    preferences.end();

    // Rückmeldung
    send_website_save();

    // WLAN Verbindung starten
    delay(1000);
    connectWiFi();
  }
}



/**************************************************************************************************************
 * NFC-Funktionen
 **************************************************************************************************************/

void init_nfc(){                                                        // wird in start() aufgerufen
  // Wir nutzen die I2C-Konfiguration, die in Wire.begin() gestartet wird. Daher ist ein nfc.begin() hier ausreichend, um die Kommunikation zu starten.
  nfc.begin();                                                          // NFC Reader PN532 starten
  uint32_t nfc_versiondata = nfc.getFirmwareVersion();                  // Firmware-Version abfragen
  if (!nfc_versiondata) {
    Serial.println("Kein PN532 gefunden – Verbindung prüfen.");
    while (1);                                                          // bleibt hängen, wenn nichts gefunden wird
  }

  nfc.SAMConfig();                                                      // Konfiguriere das Modul für RFID-Lesen
  Serial.println("Found chip RFID/NFC Reader. Warte auf Tag...");
}



void read_nfc(){
  nfc_tag_erkannt = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, nfc_uid, &nfc_uidLength, 100);   // Versuche für 100ms, einen Tag zu erkennen. Der Timeout sollte nicht zu kurz sein, da der PN532 Zeit benötigt..
  nfc_isTargetTag = false;  

  String current_NFC_string = "";

  if (nfc_tag_erkannt) {  
    // UID in Hex-String konvertieren
    char nfc_payload[3 * sizeof(nfc_uid) + 1]; 
    nfc_payload[0] = '\0'; 

    for (uint8_t i = 0; i < nfc_uidLength; i++) {
      char byteStr[4];
      sprintf(byteStr, "%02X", nfc_uid[i]);      
      strcat(nfc_payload, byteStr);              
    }
    current_NFC_string = nfc_payload; 
    // Serial.println(current_NFC_string);

    current_headphone_id = identifyHeadphone(current_NFC_string);
  }

  // Logik für die Bestätigung von Anwesenheit/Abwesenheit
  if (nfc_isTargetTag) {
    nfc_absentConfirmCounter = 0;                                         // Reset des Abwesenheitszählers
    nfc_presentConfirmCounter++;                                          // Inkrementiere Anwesenheitszähler

    if (nfc_presentConfirmCounter >= NFC_CONFIRM_PRESENT_THRESHOLD) {     // Wenn der Tag oft genug hintereinander bestätigt wurde   
      nfc_presentConfirmCounter = NFC_CONFIRM_PRESENT_THRESHOLD;          // Zähler auf Max setzen, um Überlauf zu vermeiden
      if (!is_object_detected) {  
        Serial.printf("Ziel-Tag erkannt: %s\n", current_NFC_string);      // current_NFC_string == target_id 
        is_object_detected = true;                                        // Status auf "anwesend" setzen
      }
    }
  } else {                                                                // Wenn der Ziel-Tag NICHT gelesen wurde (entweder kein Tag oder falscher Tag)
    nfc_presentConfirmCounter = 0;                                        // Reset des Anwesenheitszählers
    nfc_absentConfirmCounter++;                                           // Inkrementiere Abwesenheitszähler

    if (nfc_absentConfirmCounter >= NFC_CONFIRM_ABSENT_THRESHOLD) {
      nfc_absentConfirmCounter = NFC_CONFIRM_ABSENT_THRESHOLD;            // Wenn der Tag oft genug hintereinander NICHT gelesen wurde -> Zähler auf Max setzen
      if (is_object_detected) {                                           // Wenn der Tag vorher als anwesend galt, jetzt aber nicht mehr      
        Serial.println("Ziel-Tag entfernt (entprellt).");
        is_object_detected = false;                                       // Status auf "nicht anwesend" setzen
      }
    }
  }
}


/*****************************************************************************************
 *    Erkenne, welcher Kopfhörer 1 - 6 in die Station gehängt oder entfernt wurde. Ordne den RFID String dem richtigen Kopfhörer zu.
 *****************************************************************************************/

// called in read_nfc()
int identifyHeadphone(String value) { 
  for (int i = 0; i < sizeof(target_rfid_1) / sizeof(target_rfid_1[0]); i++) {
    if (target_rfid_1[i] == value) {
      nfc_isTargetTag = true;
      return 1;
    }
  }
  for (int i = 0; i < sizeof(target_rfid_2) / sizeof(target_rfid_2[0]); i++) {
    if (target_rfid_2[i] == value) {
      nfc_isTargetTag = true;
      return 2;
    }
  }
  for (int i = 0; i < sizeof(target_rfid_3) / sizeof(target_rfid_3[0]); i++) {
    if (target_rfid_3[i] == value) {
      nfc_isTargetTag = true;
      return 3;
    }
  }
  for (int i = 0; i < sizeof(target_rfid_4) / sizeof(target_rfid_4[0]); i++) {
    if (target_rfid_4[i] == value) {
      nfc_isTargetTag = true;
      return 4;
    }
  }
  for (int i = 0; i < sizeof(target_rfid_5) / sizeof(target_rfid_5[0]); i++) {
    if (target_rfid_5[i] == value) {
      nfc_isTargetTag = true;
      return 5;
    }
  }
  for (int i = 0; i < sizeof(target_rfid_6) / sizeof(target_rfid_6[0]); i++) {
    if (target_rfid_6[i] == value) {
      nfc_isTargetTag = true;
      return 6;
    }
  }
  return 0; // nichts gefunden
}



/****************************************************************************************************
 * Taster lesen
 ****************************************************************************************************/ 

void read_button(){
  buttonState = digitalRead(TASTER_PIN);
  if(buttonState != prev_buttonState) {
    prev_buttonState = buttonState;
    if (buttonState == 1) {
      ESP.restart();
    }
  }
}


/****************************************************************************************************
 * LED-Ring
 ****************************************************************************************************/ 

void init_led_ring(){
  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS);
  strip.clear();
  for (int i = 0; i < 12; i++) {  
    strip.setPixelColor(i, strip.Color(20, 40, 0));                 // Werte: 0 - 255
  }
  strip.show();  
}


