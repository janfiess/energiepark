/**********************************************************************************************
*  06_Distanzsensor_TOF.ino
*  TOF050C Time-of-flight sensor mit V16180x
*  Messe Entfernung zwischen 1.5 und 50cm und drucke sie auf den seriellen Port
*  Installiere Adafruit_VL6180X Library von Adafruit.
*  4 Anschlüsse an ESP32-C6 (Kommunikation per I2C -> dafür braucht es die vorinstallierte Wire-Library):
*  Sensor: Vin  <->  ESP32-C6: 3.3V 
*  Sensor: GND  <->  ESP32-C6: GND
*  Sensor: SDA  <->  ESP32-C6: GPIO6
*  Sensor: SCL  <->  ESP32-C6: GPIO7
***********************************************************************************************/



#include <Wire.h>
#include "Adafruit_VL6180X.h"

Adafruit_VL6180X tof = Adafruit_VL6180X();
int tof_maxdistance = 20;                                                // 20 ~ 30mm -> Sensor misst ab ca 10mm


// --- Globale Variablen für die Zustandsverwaltung ---
bool tof_objectDetected_temp = false;                                    // Aktueller Zustand (ohne Entprellung)
bool tof_objectDetected = false;                                         // Endgültiger Zustand (nach Entprellung) -> Wert gilt als endgültig, sobald mehrmals derselbe Wert erkannt wurde

// Array zum Speichern der letzten Zustände für die Entprellung
const int tof_toleranz_entprellung = 5;
bool tof_prevStates[tof_toleranz_entprellung]; 
int tof_state_index = 0;                                                 // Index für das Array
int tof_timestamp_prev_detected = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin(6, 7);                                                       // I2C mit benutzerdefinierten Pins: SDA = GPIO6, SCL = GPIO7

  if (! tof.begin()) {
    Serial.println("Failed to find sensor");
    while (1);
  }
  Serial.println("Sensor found!");


  for(int i=0; i < tof_toleranz_entprellung; i++){
    tof_prevStates[i] = false;
  }
}

void loop() {
  uint8_t tof_range = tof.readRange();
  uint8_t tof_status = tof.readRangeStatus();

  if (tof_status == VL6180X_ERROR_NONE) {


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
      Serial.println("Objekt in der Nähe");
    } else if (tof_none_detected && tof_objectDetected) {
      tof_objectDetected = false;
      Serial.println("Kein Objekt in der Nähe");
    }
  }

  delay(100);                                                               // Eine kurze Verzögerung ist gut, um den Sensor nicht zu überfordern
}
