/******************************************************************************************************
 * 00_Button.ino
 * receive eg.  button / reed sensor input 
 * and print value to serial port
 " Connect ...
 * Sensor: Vin       <->  ESP32-C6: 3.3V 
 * Sensor: GND/Data  <->  ESP32-C6: GPIO10
 ******************************************************************************************************/

const int buttonPin = 10;
int buttonState = 0;         
int prev_buttonState = 0;

void setup() {  
    Serial.begin(115200); 
    // initialize the pushbutton pin as an input:
}

void loop(){
    buttonState = digitalRead(buttonPin);
    if(buttonState == prev_buttonState) {
        return;
    }
    prev_buttonState = buttonState;
    if (buttonState == 1) {
        Serial.println(1);
    } else {
        Serial.println(0);
    } 
}
