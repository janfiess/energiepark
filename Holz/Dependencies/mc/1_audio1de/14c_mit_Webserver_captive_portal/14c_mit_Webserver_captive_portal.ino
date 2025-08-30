/*

Mit diesem Code erzeugt der ESP einen Access Point namens "ESP32-Setup" ohne Passwort.
Mit diesem Code kommt man direkt auf die Config-Website, ohne dass man die IP Adresse eintippen muss, wie im Hotel-WLAN ("Captive Portal").
Dazu starten wir einen DNS-Server, der alle Domains auf die ESP-IP 192.168.4.1 umleitet.
Auf der Setup-Website kann man dann wie gehabt Variablen im Code benennen, wie z. B. SSID und Passwort eines Netzwerks, mit dem sich der ESP verbinden soll. 
Das wird dann im Flash Speicher des ESP gespeichert, damit er sich diese Info beim nächsten Start direkt abrufen kann 
und sich z. B. ohne Nachfrage mit einem Netzwerk verbinden kann.

*/


#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <DNSServer.h>                  // @neu für Captive Portal

Preferences preferences;

// Variablen für die Ziel-WLAN-Daten - wird auf der Setup-Website des ESP-Access Points eingegeben.
String targetSSID = "";
String targetPassword = "";

bool apMode = false;

// Lokaler Webserver auf Port 80
WebServer server(80);
DNSServer dnsServer;                    // @neu für Captive Portal
const byte DNS_PORT = 53;               // @neu für Captive Portal.  // DNS-Server: Alle Anfragen -> ESP32

// Funktionsprotot
String htmlPage();
void handleSave();
void handleRoot();


void setup() {
  delay(1000);
  Serial.begin(115200);

  // Gespeicherte WLAN-Daten laden, falls bereits per Setup-Seite festgelegt

  preferences.begin("wifi", true);
  targetSSID = preferences.getString("ssid", "");
  targetPassword = preferences.getString("password", "");
  preferences.end();

  // ... und dann mit diesen Infos mit dem WLAN verbinden

  if (targetSSID != "") {
    Serial.print("Versuche Verbindung mit gespeichertem WLAN: ");
    Serial.println(targetSSID);
    WiFi.begin(targetSSID.c_str(), targetPassword.c_str());
    unsigned long startAttemptTime = millis();

    // 10 Sekunden lang versuchen
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
      delay(500);
      Serial.print(".");
    }
    Serial.println();
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Erfolgreich verbunden!");
    Serial.print("IP-Adresse: ");
    Serial.println(WiFi.localIP());
    apMode = false;
  } 
  
  // Falls keine Verbindung zustande kommt (z. B. weil SSID und PW auf der Setup-Seite noch nicht eingetragen wurden), starte den Access Point

  else {
    Serial.println("Keine Verbindung, starte Access Point...");

    // Eigenes WLAN starten (Access Point)
    WiFi.softAP("ESP32-Setup");                   // offen, ohne Passwort
    // Serial.println("Access Point gestartet: ESP32-Setup");

    IPAddress apIP = WiFi.softAPIP();           // @neu für Captive Portal
    Serial.print("AP gestartet, IP: ");
    Serial.println(apIP);                       // @neu für Captive Portal

    // DNS-Server: Alle Domains -> ESP
    dnsServer.start(DNS_PORT, "*", apIP);

    // Webserver starten
    server.on("/", handleRoot);         // falls 192.168.4.1 aufgerufen wurde, führe die Funktion handleRoot() aus
    server.on("/save", handleSave);     // falls 192.168.4.1/save aufgerufen wurde, führe die Funktion handleSave() aus
    server.onNotFound(handleRoot);              // @neu für Captive Portal       alles auf Setup-Seite leiten
    server.begin();
    Serial.println("Webserver + DNS gestartet (Captive Portal aktiv)");

    apMode = true;   // wir sind im AP-Modus
  }

}

void loop() {
  dnsServer.processNextRequest();                // @neu für Captive Portal
  server.handleClient();



  // Nur im STA-Modus reconnecten!
  if (!apMode && targetSSID != "" && WiFi.status() != WL_CONNECTED) {
      Serial.println("Versuche Verbindung mit gespeicherten Daten...");
      WiFi.begin(targetSSID.c_str(), targetPassword.c_str());
      delay(10000);
  }





}




// HTML-Seite mit Formular an den aufrufenden Client schicken. Diese Funktion wird aufgerufen in handleRoot()
String htmlPage() {
  String page = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  page += "<title>ESP32-C6 Setup</title></head><body>";
  page += "<h2>WLAN Einstellungen</h2>";
  page += "<form action='/save' method='POST'>";
  page += "SSID: <input type='text' name='ssid' value='" + targetSSID + "'><br><br>";
  page += "Passwort: <input type='password' name='password' value='" + targetPassword + "'><br><br>";
  page += "<input type='submit' value='Speichern & Verbinden'>";
  page += "</form>";
  page += "</body></html>";
  return page;
}

// "/" → wenn 192.168.4.1 aufgerufen wird: Formular anzeigen (diese Funktion ist in setup() hinterlegt)
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

// "/save" → wenn 192.168.4.1/save aufgerufen wurd, also wenn Formular abgeschickt wurde: Daten speichern und WLAN verbinden (dieser Funktionsaufruf ist in setup() hinterlegt)
void handleSave() {
  if (server.method() == HTTP_POST) {
    if (server.hasArg("ssid")) {
      targetSSID = server.arg("ssid");
    }
    if (server.hasArg("password")) {
      targetPassword = server.arg("password");
    }

    // Im Flash speichern
    preferences.begin("wifi", false);
    preferences.putString("ssid", targetSSID);
    preferences.putString("password", targetPassword);
    preferences.end();

    // Rückmeldung
    String response = "<!DOCTYPE html><html><body>";
    response += "<h2>Daten gespeichert!</h2>";
    response += "<p>SSID: " + targetSSID + "</p>";
    response += "<p>Passwort: (versteckt)</p>";
    response += "<p>ESP versucht, sich zu verbinden...</p>";
    response += "</body></html>";

    server.send(200, "text/html", response);

    // WLAN Verbindung starten
    delay(1000);
    WiFi.begin(targetSSID.c_str(), targetPassword.c_str());



    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.println("Direkt nach Setup verbunden!");
      Serial.print("IP-Adresse: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println();
      Serial.println("Konnte sich nach Speichern nicht sofort verbinden.");
    }





  }
}
