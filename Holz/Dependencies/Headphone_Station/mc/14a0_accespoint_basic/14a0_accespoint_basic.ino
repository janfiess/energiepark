#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nStarte Access Point...");
  WiFi.softAP("ESP32-Test");
  IPAddress apIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(apIP);
}

void loop() {
  // Nichts zu tun im Loop
}