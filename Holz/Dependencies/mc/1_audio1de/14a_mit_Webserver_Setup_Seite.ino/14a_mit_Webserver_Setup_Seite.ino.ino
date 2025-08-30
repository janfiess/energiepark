/*

Mit diesem Code erzeugt der ESP einen Acecss Point namens "ESP32-Setup" ohne Passwort.
Standardmässig muss man beim ESP dann im Browser 192.168.4.1 eintippen, damit man auf eine Config-Website kommt, 
auf der man Variablen im Code benennen kann, wie z. B. SSID und Passwort eines Netzwerks, mit dem sich der ESP verbinden soll. 
Achtung: Infos werden nicht persiostent gespeichert.

*/


#include <WiFi.h>
#include <WebServer.h>

// Variablen für die Ziel-WLAN-Daten (werden per Formular gesetzt)
String targetSSID = "";
String targetPassword = "";

// Lokaler Webserver auf Port 80
WebServer server(80);

// Funktionsprotot
String htmlPage();
void handleSave();
void handleRoot();



void setup() {
  delay(1000);
  Serial.begin(115200);

  // Eigenes WLAN starten (Access Point)
  WiFi.softAP("ESP32-Setup");  // ohne Passwort
  Serial.println("Access Point gestartet: ESP32-Setup");

  // Server Routen
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.begin();
  Serial.println("Webserver gestartet");
}

void loop() {
  // Webserver laufen lassen
  server.handleClient();

  // Verbindung prüfen
  if (targetSSID != "" && WiFi.status() == WL_CONNECTED) {
    Serial.println("Mit WLAN verbunden!");
    Serial.print("IP-Adresse: ");
    Serial.println(WiFi.localIP());
    delay(10000); // nicht spammen
  }
}


// HTML-Seite mit Formular
String htmlPage() {
  String page = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  page += "<title>ESP32-C6 Setup</title></head><body>";
  page += "<h2>WLAN Einstellungen</h2>";
  page += "<form action='/save' method='POST'>";
  page += "SSID: <input type='text' name='ssid'><br><br>";
  page += "Passwort: <input type='password' name='password'><br><br>";
  page += "<input type='submit' value='Speichern & Verbinden'>";
  page += "</form>";
  page += "</body></html>";
  return page;
}

// "/" → wenn 192.168.4.1 aufgerufen wird: Formular anzeigen (dieser Funktionsaufruf ist in setup() hinterlegt)
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

// "/save" → wenn 192.168.4.1/save aufgerufen wurd, also wenn Formular abgeschickt wurde: Daten speichern und WLAN verbinden (diese Funktion ist in setup() hinterlegt)
void handleSave() {
  if (server.method() == HTTP_POST) {
    if (server.hasArg("ssid")) {
      targetSSID = server.arg("ssid");
    }
    if (server.hasArg("password")) {
      targetPassword = server.arg("password");
    }

    // Rückmeldung
    String response = "<!DOCTYPE html><html><body>";
    response += "<h2>Daten gespeichert!</h2>";
    response += "<p>SSID: " + targetSSID + "</p>";
    response += "<p>Passwort: (versteckt)</p>";
    response += "<p>ESP versucht, sich zu verbinden...</p>";
    response += "</body></html>";

    server.send(200, "text/html", response);

    // Nach Senden der Antwort: mit WLAN verbinden
    delay(1000);
    WiFi.begin(targetSSID.c_str(), targetPassword.c_str());
  }
}

