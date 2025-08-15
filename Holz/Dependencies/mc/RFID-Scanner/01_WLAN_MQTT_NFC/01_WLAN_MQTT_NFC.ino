/*****************************************************************
 * Beispiel: WLAN + RFID Scanner
 * Read RFID cards via I2C and publish the recognized ID to MQTT broker 
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
******************************************************************/


#include <WiFi.h>
#include <MQTT.h>
#include <Wire.h>                                                      // I2C
#include <Adafruit_PN532.h>                                            // rfid Reader


// WLAN und MQTT Einstellungen
const char* ssid = "tinkergarden";                                // @todo: add your wifi name "FRITZ!Box 6690 TA"
const char* pass = "strenggeheim";                             // @todo: add your wifi pw, "79854308499311013585"
const char* mqtt_broker = "192.168.0.60";                            // "broker.emqx.io", "192.168.0.80"
const char* mqtt_client_id = "holz_rfid_station";

const char* MQTT_PUBLISH_TOPIC_RFID = "holz/rfid";         // Topics.  -   Diese werden für Subscribing und Publishing genutzt
// const char* MQTT_SUBSCRIBE_TOPIC_LED = "device_audio1de/ledring";
// const char* subscribe_topics[] = { MQTT_SUBSCRIBE_TOPIC_LED };         // Array der zu abonnierenden Topics (hier nur eines, aber erweiterbar)

WiFiClient wificlient;
MQTTClient mqttclient;




// I2C Pins definieren
#define SDA_PIN 6
#define SCL_PIN 7

// IRQ und RESET Pins definieren – werden vom PN532-Modul NICHT verwendet bei I2C, aber Bibliothek erwartet sie
#define PN532_IRQ   (2)
#define PN532_RESET (3)

// Konstruktor mit IRQ, RESET und Wire
Adafruit_PN532 rfid(PN532_IRQ, PN532_RESET, &Wire);



// --- Statusvariablen für die verbesserte Erkennung ---
// bool rfid_tagCurrentlyPresent = false;                                  // Merkt sich, ob der TARGET_UID-Tag aktuell als "anwesend" gilt
// int rfid_presentConfirmCounter = 0;                                     // Zählt aufeinanderfolgende erfolgreiche Lesungen des TARGET_UID
// int rfid_absentConfirmCounter = 0;                                      // Zählt aufeinanderfolgende fehlgeschlagene Lesungen (Tag nicht da)

// const int rfid_CONFIRM_PRESENT_THRESHOLD = 3;                           // Der Tag muss X-mal hintereinander korrekt gelesen werden, um als "anwesend" zu gelten.
// const int rfid_CONFIRM_ABSENT_THRESHOLD = 5;                            // Der Tag muss X-mal hintereinander NICHT gelesen werden, um als "entfernt" zu gelten.

uint8_t rfid_uid[7];                                                    // Buffer zum Speichern der UID
uint8_t rfid_uidLength;                                                 // Länge der UID
bool rfid_tag_erkannt;                                                  // in dieser Variable wird gespeichert, ob irgendein rfid Tag erkannt wurde
// bool rfid_isTargetTag;                                                  // handelt es sich bei dem erkannten rfid Tag wirklich um den gewünschten? 




// Funktionsprototypen
void connectWiFi();
void connectMQTT();


void setup() {
  Serial.begin(115200);
  delay(500);                                                           // warten, sonst werden die Serial.print in setup() nicht auf die Komsole gedruckt
  Serial.println("Kopfhörer-Station startet...");

  // WLAN verbinden
  connectWiFi();

  // --- MQTT Client initialisieren und Callback setzen ---
  // Diese Zeilen müssen hier stehen und nicht in connectMQTT()
  mqttclient.begin(mqtt_broker, wificlient);
  connectMQTT();

  // I2C
  Wire.begin(SDA_PIN, SCL_PIN);                                         // Wire starten mit den benutzerdefinierten I2C Pins

  // rfid
  rfid.begin();                                                          // rfid Reader PN532 starten
  uint32_t rfid_versiondata = rfid.getFirmwareVersion();                  // Firmware-Version abfragen
  if (!rfid_versiondata) {
    Serial.println("Kein PN532 gefunden – Verbindung prüfen.");
    while (1);                                                          // bleibt hängen, wenn nichts gefunden wird
  }

  // Chip-Daten anzeigen
  Serial.printf("Found chip PN5%X\n", (rfid_versiondata >> 24) & 0xFF);
  Serial.printf("Firmware Version: %d.%d\n", (rfid_versiondata >> 16) & 0xFF, (rfid_versiondata >> 8) & 0xFF);

  // Konfiguriere das Modul für RFID-Lesen
  rfid.SAMConfig();
  Serial.println("Warte auf ein RFID/rfid Tag...");


  Serial.println("Setup abgeschlossen.");
}

void loop() {
  // MQTT
  mqttclient.loop();                                                    // MQTT Loop regelmässig aufrufen, um die Verbindung am Leben zu halten und Nachrichten zu empfangen  
  if (!mqttclient.connected()) {                                        // Überprüfen, ob die MQTT-Verbindung aktiv ist. Wenn nicht, versuchen wir uns neu zu verbinden.
    Serial.println("MQTT Verbindung verloren. Versuche Neuverbindung...");
    connectMQTT();
  }

  // rfid
  rfid_tag_erkannt = rfid.readPassiveTargetID(PN532_MIFARE_ISO14443A, rfid_uid, &rfid_uidLength, 100);   // Versuche für 100ms, einen Tag zu erkennen. Der Timeout sollte nicht zu kurz sein, da der PN532 Zeit benötigt..
  // rfid_isTargetTag = false;  



   if (rfid_tag_erkannt) {                                                 // Prüfen, ob ein RFID Tag erkannt wurde
    // UID in Hex-String konvertieren, um sie als lesbare Zeichenkette per MQTT zu senden
    char rfid_payload[3 * sizeof(rfid_uid) + 1]; // 2 Zeichen pro Byte + Leerzeichen + Terminator
    rfid_payload[0] = '\0'; // String leeren/initialisieren

    for (uint8_t i = 0; i < rfid_uidLength; i++) {
      char byteStr[4];
      sprintf(byteStr, "%02X", rfid_uid[i]); // Byte als zweistellige HEX-Zahl
      // strcat(rfid_payload, byteStr);
      if (i < rfid_uidLength - 1) {
        strcat(rfid_payload, " "); // Leerzeichen zwischen Bytes (optional, kann entfernt werden)
      }
    }

    // UID an MQTT Broker senden
    mqttclient.publish(MQTT_PUBLISH_TOPIC_RFID, rfid_payload);

    // UID auch im Serial Monitor anzeigen
    Serial.print("RFID Tag erkannt: ");
    Serial.println(rfid_payload);
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
}
