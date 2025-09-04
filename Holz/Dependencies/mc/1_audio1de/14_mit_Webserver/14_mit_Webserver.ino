/*****************************************************************
 * Read PN532 sensor via I2C and publish to MQTT broker whether an RFID Tag is close to the reader or not.
 * In der Konsole wird nur geprintet, wann die "Session" (erkannt oder nicht erkannt) beginnt und wann sie endet.
 * Für etwas Toleranz wird der Wert mit den letzten Werten verglichen. Um Fehlimpulse zu vermeiden, wird die Konsole erst beschrieben,
 * sobald die aktuelle Messung mit den letzten paar Messungen übereinstimmt.
 * Wenn kein bekanntes Netzwerk gefunden wird, startet der ESP einen Access Point. Wenn man dieses WLAN Netzwerk am Computer / Smartphone auswählt,
 * kommt man per Captive WLAN auf eine Setup-Website. Dort können SSID und PW des WLAN-Netzwerks eingegeben werden, mit dem man sich verbinden mag.
 *
 * Anschluss:

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
 *
 Mit diesem Code erzeugt der ESP einen Access Point namens "ESP32-Setup" ohne Passwort.
Mit diesem Code kommt man direkt auf die Config-Website, ohne dass man die IP Adresse eintippen muss, wie im Hotel-WLAN ("Captive Portal").
Dazu starten wir einen DNS-Server, der alle Domains auf die ESP-IP 192.168.4.1 umleitet.
Auf der Setup-Website kann man dann wie gehabt Variablen im Code benennen, wie z. B. SSID und Passwort eines Netzwerks, mit dem sich der ESP verbinden soll. 
Das wird dann im Flash Speicher des ESP gespeichert, damit er sich diese Info beim nächsten Start direkt abrufen kann 
und sich z. B. ohne Nachfrage mit einem Netzwerk verbinden kann.

******************************************************************/


#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <DNSServer.h>                  // @neu für Captive Portal

#include <MQTT.h>
#include <Wire.h>                                                      // I2C
// #include "Adafruit_VL6180X.h"
#include <Adafruit_PN532.h>                                            // NFC Reader
#include "esp_log.h"                                                   // nicht die Konsole voll schreiben, wenn es ab und zu einen I2C Nack gibt
#include <Adafruit_NeoPixel.h>


Preferences preferences;
String device_id = "";

// Variablen für die Ziel-WLAN-Daten - wird auf der Setup-Website des ESP-Access Points eingegeben.
String targetSSID = "";                 // "Energiepark Technik", "tinkergarden", "LinusFetzMusikGast"
String targetPassword = "";             // "Energiepark2025.8", "strenggeheim", "linusfetzgast"
WiFiClient wificlient;

bool apMode = false;

// Lokaler Webserver auf Port 80
WebServer server(80);
DNSServer dnsServer;                    // @neu für Captive Portal
const byte DNS_PORT = 53;               // @neu für Captive Portal.  // DNS-Server: Alle Anfragen -> ESP32

// MQTT
String mqtt_broker = "";                            // "broker.emqx.io", "192.168.0.60", "test.mosquitto.org", "broker.hivemq.com"
const char* mqtt_client_id = "headphone_station_audio1de";

String MQTT_PUBLISH_TOPIC_AUDIO = "holz/player/audio<device_id>de";                    // Topics.  -   Diese werden für Subscribing und Publishing genutzt
String MQTT_SUBSCRIBE_TOPIC_SECTIONS_LEDRING = "holz/ledring_audio<device_id>de/numsections";      // Payload: 0 - 12
String MQTT_SUBSCRIBE_TOPIC_TRACK_STATE = "holz/ledring_audio<device_id>de/state";                 // message when audio track ended -> Payload: "ended"
String subscribe_topics[10];         // Array der zu abonnierenden Topics (hier nur eines, aber erweiterbar)
MQTTClient mqttclient;


// I2C Pins definieren
#define SDA_PIN 6
#define SCL_PIN 7


