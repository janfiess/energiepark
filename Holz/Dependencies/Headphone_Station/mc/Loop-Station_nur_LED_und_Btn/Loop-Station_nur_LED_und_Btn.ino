/*****************************************************************
 * Energiepark: Audiostationen im Loop
 * LED Ring leuchtet statisch und Button erursacht einen Reset.
 *
 * LED-Ring 
 * WS2812B-Ring: Data In  <->  ESP32-C6: GPIO 5
 * WS2812B-Ring: +5V/Vcc  <->  ESP32-C6: 5V
 * WS2812B-Ring: GND      <->  ESP32-C6: GND

 * Taster
 * Taster Pin 1   <->  ESP32-C6: 3.3V
 * Taster Pin 2   <->  ESP32-C6: GPIO 10
 *
 * Installiere folgende Libraries: 
 * -  "Adafruit_NeoPixel" von Adafruit
******************************************************************/


#include <Adafruit_NeoPixel.h>                                         // LED-Ring

// LED-Ring
#define LED_PIN 5
#define NUM_PIXELS 12
#define LED_BRIGHTNESS 50
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);


// Taster
#define TASTER_PIN 10
int buttonState = 0;         
int prev_buttonState = 0;

void read_button();
void init_led_ring();

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Init LED-Ring
  init_led_ring();

  // Init Taster
  pinMode(TASTER_PIN, INPUT_PULLDOWN);
  Serial.println("Setup abgeschlossen."); 
}

void loop() {  
  read_button();                                                          // Alterative: Taster statt RFID-Erkennung
  delay(100);                                                             // kurze Pause, entlastet den Prozessor
}

/****************************************************************************************************
 * Taster lesen
 ****************************************************************************************************/ 

void read_button(){
  buttonState = digitalRead(TASTER_PIN);
  if(buttonState != prev_buttonState) {
    prev_buttonState = buttonState;
    if (buttonState == 1) {
      ESP.restart();
    }
  }
}

/****************************************************************************************************
 * LED-Ring
 ****************************************************************************************************/ 

void init_led_ring(){
  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS);
  strip.clear();
  for (int i = 0; i < 12; i++) {  
    strip.setPixelColor(i, strip.Color(20, 40, 0));                 // Werte: 0 - 255
  }
  strip.show();  
}