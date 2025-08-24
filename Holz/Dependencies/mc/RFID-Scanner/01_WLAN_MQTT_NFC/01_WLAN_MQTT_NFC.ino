/*****************************************************************
 * Beispiel: WLAN + RFID Scanner
 * Read RFID cards via I2C and publish the recognized ID to MQTT broker 
 * Sende RFID Payload nur einmal, während der Chip aufliegt. 
 * Sende erst wieder, nachdem der Chip mindestens einmal entfernt wurde.
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
#include <Adafruit_PN532.h>                                           der // rfid Rea

// WLAN und MQTT Einstellungen
const char* ssid = "Energiepark Technik";                                
const char* pass = "Energiepark2025.8";                             
const char* mqtt_broker = "192.168.178.25";                            
const char* mqtt_client_id = "holz_rfid_station";

const char* MQTT_PUBLISH_TOPIC_RFID = "holz/rfid";         

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

// --- Statusvariablen ---
uint8_t rfid_uid[7];                                                    
uint8_t rfid_uidLength;                                                 
bool rfid_tag_erkannt;                                                  
String latest_RFID_string = "";          // zuletzt erkannte UID (gesendet)
String current_RFID_string = "";         // aktuell erkannte UID
bool rfid_present = false;               // merkt, ob gerade ein Tag aufgelegt ist
// long recent_RFID_recognition_time = 0;   

// Funktionsprototypen
void connectWiFi();
void connectMQTT();

void setup() {
  Serial.begin(115200);
  delay(500);                                                           
  Serial.println("RFID-Station startet...");

  connectWiFi();

  mqttclient.begin(mqtt_broker, wificlient);
  connectMQTT();

  Wire.begin(SDA_PIN, SCL_PIN);                                         

  rfid.begin();                                                          
  uint32_t rfid_versiondata = rfid.getFirmwareVersion();                  
  if (!rfid_versiondata) {
    Serial.println("Kein PN532 gefunden – Verbindung prüfen.");
    while (1);                                                          
  }

  Serial.printf("Found chip PN5%X\n", (rfid_versiondata >> 24) & 0xFF);
  Serial.printf("Firmware Version: %d.%d\n", (rfid_versiondata >> 16) & 0xFF, (rfid_versiondata >> 8) & 0xFF);

  rfid.SAMConfig();
  Serial.println("Warte auf ein RFID/rfid Tag...");

  Serial.println("Setup abgeschlossen.");
}

void loop() {
  mqttclient.loop();                                                    
  if (!mqttclient.connected()) {                                        
    Serial.println("MQTT Verbindung verloren. Versuche Neuverbindung...");
    connectMQTT();
  }

  // RFID Tag abfragen
  rfid_tag_erkannt = rfid.readPassiveTargetID(PN532_MIFARE_ISO14443A, rfid_uid, &rfid_uidLength, 100);   

  if (rfid_tag_erkannt) {  
    // UID in Hex-String konvertieren
    char rfid_payload[3 * sizeof(rfid_uid) + 1]; 
    rfid_payload[0] = '\0'; 

    for (uint8_t i = 0; i < rfid_uidLength; i++) {
      char byteStr[4];
      sprintf(byteStr, "%02X", rfid_uid[i]);      
      strcat(rfid_payload, byteStr);              
    }

    current_RFID_string = rfid_payload;  
    // recent_RFID_recognition_time = millis();

    // --- NEUE LOGIK ---
    if (!rfid_present || current_RFID_string != latest_RFID_string) {
      // Nur senden, wenn neuer Tag erkannt wurde oder vorher keiner präsent war
      latest_RFID_string = current_RFID_string;
      rfid_present = true;

      Serial.printf("MQTT Topic: %s, Payload: %s\n", MQTT_PUBLISH_TOPIC_RFID, rfid_payload);
      mqttclient.publish(MQTT_PUBLISH_TOPIC_RFID, rfid_payload);
    }
  } else {
    // Kein Tag erkannt -> Präsenz zurücksetzen
    rfid_present = false;
  }

  delay(100);                                                             
}

// --- WLAN Funktionen ---
void connectWiFi() {
  Serial.printf("Verbinde mit WLAN %s", ssid);                            
  WiFi.begin(ssid, pass);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {                
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
void connectMQTT() {                                                      
  Serial.printf("Verbinde mit MQTT Broker: %s\n", mqtt_broker);

  while (!mqttclient.connect(mqtt_client_id)) {                           
    Serial.println("Fehler beim Verbinden mit MQTT-Broker. Neuer Versuch in 2 Sekunden...");
    delay(2000);
  }

  Serial.println("Verbunden mit MQTT-Broker");
}
