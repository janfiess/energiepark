# Energiepark Grischa

## Kopfhörerstationen

### MC -> TouchDesigner

Play/pause Audio 


* MQTT Topic: holz/player/audio/1     Payload: play oder pause
* MQTT Topic: holz/player/audio/2     Payload: play oder pause
* MQTT Topic: holz/player/audio/3     Payload: play oder pause
* MQTT Topic: holz/player/audio/4     Payload: play oder pause
* MQTT Topic: holz/player/audio/5     Payload: play oder pause
* MQTT Topic: holz/player/audio/6     Payload: play oder pause

TouchDesigner -> MC

### Drive LED-Ring sections (12 LEDs):

* MQTT Topic: holz/ledring/numsections/1     Payload: 0 - 12
* MQTT Topic: holz/ledring/numsections/2     Payload: 0 - 12
* MQTT Topic: holz/ledring/numsections/3     Payload: 0 - 12
* MQTT Topic: holz/ledring/numsections/4     Payload: 0 - 12
* MQTT Topic: holz/ledring/numsections/5     Payload: 0 - 12
* MQTT Topic: holz/ledring/numsections/6     Payload: 0 - 12

Notification when track ended, wenn der Kopfhörer eingehängt wurde (denn RFID Tag erkannt wurde):

* MQTT Topic:  ledring/state/1          Payload: leds_idle
* MQTT Topic:  ledring/state/2          Payload: leds_idle
* MQTT Topic:  ledring/state/3          Payload: leds_idle
* MQTT Topic:  ledring/state/4          Payload: leds_idle
* MQTT Topic:  ledring/state/5          Payload: leds_idle
* MQTT Topic:  ledring/state/6          Payload: leds_idle

RFID Tags:
Jeder Kopfhörer ist mit 3 RFID Tags ausgestattet. Mindestens einer muss erkannt werden.

Kopfhörer 1:
* 14AA77D6 (K1A)
* A44347D0 (K1B)
* ....

Kopfhörer 2:
* 241A5FD6 (K2A)
* F40735D6 (K2B)
* ...

Kopfhörer 3:
* 94326BD6 (K3A)
* 271652FA (K3B)
* ...

Kopfhörer 4:
* 74A06BD6 (K4A)
* 27E655FA (K4B)
* ...

Kopfhörer 5:
* 747E6AD6 (K5A) 
* 477251FA (K5B)
* ...

Kopfhörer 6:
* C4B86DD6 (K6A)
* D7091DFA (K6B)
* ...





## RFID-Scan-Station

### MQTT: MC -> TouchDesigner

Topic: holz/rfid     Payload: [id]


### RFID Tags:

Objekt 1 (Kohle klein):
* A5377800 (S1A)
* 1B047A00 (S1B)

Objekt 2 (Kohle gross):
* 57DD7900 (S2A)
* FA9B7800 (S2B)

Objekt 3 (Heizöl):
* 01107B00 (S3A)
* C6937A00 (S3B)

Objekt 4 (Holz):
* 67BC7A00 (S4A)
* E5BA7800 (S4B)

Objekt 5 (tbd):
* 492F7A00 (S5A)
* 35727D00 (S5B)



## LED-Strip

Bespielung via Art-Net.
Adresse LED-Controller (ebenfalls im Switch-Netzwerk): 	192.168.50.195

