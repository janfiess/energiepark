/**********************************************************************************************
*  ICM20948 9-Degrees-of-Freedom
*  Messe Neigung an zwei Achsen (Beschleunigungssensor + Gyroskop)
*  und sende sie via MQTT an den Broker
*
*  Sensor: Vin  <->  ESP32-C6: 3.3V 
*  Sensor: GND  <->  ESP32-C6: GND
*  Sensor: SDA  <->  ESP32-C6: GPIO6
*  Sensor: SCL  <->  ESP32-C6: GPIO7
*
*  Installiere ICM20948_WE Library von Wolfgang Ewald.
*  4 Anschlüsse an ESP32-C6 (Kommunikation per I2C -> dafür braucht es die vorinstallierte Wire-Library):
***********************************************************************************************/

#include <WiFi.h>
#include <MQTT.h>
#include <Wire.h>
#include <ICM20948_WE.h>
#define ICM20948_ADDR 0x68

ICM20948_WE myIMU = ICM20948_WE(ICM20948_ADDR);

// WLAN und MQTT Einstellungen
const char* ssid = "Energiepark Technik";                                // @todo: add your wifi name "FRITZ!Box 6690 TA"
const char* pass = "Energiepark2025.8";                                // @todo: add your wifi pw, "79854308499311013585"
const char* mqtt_broker = "192.168.178.27";                         // z.B. "broker.emqx.io", "192.168.0.80"
const char* mqtt_client_id = "sonne_lagesensor";
const char* MQTT_PUBLISH_TOPIC_RFID = "sonne/lagesensor";         // Topics.  -   Diese werden für Subscribing und Publishing genutzt

WiFiClient wificlient;
MQTTClient mqttclient;

// Funktionsprototypen
void connectWiFi();
void connectMQTT();

void setup() {
  //delay(2000); // maybe needed for some MCUs, in particular for startup after power off
  Wire.begin(6, 7);
  Serial.begin(115200);
  while (!Serial) {}

  // WLAN verbinden
  connectWiFi();

  // --- MQTT Client initialisieren und Callback setzen ---
  // Diese Zeilen müssen hier stehen und nicht in connectMQTT()
  mqttclient.begin(mqtt_broker, wificlient);
  connectMQTT();

  // ICM20948 initialisieren
  if (!myIMU.init()) {
    Serial.println("ICM20948 does not respond");
  }
  else {
    Serial.println("ICM20948 is connected");
  }

  Serial.println("Position your ICM20948 flat and don't move it - calibrating...");
  delay(1000);
  myIMU.autoOffsets();  // Automatische Kalibrierung
  Serial.println("Done!"); 

  // Beschleunigungssensor konfigurieren
  myIMU.setAccRange(ICM20948_ACC_RANGE_2G);
  myIMU.setAccDLPF(ICM20948_DLPF_6);    
  myIMU.setAccSampleRateDivider(10);
}

void loop() {
  // MQTT Loop regelmässig aufrufen, um die Verbindung am Leben zu halten und Nachrichten zu empfangen  
  mqttclient.loop();
  if (!mqttclient.connected()) {                                        
    Serial.println("MQTT Verbindung verloren. Versuche Neuverbindung...");
    connectMQTT();
  }

  xyzFloat gValue;
  xyzFloat angle;
  myIMU.readSensor();
  myIMU.getGValues(&gValue);
  myIMU.getAngles(&angle);

  /* Angles are also based on the corrected raws. Angles are simply calculated by
     angle = arcsin(g Value) */
  
  // Aus x, y, z Werten einen einzigen String im Format "x:WERT,y:WERT,z:WERT" erzeugen
  char payload[64]; // Puffer für die Zeichenkette, ausreichend groß für drei float-Werte
  snprintf(payload, sizeof(payload), "x:%.2f,y:%.2f", angle.x, angle.y);

  // Ausgabe im Serial Monitor
  Serial.println(payload);

  // String per MQTT an den Broker senden
  mqttclient.publish(MQTT_PUBLISH_TOPIC_RFID, payload);

  delay(300); // kleine Pause zwischen Messungen
}

// --- WLAN Funktionen ---
void connectWiFi() {
  Serial.printf("Verbinde mit WLAN %s", ssid);
  WiFi.begin(ssid, pass);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) { // Max 20 Versuche (10 Sekunden)
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

  // Versuche, eine Verbindung herzustellen, bis sie erfolgreich ist
  while (!mqttclient.connect(mqtt_client_id)) {                           
    Serial.println("Fehler beim Verbinden mit MQTT-Broker. Neuer Versuch in 2 Sekunden...");
    delay(2000);
  }

  Serial.println("Verbunden mit MQTT-Broker");
}
