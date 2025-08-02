/******************************************************************
 * Read NFC tags and RFID cards (I2C mode)
 * turn on I2C mode by switching physical switches on the PN532 to 1 / 0 (I2C)
 * Anschluss:
 * PN532: SDA <-> ESP32-C6: GPIO 6
 * PN532: SCL <-> ESP32-C6: GPIO 7
 * PN532: Vcc <-> ESP32-C6: 3.3V
 * PN532: GND <-> ESP32-C6: GND
 * Installiere Library "Adafruit_PN532" von Adafruit
 ********************************************************************/

#include <Wire.h>
#include <Adafruit_PN532.h>

// I2C Pins definieren
#define SDA_PIN 6
#define SCL_PIN 7

// IRQ und RESET Pins definieren – werden vom PN532-Modul NICHT verwendet bei I2C, aber Bibliothek erwartet sie
#define PN532_IRQ   (2)
#define PN532_RESET (3)

// Konstruktor mit IRQ, RESET und Wire
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET, &Wire);

// UID-Zwischenspeicher
uint8_t lastUid[7] = {0};
uint8_t lastUidLength = 0;
unsigned long lastPrintTime = 0;

void setup(void) {
  Serial.begin(115200);
  while (!Serial) delay(10);  // auf Serial warten

  Serial.println("PN532 NFC Reader Test (ESP32-C6, I2C)");

  // Wire starten mit den benutzerdefinierten I2C Pins
  Wire.begin(SDA_PIN, SCL_PIN);

  // PN532 starten
  nfc.begin();

  // Firmware-Version abfragen
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Kein PN532 gefunden – Verbindung prüfen.");
    while (1); // bleibt hängen, wenn nichts gefunden wird
  }

  // Chip-Daten anzeigen
  Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware Version: "); Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);

  // Konfiguriere das Modul für RFID-Lesen
  nfc.SAMConfig();
  Serial.println("Warte auf ein RFID/NFC Tag...");
}

void loop(void) {
  uint8_t uid[7];          // Buffer zum Speichern der UID
  uint8_t uidLength;       // Länge der UID

  // Versuche für 100ms, einen Tag zu erkennen
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100)) {
    // Prüfen, ob es der gleiche Tag ist wie vorher
    bool sameTag = (uidLength == lastUidLength);
    for (uint8_t i = 0; i < uidLength && sameTag; i++) {
      if (uid[i] != lastUid[i]) sameTag = false;
    }

    if (!sameTag) {
      // Neuer Tag → UID speichern und ausgeben
      memcpy(lastUid, uid, uidLength);
      lastUidLength = uidLength;
      Serial.print("Neuer Tag erkannt, UID: ");
      for (uint8_t i = 0; i < uidLength; i++) {
        Serial.print(uid[i], HEX); Serial.print(" ");
      }
      Serial.println();
      lastPrintTime = millis();
    } else if (millis() - lastPrintTime >= 1000) {    // selber NFC-Tag
      // Gleicher Tag → UID periodisch ausgeben
      Serial.print("Gleicher Tag noch da, UID: ");
      for (uint8_t i = 0; i < uidLength; i++) {
        Serial.print(uid[i], HEX); Serial.print(" ");
      }
      Serial.println();
      lastPrintTime = millis();
    }
  } else {
    // Kein Tag erkannt → UID zurücksetzen
    if (lastUidLength != 0) {
      Serial.println("Kein Tag mehr erkannt.");
      lastUidLength = 0;
    }
  }

  delay(100); // kurze Pause, entlastet den Prozessor
}
