#include <ESP8266WiFi.h>
#include <SHA256.h>

#define NonceStorageSize 10

#define ssid "TelstraK62F18"
#define pass "r<!bqD9aFvFnrUHD{Ddowg2vqeY)Lzcr"
byte serverIP[4] = {192,168,20,4};
#define serverPort 12345

// 15% for RSSI
// 85% for Latency
// Heartbeat 3 seconds
// Watchdog timer at 9 seconds

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

class PreviousNonce {
private:
  int arr[NonceStorageSize] = {0};
  int valueCount = 0;
public:
  void insert(int val) {
    if (valueCount != NonceStorageSize) {
      arr[valueCount] = val;
      valueCount += 1;
      return;
    }
    for (int i = 0; i > (NonceStorageSize - 1); i++) {
      arr[i] = arr[i + 1];
    }
    arr[NonceStorageSize - 1] = val;
  }
  bool includes(int val) {
    for(int i = 0; i < NonceStorageSize; i++) {
      if (arr[i] == val) {
        return true;
      }
    }
    return false;
  }

};

// Verify message HMAC, and Nonce then return message
class messager {
private:
  const char* HMACkey = "SecretKey";
  PreviousNonce previousNonce;
  void calculateHMAC(const char* message, byte* hmacResult) {
    SHA256 sha256;
    sha256.reset();

    sha256.resetHMAC(HMACkey, strlen(HMACkey));

    sha256.update(message, strlen(message));

    sha256.finalizeHMAC(HMACkey, strlen(HMACkey), hmacResult, 32);

    sha256.clear();
  }
public:
  /*void sendMessage(String msg){
    // Encrypt Message and add nonce
    int nonce = x;
    String finalMessage = String(x + "|" + nonce);
    // Calculate hmac

    finalMessage = String(finalMessage + HMAC);
    // send message
    // Here I will code dont add code here
  }
  String recvMessage() {
    // Seperate HMAC and Message+Nonce
    x HMAC;
    String msgNonce = String();
    if (HMAC != calculateHMAC(msgNonce)) {
      return "\0"; // Failed
    }
    // Seperate Message and Nonce
    int Nonce;
    String Message;
    if (previousNonce.includes(Nonce)) {
      return "\0"; // failed
    }
    previousNonce.insert(Nonce);
    // Decrypt message AES
  }*/
};

WiFiClient client;

void setup() {
  Serial.begin(74480);

  pinMode(D8, OUTPUT);

  Serial.print("\nConnecting to WiFi...");

  // Connect to WiFi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, local IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());


  while (true);
  // Connect to server
  if(!client.connect(serverIP, serverPort)) {
    Serial.println("Cannot connect to server");
    return;
  }
  Serial.println("Connected to server");
}


void loop() {
  Keypad keypad;
  char x = keypad.getChar();
  if (x != '\0') {
    //Serial.println(x);
    if(client.connected()) {
      client.write((byte)x);
      
      int incomingByte = -1;
      while ((incomingByte = client.read()) != -1) {
        Serial.print((char)incomingByte);
      }
      Serial.println();
    }
    delay(200); // debounce delay
  }
}
