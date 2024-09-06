#include <ESP8266WiFi.h>
#include <SHA256.h>
#include <AES.h>

#define NonceStorageSize 10

#define ssid "TelstraK62F18"
#define pass "r<!bqD9aFvFnrUHD{Ddowg2vqeY)Lzcr"
const byte serverIP[4] = {192,168,20,4};
#define serverPort 12345

const char passcode[4] = {'1', '2', '3', '4'};
#define RSSIRange 1.15
#define LatencyRange 1.85
// Heartbeat 3 seconds
// Watchdog timer at 9 seconds
#define probationaryAlarmTime 60


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
  bool status;
public:
  RSSIMonitor() : average(0), total(0), status(false) {}

  void update() {
    int curr = WiFi.RSSI();

    // Allow a stable average to establish
    if (total > 10 && curr <= (RSSIRange * average)) {
        status = true;
        return;
    }

    average = ((average * total) + curr) / (total + 1);
    total += 1;
  }

  bool getStatus() {
    status = false;
    return status;
  }
};

class LatencyMonitor {
private:
  float average;
  int total;
  unsigned long startTime;
  bool status;
public:
  LatencyMonitor() : average(0), total(0), status(false) {}

  void start() {
    startTime = millis();
  }

  void end() {
    unsigned long curr = millis();
    if (total > 0 && (curr - startTime) >= (average * LatencyRange)) {
      status = true;
      return;
    }
    average = ((average * total) + (curr - startTime)) / (total + 1);
    total += 1;
  }

  bool getStatus() {
    status = false;
    return status;
  }
};

class OutgoingMessages { // circular buffer
private:
  byte messages[3];
  int head;
  int tail;
  int count;
public:
  OutgoingMessages() : head(0), tail(0), count(0) {}

  void add(byte message) {
    messages[head] = message;
    head = (head + 1) % 3;

    if (count < 3) {
      count += 1;
    } else {
      tail = (tail + 1) % 3;
    }
  }

  byte get() {
    if (count == 0) { // buffer empty
      return 0;
    }

    byte message = messages[tail];
    tail = (tail + 1) % 3;
    count -= 1;
    return message;
  }

  bool isEmpty() {
    return count == 0;
  }
}

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
}

unsigned long nextAlarm = 0;
unsigned long probationary = 0;
bool probationaryAlarm = false;
bool alarmOn = false;
bool alarmActive = false;
void alarmController() {
  unsigned long curr = millis();
  if (alarmActive && curr >= nextAlarm) {
    digitalWrite(D8, alarmOn);
    nextAlarm = curr + 100;
    alarmOn = !alarmOn;
  } else if (probationaryAlarm) {
    if (curr >= probationary + probationaryAlarmTime) {
      probationaryAlarm = false;
      alarmActive = true;
    } else if (curr >= nextAlarm) {
      digitalWrite(D8, alarmOn);
      if (alarmOn) {
        nextAlarm = curr + 450;
      } else {
        nextAlarm = curr + 50;
      }
      alarmOn = !alarmOn;
    }
  } else {
    digitalWrite(D8, LOW);
  }
}

LatencyMonitor latencyMonitor;
RSSIMonitor rssiMonitor;
WatchDog watchDog;
void jammingController() {
  if (watchDog.getStatus() || (rssiMonitor.getStatus() & latencyMonitor.getStatus())) {
    alarmActive = true;
    outgoingMessages.add(1<<2);
  }
}

bool onLockdown = false;
OutgoingMessages outgoingMessages;
Messenger messenger;
void serverController() {
  if (!client.connected()) {
    if (!client.connect(serverIP, serverPort)) {
      return;
    }
  }

  // Recv all messages
  byte in;
  while ((in = messenger.recvMessage()) != 0) {
    if (in & (1<<0) && onLockdown) { // Input from a sensor
      probationary = millis()
      probationaryAlarm = true;
      outgoingMessages.add(((1<<0) | (1<<3))); // Confirmation Message
    } else if (in & (1<<1)) { // Heartbeat
      if (!(in & (1<<3))) { // if not confirmation
        latencyMonitor.start();
        outgoingMessages.add(((1<<1) | (1<<3))); // Confirmation Message
      } else {
        latency.end();
      }
    } else if (in & (1<<2)) { // Jamming
      alarmActive = true;
      outgoingMessages.add(((1<<2) | (1<<3))); // Confirmation Message
    }
  }

  // Send all messages
  while (!outgoingMessages.isEmpty()) {
    messenger.sendMessage(outgoingMessages.get());
  }
}

int currCode[4] = {0};
int codeIndex = 0;
Keypad keypad;
void keypadController() {
  char key;
  if ((key = keypad.getChar()) != '\0') {
    if (alarmActive || probationaryAlarm) { // Waiting for code
      if (key == '#') {
        bool valid = true;
        for (int i = 0; i < 4; i++) {
          if (currCode[i] != passcode[i]) {
            valid = false;
            break;
          }
        }

        if (valid) {
          alarmActive = false;
          probationaryAlarm = false;
          codeIndex = 0;
        }

      } else if (key == '*') {
        for (int i = 0; i < 4; i++) {
          currCode[i] = 0;
        }
        codeIndex = 0;

      } else {
        if (codeIndex <= 3) {
          currCode[codeIndex] = key;
          codeIndex += 1;
        }
      }
    } else if (key == 'A') {
        outgoingMessages.add((1<<0));
    }
  }
}

void loop() {
  alarmController();
  jammingController();
  serverController();
  keypadController();
}