// NFC Reader
// IRQ und RESET Pins definieren – werden vom PN532-Modul NICHT verwendet bei I2C, aber Bibliothek erwartet sie
#define PN532_IRQ   (2)
#define PN532_RESET (3)

// Konstruktor mit IRQ, RESET und Wire
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET, &Wire);

// Ziel-UID definieren (7 Bytes)
// const uint8_t TARGET_UID[] = { 0xFF, 0x0F, 0x17, 0xE9, 0x3F, 0x00, 0x00 };
String target_rfid_1 = "";          // 14AA77D6
String target_rfid_2 = "";          // A44347D0
const uint8_t NFC_TARGET_UID_LENGTH = 7;

// --- Statusvariablen für die verbesserte Erkennung ---
// bool nfc_tagCurrentlyPresent = false;                                  // Merkt sich, ob der TARGET_UID-Tag aktuell als "anwesend" gilt // Wert ist bereits entprellt
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
void mqtt_messageReceived(String& topic, String& payload);
void init_nfc();
void read_nfc();

String htmlPage();
void handleSave();
void handleRoot();











void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Kopfhörer-Station startet...");

  // Gespeicherte WLAN-Daten laden, falls bereits per Setup-Seite festgelegt
  preferences.begin("wifi", true);
  targetSSID = preferences.getString("ssid", "");
  targetPassword = preferences.getString("password", "").c_str();
  mqtt_broker = preferences.getString("mqtt_broker", "").c_str();
  target_rfid_1 = preferences.getString("target_rfid_1", "");
  target_rfid_2 = preferences.getString("target_rfid_2", "");
  device_id = preferences.getString("device_id", "9");
  preferences.end();
  Serial.printf("Found in Flash: SSID: %s, Passwort: %s \n", 
                targetSSID.c_str(), targetPassword.c_str());

  // Versuche, mit den gespeicherten Daten zu verbinden
  if (targetSSID != "") {
    Serial.printf("Versuche Verbindung mit gespeichertem WLAN: '%s' (PW: '%s')\n", 
                  targetSSID.c_str(), targetPassword.c_str());
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
    Serial.printf("Erfolgreich verbunden: SSID: %s, IP-Adresse: %s \n", 
                  targetSSID.c_str(), WiFi.localIP().toString().c_str());
    apMode = false;


    // --- MQTT Client initialisieren und Callback setzen ---

    mqttclient.begin(mqtt_broker.c_str(), wificlient);
    connectMQTT();
    mqttclient.onMessage(mqtt_messageReceived);

  } else {
    // Falls keine Verbindung zustande kommt, starte den Access Point
    Serial.println("Keine Verbindung, starte Access Point...");

    WiFi.softAP("Headphone Setup");  // offen, ohne Passwort

    IPAddress apIP = WiFi.softAPIP();
    Serial.print("AP gestartet, IP: ");
    Serial.println(apIP);

    // DNS-Server: Alle Domains -> ESP
    dnsServer.start(DNS_PORT, "*", apIP);

    apMode = true;   // wir sind im AP-Modus
  }

  // --- EINMALIG: Webserver starten, unabhängig von STA oder AP ---
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.onNotFound(handleRoot);
  server.begin();

  if (apMode) {
    Serial.println("Webserver + DNS gestartet (Captive Portal aktiv)");
  } else {
    Serial.println("Webserver gestartet (STA-Modus, erreichbar über lokale IP)");
  }

  // I2C
  esp_log_set_vprintf([](const char *fmt, va_list args) -> int {
    (void)fmt;   // ignore
    (void)args;  // ignore
    return 0;    // nichts ausgeben
  });

  Wire.begin(SDA_PIN, SCL_PIN);   // Wire starten mit den benutzerdefinierten I2C Pins
  init_nfc();                     // NFC Sensor

  // Init LED-Ring
  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS);
  strip.clear();
  for (int i = 0; i < 12; i++) {  
    strip.setPixelColor(i, strip.Color(20, 40, 0));   // Werte: 0 - 255
  }
  strip.show();  

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
    delay(10000);
  }
  
  // MQTT
  if (!apMode) {
    mqttclient.loop();                                                    // MQTT Loop regelmässig aufrufen, um die Verbindung am Leben zu halten und Nachrichten zu empfangen  
    if (!mqttclient.connected()) {                                        // Überprüfen, ob die MQTT-Verbindung aktiv ist. Wenn nicht, versuchen wir uns neu zu verbinden.
      Serial.println("MQTT Verbindung verloren. Versuche Neuverbindung...");
      connectMQTT();
    }
  }

  // RFID
  read_nfc();                                                            // get NFC "button" state

  // Alterative: Taster:
  buttonState = digitalRead(TASTER_PIN);
  if(buttonState != prev_buttonState) {
    prev_buttonState = buttonState;
    if (buttonState == 1) {
      String publishPayload = "play";
      Serial.printf("--> Publishing: Topic: %s, Payload: %s\n", MQTT_PUBLISH_TOPIC_AUDIO.c_str(), publishPayload);
      if(mqttclient.connected()) {
        mqttclient.publish(MQTT_PUBLISH_TOPIC_AUDIO.c_str(), publishPayload);
      }
      strip.clear();
      strip.show();  
    }
  }

  if(is_object_detected != prev_is_object_detected) {
    prev_is_object_detected = is_object_detected;
    
    if(is_object_detected) {
      String publishPayload = "pause";
      Serial.printf("--> Publishing: Topic: %s, Payload: %s\n", MQTT_PUBLISH_TOPIC_AUDIO.c_str(), publishPayload);
      if(mqttclient.connected()) {
        mqttclient.publish(MQTT_PUBLISH_TOPIC_AUDIO.c_str(), publishPayload);
      }
    } else {
      String publishPayload = "play";
      Serial.printf("--> Publishing: Topic: %s, Payload: %s\n", MQTT_PUBLISH_TOPIC_AUDIO.c_str(), publishPayload);
      if(mqttclient.connected()) {
        mqttclient.publish(MQTT_PUBLISH_TOPIC_AUDIO.c_str(), publishPayload);
      }
    }
  }

  delay(100);                                                             // kurze Pause, entlastet den Prozessor
}

