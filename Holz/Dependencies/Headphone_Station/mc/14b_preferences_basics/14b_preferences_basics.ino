#include <Preferences.h>

Preferences preferences;

void setup() {
  Serial.begin(115200);
  delay(3000);

  // Preferences-Namespace öffnen (z. B. "wifi")
  preferences.begin("wifi", false);

  // SSID und Passwort speichern
  preferences.putString("ssid", "Energiepark Technik");
  preferences.putString("password", "Energiepark2025.8");

  preferences.end();
}

void loop() {
  preferences.begin("wifi", true); // read-only öffnen

  String ssid = preferences.getString("ssid", "(leer)");
  String password = preferences.getString("password", "(leer)");

  preferences.end();

  Serial.println(ssid);
  Serial.println(password);
  delay(4000);
}
