 /******************************************************************
  * Read NFC tags and RFID cards (I2C mode)
  * Das Programm reagiert nur auf den einen festgelegten UID. "FF F 17 E9 3F 0 0" ist { 0xFF, 0x0F, 0x17, 0xE9, 0x3F, 0x00, 0x00 };
  * Das Script prüft periodisch, ob dieser Tag noch verfügbar ist.
  * In der Konsole wird nur geprintet, wann die "Session" beginnt und wann sie endet.
  * Für etwas oleranz wird der Wert mit den letzten Werten verglichen. Um Fehlimpulse zu vermeiden, wird die Console erst beschrieben,
  * sobald die aktuelle Messung mit den letzten paar Messungen übereinstimmt.
  *
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

// Ziel-UID definieren (7 Bytes)
const uint8_t TARGET_UID[] = { 0xFF, 0x0F, 0x17, 0xE9, 0x3F, 0x00, 0x00 };
const uint8_t NFC_TARGET_UID_LENGTH = 7;

// --- Statusvariablen für die verbesserte Erkennung ---
bool nfc_tagCurrentlyPresent = false; // Merkt sich, ob der TARGET_UID-Tag aktuell als "anwesend" gilt
int nfc_presentConfirmCounter = 0;    // Zählt aufeinanderfolgende erfolgreiche Lesungen des TARGET_UID
int nfc_absentConfirmCounter = 0;     // Zählt aufeinanderfolgende fehlgeschlagene Lesungen (Tag nicht da)

// Schwellenwerte für die Bestätigung
// Der Tag muss X-mal hintereinander korrekt gelesen werden, um als "anwesend" zu gelten.
const int NFC_CONFIRM_PRESENT_THRESHOLD = 3; // Z.B. 3 erfolgreiche Lesungen in Folge
// Der Tag muss X-mal hintereinander NICHT gelesen werden, um als "entfernt" zu gelten.
const int NFC_CONFIRM_ABSENT_THRESHOLD = 5;  // Z.B. 5 fehlgeschlagene Lesungen in Folge


void setup(void) {
  Serial.begin(115200);
  while (!Serial) delay(10);                         // auf Serial warten
  Serial.println("PN532 NFC Reader Test");
  Wire.begin(SDA_PIN, SCL_PIN);                      // Wire starten mit den benutzerdefinierten I2C Pins
  nfc.begin();                                       // PN532 starten
  uint32_t nfc_versiondata = nfc.getFirmwareVersion();   // Firmware-Version abfragen
  if (!nfc_versiondata) {
    Serial.println("Kein PN532 gefunden – Verbindung prüfen.");
    while (1);                                       // bleibt hängen, wenn nichts gefunden wird
  }

  // Chip-Daten anzeigen
  Serial.print("Found chip PN5"); Serial.println((nfc_versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware Version: "); Serial.print((nfc_versiondata >> 16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((nfc_versiondata >> 8) & 0xFF, DEC);

  // Konfiguriere das Modul für RFID-Lesen
  nfc.SAMConfig();
  Serial.println("Warte auf ein RFID/NFC Tag (UID: FF 0F 17 E9 3F 00 00)...");
}

void loop(void) {
  uint8_t uid[7];          // Buffer zum Speichern der UID
  uint8_t nfc_uidLength;       // Länge der UID

  bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &nfc_uidLength, 100);   // Versuche für 100ms, einen Tag zu erkennen. Der Timeout sollte nicht zu kurz sein, da der PN532 Zeit benötigt..
  bool nfc_isTargetTag = false;  

  // Prüfen, ob die erkannte UID der gewünschten UID entspricht
  if (success) {
    bool nfc_currentMatch = (nfc_uidLength == NFC_TARGET_UID_LENGTH);   // Prüfe erst ob die Längen der erkannten und der gewünschten IDs übereinstimmen
    for (uint8_t i = 0; i < nfc_uidLength && nfc_currentMatch; i++) {   // Prüfe jetzt Byte für Byte, ob sie übereinstimmen
      if (uid[i] != TARGET_UID[i]) nfc_currentMatch = false;
    }
    nfc_isTargetTag = nfc_currentMatch;                                 // Setze isTargetTag basierend auf dem Vergleich
  }

  // --- Logik für die Bestätigung von Anwesenheit/Abwesenheit ---
  if (nfc_isTargetTag) {
    // Wenn der Ziel-Tag gerade erfolgreich gelesen wurde
    nfc_absentConfirmCounter = 0;                                       // Reset des Abwesenheitszählers
    nfc_presentConfirmCounter++;                                        // Inkrementiere Anwesenheitszähler

    if (nfc_presentConfirmCounter >= NFC_CONFIRM_PRESENT_THRESHOLD) {
      // Wenn der Tag oft genug hintereinander bestätigt wurde
      nfc_presentConfirmCounter = NFC_CONFIRM_PRESENT_THRESHOLD;        // Zähler auf Max setzen, um Überlauf zu vermeiden
      if (!nfc_tagCurrentlyPresent) {  
        Serial.print("Ziel-Tag erkannt (stabilisiert), UID: ");         // Wenn der Tag vorher nicht als anwesend galt, jetzt aber schon
        for (uint8_t i = 0; i < nfc_uidLength; i++) {
          Serial.print(uid[i], HEX); Serial.print(" ");
        }
        Serial.println();
        nfc_tagCurrentlyPresent = true; // Status auf "anwesend" setzen
      }
    }
  } else {
    // Wenn der Ziel-Tag NICHT gelesen wurde (entweder kein Tag oder falscher Tag)
    nfc_presentConfirmCounter = 0; // Reset des Anwesenheitszählers
    nfc_absentConfirmCounter++;    // Inkrementiere Abwesenheitszähler

    if (nfc_absentConfirmCounter >= NFC_CONFIRM_ABSENT_THRESHOLD) {
      // Wenn der Tag oft genug hintereinander NICHT gelesen wurde
      nfc_absentConfirmCounter = NFC_CONFIRM_ABSENT_THRESHOLD; // Zähler auf Max setzen
      if (nfc_tagCurrentlyPresent) {
        // Wenn der Tag vorher als anwesend galt, jetzt aber nicht mehr
        Serial.println("Ziel-Tag entfernt (stabilisiert).");
        nfc_tagCurrentlyPresent = false; // Status auf "nicht anwesend" setzen
      }
    }
  }

  delay(100); // kurze Pause, entlastet den Prozessor
}