/********************
 * WLAN-Funktionen
 ********************/
void connectWiFi() {
  // Diese Funktion wurde geleert, da die Verbindungslogik jetzt in setup() behandelt wird.
  // Es ist besser, die Verbindung nur einmal im setup() zu versuchen. Wenn sie verloren geht,
  // wird in der loop() der Reconnect-Mechanismus ausgelöst.
}




/********************
 * MQTT-Funktionen
 ********************/
void connectMQTT() {                                                      // Diese Funktion kümmert sich nur noch um den eigentlichen Verbindungsaufbau und das Abonnement.
  Serial.printf("Verbinde mit MQTT Broker: %s\n", mqtt_broker);
  int attempts = 0;
  // Versuche, eine Verbindung herzustellen, bis sie erfolgreich ist
  while (!mqttclient.connect(mqtt_client_id)) {                           // Optional: Mit Benutzername/Passwort: mqttclient.connect(mqtt_client_id, "username", "password"). ->  -> username und passwort sind oft jeweils "public"
    Serial.println("Fehler beim Verbinden mit MQTT-Broker. Neuer Versuch in 2 Sekunden...");
    attempts++;
    if(attempts > 10){
      Serial.println("\nMQTT Verbindung fehlgeschlagen! Neustart ...");
      ESP.restart();   // Neustart auslösen
    }
    delay(2000);
  }
  
  Serial.println("Verbunden mit MQTT-Broker");


  MQTT_SUBSCRIBE_TOPIC_SECTIONS_LEDRING.replace("<device_id>", String(device_id));
  mqttclient.subscribe(MQTT_SUBSCRIBE_TOPIC_SECTIONS_LEDRING);
  Serial.printf("Abonniert Topic: %s\n", MQTT_SUBSCRIBE_TOPIC_SECTIONS_LEDRING.c_str());          // Bestätigung des Abonnements

  MQTT_SUBSCRIBE_TOPIC_TRACK_STATE.replace("<device_id>", String(device_id));
  mqttclient.subscribe(MQTT_SUBSCRIBE_TOPIC_TRACK_STATE);
  Serial.printf("Abonniert Topic: %s\n", MQTT_SUBSCRIBE_TOPIC_TRACK_STATE.c_str());  
  

  MQTT_PUBLISH_TOPIC_AUDIO.replace("<device_id>", String(device_id));  // Ersetze Platzhalter
  
}




