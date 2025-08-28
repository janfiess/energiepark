#include <Wire.h>
#include <Adafruit_PN532.h>

// I2C Pins definieren
#define SDA_PIN 6
#define SCL_PIN 7

#define PN532_IRQ   (2)
#define PN532_RESET (3)

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET, &Wire);

bool tagDetected = false;
int lostCounter = 0;

// *** PARAMETER ANPASSEN ***
// Erhöhe den Schwellenwert, um das System robuster zu machen.
const int lostThreshold = 25; // Alter Wert: 10

void setup(void) {
  Serial.begin(115200);
  while (!Serial) delay(100);
  Serial.println("huarageil");
  Serial.println("PN532 Reader: Warte auf Tag...");
  Wire.begin(SDA_PIN, SCL_PIN);
  nfc.begin();
  if (!nfc.getFirmwareVersion()) {
    Serial.println("Kein PN532 gefunden!");
    while (1);
  }
  nfc.SAMConfig();
}

void loop(void) {
  uint8_t uid[7];
  uint8_t uidLength;

  bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100);
  Serial.println(success);

  if (success) {
    lostCounter = 0;
    if (!tagDetected) {
      tagDetected = true;
      Serial.print("Kopfhörer eingehängt – UID: ");
      for (uint8_t i = 0; i < uidLength; i++) {
        Serial.print(uid[i], HEX); Serial.print(" ");
      }
      Serial.println();
    }
  } else {
    if (tagDetected) {
      lostCounter++;
      
      // DEBUG-AUSGABE: Zeigt, wie die Fehlschläge gezählt werden
      Serial.print("Leseversuch fehlgeschlagen, Zähler: ");
      Serial.println(lostCounter);
      
      if (lostCounter >= lostThreshold) {
        tagDetected = false;
        Serial.println("Kopfhörer entfernt");
      }
    }
  }

  // *** PAUSE ANPASSEN ***
  // Gib dem System etwas mehr Zeit zwischen den Leseversuchen.
  delay(250); // Alter Wert: 200
}