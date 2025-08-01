#include <Wire.h>
#include <Adafruit_PN532.h>
#include "Adafruit_VL6180X.h"
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <MQTT.h>

// I2C Pins definieren
#define SDA_PIN 6
#define SCL_PIN 7

// IRQ und RESET Pins für PN532 (nicht verwendet bei I2C, aber von Bibliothek benötigt)
#define PN532_IRQ 2
#define PN532_RESET 3

// NeoPixel LED Ring Definition
#define LED_PIN 5
#define NUM_PIXELS 12
#define LED_BRIGHTNESS 50 // Helligkeit der LEDs (0-255)

// WLAN und MQTT Einstellungen
const char* ssid = "tinkergarden";
const char* pass = "strenggeheim";
const char* mqtt_broker = "192.168.0.28"; // IP-Adresse deines MQTT Brokers
const char* mqtt_publish_topic_audio = "holz/player/audio1de";
const char* mqtt_subscribe_topic_led = "device_audio1de/ledring";
const char* mqtt_client_id = "esp32c6_headphone_station";

// Schwellenwerte für TOF-Sensor und NFC-Erkennung
#define TOF_THRESHOLD_MM 100 // Beispiel: 100mm. Anpassen je nach Abstand des Kopfhörers zum Sensor.
                             // Wenn der Kopfhörer näher als dieser Wert ist, gilt er als "vorhanden".

// NFC UID des Kopfhörer-Tags (ANPASSUNG ERFOLGT)
// Ausgelesene UID: FF F 17 E9 3F 0 0
// Interpretiert als Bytes: {0xFF, 0x0F, 0x17, 0xE9, 0x3F, 0x00, 0x00}
const uint8_t HEADPHONE_NFC_UID[] = {0xFF, 0x0F, 0x17, 0xE9, 0x3F, 0x00, 0x00}; 
const uint8_t HEADPHONE_NFC_UID_LENGTH = sizeof(HEADPHONE_NFC_UID);

// Objekt-Instanzen
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET, &Wire);
Adafruit_VL6180X vl = Adafruit_VL6180X();
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

WiFiClient wificlient;
MQTTClient mqttclient;

// Zustandsvariablen
bool headphonesPresent = false; // True, wenn Kopfhörer auf der Halterung, False wenn entfernt
bool prevHeadphonesPresent = false; // Vorheriger Zustand für Erkennung von Änderungen

// Variable um zu verfolgen, ob der NFC-Reader initialisiert wurde
bool nfc_initialized = false;
bool tof_initialized = false;

// Funktionsprototypen
void connectWiFi();
void connectMQTT();
void messageReceived(String &topic, String &payload);
void setLeds(int numLeds);

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println("ESP32-C6 Kopfhörer-Station startet...");

  // I2C starten mit benutzerdefinierten Pins
  Wire.begin(SDA_PIN, SCL_PIN);


  
  

  // NeoPixel LED Ring starten
  Serial.println("Initialisiere LED Ring...");
  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS);
  strip.show(); // Alle LEDs aus
  Serial.println("LED Ring bereit.");

  // WLAN verbinden
  connectWiFi();

  // MQTT verbinden
  mqttclient.begin(mqtt_broker, wificlient);
  mqttclient.onMessage(messageReceived);
  connectMQTT();

  // TOF Sensor starten
  Serial.println("Initialisiere TOF Sensor...");
  if (! vl.begin()) {
    Serial.println("TOF Sensor nicht gefunden!");
    // while (1); // Bleibt hängen, wenn kein TOF Sensor gefunden wird
  } else {
    Serial.println("TOF Sensor gefunden!");
    tof_initialized = true;
  }

  // PN532 NFC Reader starten
  Serial.println("Initialisiere NFC Reader...");
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Kein PN532 gefunden – Verbindung prüfen.");
    // while (1); // Bleibt hängen, wenn kein NFC Reader gefunden wird
  } else {
    Serial.print("NFC Reader gefunden: PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
    Serial.print("Firmware Version: "); Serial.print((versiondata>>16) & 0xFF, DEC);
    Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
    nfc.SAMConfig(); // Konfiguriere das Modul für RFID-Lesen
    // Wichtig: Setze Retries auf 0xFF für schnelles Non-Blocking bei nicht gefundenem Tag
    nfc.setPassiveActivationRetries(0xFF); // Dies macht readPassiveTargetID nicht-blockierend
    Serial.println("NFC Reader bereit. Warte auf ein RFID/NFC Tag...");
    nfc_initialized = true;
  }

  Serial.println("Setup abgeschlossen.");
}