/****************************************
 * MQTT Callback für eingehende Nachrichten
 ****************************************/
void mqtt_messageReceived(String& topic, String& payload) {
  Serial.printf("<-- MQTT msg empfangen: Topic: %s, Payload: %s\n", topic.c_str(), payload.c_str());

  // MQTT_SUBSCRIBE_TOPIC_SECTIONS_LEDRING.replace("<device_id>", String(device_id));  // Ersetze Platzhalter
  if (topic.equals(MQTT_SUBSCRIBE_TOPIC_SECTIONS_LEDRING)) {
    int numLedsToLight = payload.toInt();
    // Serial.printf("Empfangen: %d LEDs\n", numLedsToLight);                // %d für Integer

    // LED-Ring bespielen
    strip.clear();
    for(int i=0; i<numLedsToLight; i++) {                   // für jeden einzelnen Pixel - in der Schleife
      strip.setPixelColor(i, strip.Color(128, 255, 0));   // Werte: 0 - 255
    }
    strip.show();                                     // sende den aktualisierten Pixel an den LED-Ring
  }

  // MQTT_SUBSCRIBE_TOPIC_TRACK_STATE.replace("<device_id>", String(device_id));  // Ersetze Platzhalter
  if (topic.equals(MQTT_SUBSCRIBE_TOPIC_TRACK_STATE)) {
    Serial.printf("Empfangen: %s \n", payload);                // %d für Integer

    if(payload == "ended"){
      strip.clear();
      for(int i=0; i<12; i++) {                   // für jeden einzelnen Pixel - in der Schleife    
        strip.setPixelColor(i, strip.Color(20, 40, 0));   // Werte: 0 - 255
      }
      strip.show();                                     // sende den aktualisierten Pixel an den LED-Ring
    }
  }
}

/******************
 * NFC-Funktionen
 ******************/
void init_nfc(){                                                          // wird in start() aufgerufen
  // Wir nutzen die I2C-Konfiguration, die in Wire.begin() gestartet wird.
  // Daher ist ein nfc.begin() hier ausreichend, um die Kommunikation zu starten.
  nfc.begin();                                                          // NFC Reader PN532 starten
  uint32_t nfc_versiondata = nfc.getFirmwareVersion();                  // Firmware-Version abfragen
  if (!nfc_versiondata) {
    Serial.println("Kein PN532 gefunden – Verbindung prüfen.");
    while (1);                                                          // bleibt hängen, wenn nichts gefunden wird
  }

  // Konfiguriere das Modul für RFID-Lesen
  nfc.SAMConfig();
  Serial.println("Found chip RFID/NFC Reader. Warte auf Tag...");
}

void read_nfc(){
  // NFC
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

    if(current_NFC_string == target_rfid_1 || current_NFC_string == target_rfid_2 ){
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
      if (!is_object_detected) {  
        Serial.printf("Ziel-Tag erkannt: %s\n", current_NFC_string);              // current_NFC_string == target_id 
        is_object_detected = true; // Status auf "anwesend" setzen
      }
    }
  } else {                                                                // Wenn der Ziel-Tag NICHT gelesen wurde (entweder kein Tag oder falscher Tag)
    nfc_presentConfirmCounter = 0;                                        // Reset des Anwesenheitszählers
    nfc_absentConfirmCounter++;                                           // Inkrementiere Abwesenheitszähler

    if (nfc_absentConfirmCounter >= NFC_CONFIRM_ABSENT_THRESHOLD) {
      nfc_absentConfirmCounter = NFC_CONFIRM_ABSENT_THRESHOLD;            // Wenn der Tag oft genug hintereinander NICHT gelesen wurde -> Zähler auf Max setzen
      if (is_object_detected) {                                      // Wenn der Tag vorher als anwesend galt, jetzt aber nicht mehr      
        Serial.println("Ziel-Tag entfernt (entprellt).");
        is_object_detected = false;                                  // Status auf "nicht anwesend" setzen
      }
    }
  }
}

