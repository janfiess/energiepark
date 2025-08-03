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
 * GitHub: https://github.com/Interaktive-Medien/im_physical_computing/blob/main/09_Sensoren_testen/06_Distanzsensor_TOF/06_Distanzsensor_TOF.ino
***********************************************************************************************/



#include <Wire.h>
#include "Adafruit_VL6180X.h"

Adafruit_VL6180X tof = Adafruit_VL6180X();
int tof_maxdistance = 20; // 20 ~ 30mm -> Sensor misst ab ca 10mm
int tof_toleranz_entprellung = 5;

// --- Globale Variablen für die Zustandsverwaltung ---
bool tof_objectDetected_temp = false; // Aktueller Zustand (ohne Entprellung)
bool tof_objectDetected = false;           // Endgültiger Zustand (nach Entprellung)

// Array zum Speichern der letzten 3 Zustände für die Entprellung
bool tof_prevStates[5]; 
int tof_state_index = 0; // Index für das Array

void setup() {
  Serial.begin(115200);
  Wire.begin(6, 7); // I2C mit benutzerdefinierten Pins: SDA = GPIO6, SCL = GPIO7

  if (! tof.begin()) {
    Serial.println("Failed to find sensor");
    while (1);
  }
  Serial.println("Sensor found!");


  for(int i=0; i < tof_toleranz_entprellung; i++){
    tof_prevStates[i] = false;
  }
  // // Initialer Zustand basierend auf der ersten Messung
  // uint8_t tof_initial_range = tof.readRange();
  // if (tof_initial_range < tof_maxdistance) {
  //   tof_objectDetected_temp = true;
  //   tof_objectDetected = true;
  //   for (int i = 0; i < tof_prevStates; i++) {
  //     tof_prevStates[i] = true;
  //   }
  //   Serial.println("Objekt in der Nähe");
  // } else {
  //   tof_objectDetected_temp = false;
  //   tof_objectDetected = false;
  //   for (int i = 0; i < tof_prevStates; i++) {
  //     tof_prevStates[i] = false;
  //   }
  //   Serial.println("Kein Objekt in der Nähe");
  // }
}

void loop() {
  uint8_t tof_range = tof.readRange();
  // Serial.println(tof_range);
  uint8_t tof_status = tof.readRangeStatus();
  // Serial.println(tof_status);

  if (tof_status == VL6180X_ERROR_NONE) {
    // 1. Aktuellen Zustand bestimmen
    if (tof_range < tof_maxdistance) {
      tof_objectDetected_temp = true;
      // Serial.println("Object detected");
    } else {
      tof_objectDetected_temp = false;
    }

    // if(tof_objectDetected_temp == true){
    //   Serial.println("Object detected");
    // }
    

    // 2. Zustand für Entprellung speichern -> Mehrere Werte aufnehmen und miteinander vergleichen
    tof_prevStates[tof_state_index] = tof_objectDetected_temp;
    tof_state_index = (tof_state_index + 1) % 3; // Index zyklisch erhöhen (0, 1, 2, 0, ...)

    // 3. Entprellte Logik prüfen und Ausgabe steuern
    bool tof_all_true = true;
    bool tof_all_false = true;

    for (int i = 0; i < 3; i++) {
      if (!tof_prevStates[i]) {
        tof_all_true = false;
      }
      if (tof_prevStates[i]) {
        tof_all_false = false;
      }
    }

    // Wenn alle letzten 3 Werte gleich sind und der endgültige Zustand sich ändert
    if (tof_all_true && !tof_objectDetected) {
      tof_objectDetected = true;
      Serial.println("Objekt in der Nähe");
    } else if (tof_all_false && tof_objectDetected) {
      tof_objectDetected = false;
      Serial.println("Kein Objekt in der Nähe");
    }
  }

  delay(300); // Eine kurze Verzögerung ist gut, um den Sensor nicht zu überfordern
}