void loop() {
  // MQTT Loop aufrufen, um die Verbindung am Leben zu halten und Nachrichten zu empfangen
  mqttclient.loop();
  
  // Überprüfen, ob die MQTT-Verbindung aktiv ist. Wenn nicht, versuchen wir uns neu zu verbinden.
  // Diese Prüfung sollte nur erfolgen, wenn die Loop häufig genug durchläuft.
  // Durch setPassiveActivationRetries(0xFF) im Setup wird nfc.readPassiveTargetID() nicht blockieren.
  if (!mqttclient.connected()) {
    Serial.println("MQTT Verbindung verloren. Versuche Neuverbindung...");
    connectMQTT();
  }

  // --- Kopfhörer-Erkennung (TOF + NFC) ---
  uint8_t range = 0; // Standardwert, falls TOF nicht initialisiert
  uint8_t status = VL6180X_ERROR_NONE;
  bool tof_detected = false;

  if (tof_initialized) {
    range = vl.readRange();
    status = vl.readRangeStatus();
    if (status == VL6180X_ERROR_NONE) {
      //Serial.print("TOF Range: "); Serial.println(range);
      if (range < TOF_THRESHOLD_MM && range > 0) { // range > 0 um Fehlerwerte auszuschließen
        tof_detected = true;
      }
    } else {
      //Serial.print("TOF Error: "); Serial.println(status);
    }
  } else {
    //Serial.println("TOF Sensor nicht initialisiert, überspringe Lesevorgang.");
  }


  uint8_t uid[7];
  uint8_t uidLength;
  bool nfc_detected = false;
  
  if (nfc_initialized) { // Nur versuchen zu lesen, wenn PN532 initialisiert wurde
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
      Serial.print("NFC Tag erkannt, UID: ");
      for (uint8_t i = 0; i < uidLength; i++) {
        Serial.print(uid[i], HEX); Serial.print(" ");
      }
      Serial.println();

      // Vergleiche gelesene UID mit der gespeicherten Kopfhörer-UID
      if (uidLength == HEADPHONE_NFC_UID_LENGTH) {
        bool uid_match = true;
        for (uint8_t i = 0; i < uidLength; i++) {
          if (uid[i] != HEADPHONE_NFC_UID[i]) {
            uid_match = false;
            break;
          }
        }
        if (uid_match) {
          nfc_detected = true;
        } else {
          Serial.println("Tag erkannt, aber UID stimmt nicht überein.");
        }
      } else {
          Serial.println("Tag erkannt, aber UID-Länge stimmt nicht überein.");
      }
    }
  } else {
    // Serial.println("NFC Reader nicht initialisiert.");
  }


  // Logik zur Bestimmung des Kopfhörer-Zustands
  // Kopfhörer ist auf der Halterung, wenn TOF nah genug ist UND NFC Tag erkannt wird
  headphonesPresent = tof_detected && nfc_detected;

  // Sende MQTT Nachricht, wenn sich der Zustand ändert
  if (headphonesPresent != prevHeadphonesPresent) {
    if (mqttclient.connected()) {
      if (headphonesPresent) {
        // Kopfhörer wurde zurückgehängt
        Serial.println("Kopfhörer zurückgehängt. Sende 'pause' via MQTT.");
        mqttclient.publish(mqtt_publish_topic_audio, "pause");
      } else {
        // Kopfhörer wurde entfernt
        Serial.println("Kopfhörer entfernt. Sende 'play' via MQTT.");
        mqttclient.publish(mqtt_publish_topic_audio, "play");
      }
    } else {
      Serial.println("MQTT nicht verbunden, konnte Nachricht nicht senden.");
    }
    prevHeadphonesPresent = headphonesPresent;
  }

  // Ein kurzes delay ist immer noch nützlich, um den ESP32 nicht ständig auf Hochtouren laufen zu lassen
  // Aber es muss kurz genug sein, damit mqttclient.loop() oft genug aufgerufen wird
  // 10ms oder 50ms sind hier besser als 100ms oder mehr.
  delay(50); 
}

// --- WLAN und MQTT Funktionen ---

void connectWiFi() {
  Serial.print("Verbinde mit WLAN ");
  Serial.print(ssid);
  WiFi.begin(ssid, pass);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) { // Max 20 Versuche (10 Sekunden)
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi verbunden!");
    Serial.print("IP-Adresse: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi Verbindung fehlgeschlagen! Bitte SSID/Passwort prüfen.");
    //while(1); // Optional: Hier hängen bleiben, wenn keine Verbindung möglich ist
  }
}

void connectMQTT() {
  Serial.print("Verbinde mit MQTT Broker: ");
  Serial.println(mqtt_broker);

  int attempts = 0;
  // Der Standard-Keep-Alive-Wert von MQTT ist 15 Sekunden. Wir sollten versuchen, uns innerhalb dieses Zeitraums wiederzuverbinden.
  while (!mqttclient.connected() && attempts < 10) { // Max 10 Versuche mit 500ms delay = 5 Sekunden
    Serial.print(".");
    delay(500); // Kurze Pause zwischen den Verbindungsversuchen
    attempts++;
  }

  if (mqttclient.connected()) {
    Serial.println("Erfolgreich mit MQTT Broker verbunden!");
    mqttclient.subscribe(mqtt_subscribe_topic_led);
    Serial.print("Abonniert Topic: ");
    Serial.println(mqtt_subscribe_topic_led);
  } else {
    Serial.println("Verbindung zum MQTT Broker fehlgeschlagen.");
  }
}

// --- MQTT Callback für eingehende Nachrichten (LED-Steuerung) ---
void messageReceived(String &topic, String &payload) {
  Serial.println("MQTT Nachricht empfangen:");
  Serial.print("  Topic: "); Serial.println(topic);
  Serial.print("  Payload: "); Serial.println(payload);

  if (topic == mqtt_subscribe_topic_led) {
    int numLedsToLight = payload.toInt();
    Serial.print("Setze ");
    Serial.print(numLedsToLight);
    Serial.println(" LEDs grün.");
    setLeds(numLedsToLight);
  }
}

// --- LED Ring Steuerfunktion ---
void setLeds(int numLeds) {
  strip.clear(); // Alle LEDs ausschalten

  // Sicherstellen, dass die Anzahl der LEDs im gültigen Bereich liegt
  if (numLeds < 0) numLeds = 0;
  if (numLeds > NUM_PIXELS) numLeds = NUM_PIXELS;

  for (int i = 0; i < numLeds; i++) {
    strip.setPixelColor(i, strip.Color(0, 255, 0)); // Grün (R, G, B)
  }
  strip.show(); // Änderungen an den LED-Ring senden
}