/***********************
 * Webserver-Funktionen
 ***********************/
String htmlPage() {
  String page = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  page += "<title>ESP32-C6 Setup</title></head><body>";
  page += "<h2>WLAN Einstellungen</h2>";
  page += "<form action='/save' method='POST'>";
  page += "SSID: <input type='text' name='ssid' value='" + targetSSID + "'><br><br>";
  page += "Passwort: <input type='password' name='password' value='" + targetPassword + "'><br><br>";
  page += "Device ID: <input type='text' name='device_id' value='" + device_id + "'><br><br>";
  page += "MQTT Broker IP-Adresse: <input type='text' name='mqtt_broker' value='" + mqtt_broker + "'><br><br>";
  page += "RFID-Tag 1: <input type='text' name='target_rfid_1' value='" + target_rfid_1 + "'><br><br>";
  page += "RFID-Tag 2: <input type='text' name='target_rfid_2' value='" + target_rfid_2 + "'><br><br>";
  page += "<input type='submit' value='Speichern & Verbinden'>";
  page += "</form>";
  page += "</body></html>";
  return page;
}

// "/" → wenn 192.168.4.1 aufgerufen wird: Formular anzeigen (diese Funktion ist in setup() hinterlegt)
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

// "/save" → wenn 192.168.4.1/save aufgerufen wurd, also wenn Formular abgeschickt wurde: Daten speichern und WLAN verbinden (dieser Funktionsaufruf ist in setup() hinterlegt)
void handleSave() {
  if (server.method() == HTTP_POST) {
    if (server.hasArg("ssid")){
      targetSSID = server.arg("ssid");
      targetSSID.trim();      // Führende und nachfolgende Leerzeichen entfernen, um Verbindungsfehler zu vermeiden
    }
    if (server.hasArg("password")){
      targetPassword = server.arg("password");
      targetPassword.trim();
    }
    if (server.hasArg("device_id")){
      device_id = server.arg("device_id");
      device_id.trim();
    }
    if (server.hasArg("mqtt_broker")) {
      mqtt_broker = server.arg("mqtt_broker");
      mqtt_broker.trim();
    }
    if (server.hasArg("target_rfid_1")){
      target_rfid_1 = server.arg("target_rfid_1");
      target_rfid_1.trim();
    }
    if (server.hasArg("target_rfid_2")){
      target_rfid_2 = server.arg("target_rfid_2");
      target_rfid_2.trim();
    }

    // Im Flash speichern
    preferences.begin("wifi", false);
    preferences.putString("ssid", targetSSID);
    preferences.putString("password", targetPassword);
    preferences.putString("device_id", device_id);
    preferences.putString("mqtt_broker", mqtt_broker);
    preferences.putString("target_rfid_1", target_rfid_1);
    preferences.putString("target_rfid_2", target_rfid_2);
    preferences.end();

    // Rückmeldung
    String response = "<!DOCTYPE html><html><body>";
    response += "<h2>Daten gespeichert!</h2>";
    response += "<p>SSID: '" + targetSSID + "'</p>";
    response += "<p>Passwort: '" + targetPassword + "'</p>";
    response += "<p>ESP versucht, sich zu verbinden...</p>";
    response += "</body></html>";

    server.send(200, "text/html", response);

    // WLAN Verbindung starten
    delay(1000);
    WiFi.begin(targetSSID.c_str(), targetPassword.c_str());

    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.println("Direkt nach Setup verbunden!");
      Serial.printf("Erfolgreich verbunden: SSID: %s, IP-Adresse: %s \n", targetSSID.c_str(), WiFi.localIP().toString().c_str() );
    } else {
      Serial.println();
      Serial.println("Konnte sich nach Speichern nicht sofort verbinden.");
    }
  }
}