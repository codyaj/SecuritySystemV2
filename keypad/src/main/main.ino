#include <ESP8266WiFi.h>
#include <SHA256.h>
#include <AES.h>

#define NonceStorageSize 10

#define ssid "TelstraK62F18"
#define pass "r<!bqD9aFvFnrUHD{Ddowg2vqeY)Lzcr"
byte serverIP[4] = {192,168,20,4};
#define serverPort 12345

#define RSSIRange 1.15
#define LatencyRange 1.85
// Heartbeat 3 seconds
// Watchdog timer at 9 seconds

WiFiClient client;


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
  byte arr[NonceStorageSize][15] = {0};
  int valueCount = 0;

  bool compareNonce(const byte* nonce1, const byte* nonce2) const {
    return memcmp(nonce1, nonce2, 15) == 0;
  }
public:
  void insert(byte* val) {
    if (valueCount != NonceStorageSize) {
      memcpy(arr[valueCount], val, 15);
      valueCount++;
    } else {
      // Shift elements to the left
      for (int i = 0; i < (NonceStorageSize - 1); i++) {
        memcpy(arr[i], arr[i + 1], 15);
      }
      // Insert the new nonce at the end
      memcpy(arr[NonceStorageSize - 1], val, 15);
    }
  }
  bool includes(byte* val) {
    for(int i = 0; i < NonceStorageSize; i++) {
      if (compareNonce(arr[i], val)) {
        return true;
      }
    }
    return false;
  }
};

class Messenger {
private:
  const char* HMACkey = "SecretKey";
  const byte key[16] = {0xBA, 0x7D, 0x66, 0x18, 0x5B, 0xD7, 0x88, 0xCD, 0xA1, 0x50, 0xCD, 0x3A, 0xB3, 0xB4, 0x19, 0xA5};
  PreviousNonce previousNonce;

  void calculateHMAC(const char* message, byte* hmacResult) {
    SHA256 sha256;
    sha256.reset();
    sha256.resetHMAC(HMACkey, strlen(HMACkey));
    sha256.update(message, strlen(message));
    sha256.finalizeHMAC(HMACkey, strlen(HMACkey), hmacResult, 32);
    sha256.clear();
  }

  void encryptData(const byte data, const byte* nonce, byte* ciphertext) {
    AES128 aes;
    aes.setKey(key, 16);

    byte block[16];

    // Concatenate data and nonce
    block[0] = data;
    memcpy(block + 1, nonce, 15);

    aes.encryptBlock(ciphertext, block);
  }

  void decryptData(const byte* ciphertext, byte* decryptedData, byte* nonce) {
    AES128 aes;
    aes.setKey(key, 16); // Set AES key

    byte block[16]; // AES block size is 16 bytes

    // Decrypt the 16-byte block
    aes.decryptBlock(block, ciphertext);

    // Extract data and nonce from the decrypted block
    *decryptedData = block[0]; // The data is in the first byte
    memcpy(nonce, block + 1, 15); // The nonce is in the remaining 15 bytes
  }

  void generateNonce(byte nonce[15]) {
    for (int i = 0; i < 16; i++) {
      nonce[i] = random(0, 255);
    }
  }
public:
  void sendMessage(const byte data){
    byte hmacResult[32];
    byte ciphertext[16];
    byte nonce[15];

    generateNonce(nonce);

    encryptData(data, nonce, ciphertext);

    calculateHMAC(reinterpret_cast<const char*>(ciphertext), hmacResult);

    // send hmac
    for (int i = 0; i < 32; i++) {
      client.print(hmacResult[i]);
    }

    // send ciphertext
    for (int i = 0; i < 16; i++) {
      client.print(ciphertext[i]);
    }
  }

  byte recvMessage() {
    byte hmacMessage[32];
    byte dataNonce[16];
    byte data;
    byte nonce[15];

    // Seperate HMAC and dataNonce
    int index = 0;
    int incomingByte = -1;
    while ((incomingByte = client.read()) != -1) {
      if (index < 32) {
        hmacMessage[index] = incomingByte;
      } else if (index < 48) {
        dataNonce[index - 32] = incomingByte;
      } else {
        return '\0'; // Message is too long
      }
      index += 1;
    }
    
    // Calculate HMAC
    byte hmacResult[32];
    calculateHMAC(reinterpret_cast<const char*>(dataNonce), hmacResult);

    // Compare HMACs
    if (memcmp(hmacResult, hmacMessage, 32) != 0) {
      return '\0'; // HMAC check failed
    }
    Serial.println("Passed HMAC");  // Testing

    // Decrypt message AES
    decryptData(dataNonce, &data, nonce);
    Serial.println("Decrypted Nonce and Data"); // Testing

    // Check nonce
    if (previousNonce.includes(nonce)) {
      return '\0'; // Nonce check failed
    }
    Serial.println("Passed Nonce");  // Testing
    previousNonce.insert(nonce);

    return data;
  }
};

class WatchDog {
private:
  const int watchdog = 9000; // 9000 milliseconds
  unsigned long lastUpdate;
public:
  void update() {
    lastUpdate = millis(); // heartbeat
  }
  bool getStatus() const {
    if ((millis() - lastUpdate) >= watchdog) {
      return true; // Set an alarm
    }
    return false;
  }
};

class RSSIMonitor {
private:
  float average;
  int total;
  bool alarm;
public:
  RSSIMonitor() : average(0), total(0), alarm(false) {}

  void update() {
    int curr = WiFi.RSSI();

    // Allow a stable average to establish
    if (total > 10 && curr <= (RSSIRange * average)) {
        alarm = true;
        return;
    }

    average = ((average * total) + curr) / (total + 1);
    total += 1;
  }

  bool getStatus() const {
    return alarm;
  }

  void resetStatus() {
    alarm = false;
  }
};

class LatencyMonitor {
private:
  float average;
  int total;
  unsigned long startTime;
public:
  LatencyMonitor() : average(0), total(0) {}

  void start() {
    startTime = millis();
  }

  bool end() {
    unsigned long curr = millis();
    if (total > 0 && (curr - startTime) >= (average * LatencyRange)) {
      return true;
    }
    average = ((average * total) + (curr - startTime)) / (total + 1);
    total += 1;
    return false;
  }
};

void setup() {
  Serial.begin(74480);

  pinMode(D8, OUTPUT);

  Messenger m;
  m.testing();

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
}

Messenger messenger;

void loop() {
  LatencyMonitor latencyMonitor;
  
  if (byte data = messenger.recvMessage() == '\0') {

  }
}
