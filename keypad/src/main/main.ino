#include <ESP8266WiFi.h>

#define SSID ""
#define PASS ""

// 15% for RSSI
// 85% for Latency
// Heartbeat 3 seconds
// Watchdog timer at 9 seconds


//const uint16_t port = 12345;

class Keypad {
private:
  const char keypadOutputs[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
  };
  const int colPins[4] = {D0, D1, D2, D3}; // Column pins
  const int rowPins[4] = {D4, D5, D6, D7}; // Row pins
public:
  Keypad() {
    for(int i = 0; i < 4; i++){
      pinMode(colPins[i], OUTPUT);
      digitalWrite(colPins[i], HIGH); // Set column pins to HIGH by default
      pinMode(rowPins[i], INPUT_PULLUP);
    }
  }
  char getChar() {
    for (int col = 0; col < 4; col++) { // Set each column to high individually
      digitalWrite(colPins[col], LOW);
      for (int row = 0; row < 4; row++) { // Read each row individually if there is a short
        if (digitalRead(rowPins[row]) == LOW) {
          return keypadOutputs[col][row];
        }
      }
      digitalWrite(colPins[col], HIGH); 
    }
    return '\0'; // Return null char if no key pressed
  }
};


void setup() {
  Serial.begin(74480);

  pinMode(D8, OUTPUT);

  Serial.print("\nConnecting to WiFi...");

  WiFi.begin(SSID, PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, local IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());
}


void loop() {
  Keypad keypad;
  char x = keypad.getChar();
  if (x != '\0') {
    Serial.println(x);
    delay(200); // debounce delay
  }
}
