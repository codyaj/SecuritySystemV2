# Design Documentation


## Introduction

### Purpose
The purpose of this microcontroller is to grant that user the ability to arm and disarm the system from a central physical location.

### Scope
This document covers the software aspects of the microcontroller, including:
* **Keypad Integration:** Interaction and handling of keypad inputs.
* **Communication Protocols:** Methods used for data exchange between the microcontroller and server.
* **Data Flow:** The process of sending and receiving data.
* **Security:** Measures for ensuring secure communication.
* **Testing:** Strategies for validating the functionality and reliability of the system.


## Keypad Integration

### Keypress Handling
  - **Initialization:** The Keypad class handles the initialization of the keypad by assigning the pin mode for each pin.
  - **Key Press Detection:** Key presses are detected when needed by calling the getChar function of a Keypad object. To manage debounce there is a 200ms between each reading.

## Communication Protocols

### Message Formatting

The message format used in communication between the microcontroller and the server consists of the follwing components:

  * **32 Bytes for HMAC:** This secures the message by ensuring its integrity and authenticity, preventing tampering and verifying the sender's identity.
  * **1 Byte for Information:** This byte is enough to carry all necessary commands or status signals, efficiently conveying the required data without excess overhead.
  * **15 Bytes for Nonce:** The nonce provides a unique identifier for each message, protecting against replay attacks by ensuring no two messages are the same. The AES128 encryption required 16 bytes of data to be used. Instead of wasting this on padding using 15 bytes for the Nonce improves its security.

### ESP8266 to Server Communication

  1) **Arm/Disarm Requests:**
    - **ESP8266 --> Server:** Send an "Arm" or "Disarm" request.
    - **Server --> ESP8266:** Server responds with an "Arm/Disarm confirm"
  1) **Heartbeat Monitoring:**
    - **ESP8266 --> Server:** Send periodic heartbeat messages.
    - **Server --> ESP8266:** Server responds with a heartbeat response.
  1) **Arm/Disarm Requests:**
    - **ESP8266 <--> Server:** Notify if jamming is detected.
    - **ESP8266 <--> Server:** Responds with a message confirmation

## Security

### Authentication
  - **Nonces:** Unique values to prevent replay attacks and ensure message freshness.
  - **HMAC:** Ensures message integrity and authenticity using a shared secret key. 

### Data Encryption
  - **Encrypted Communication:** AES128 encryption protects all data exchanged between the microcontroller and server. To protect against recognizing patterns in the data sent the nonce is included with the data to always give a unique encrypted output.

### Jammer Detection
  - **RSSI Monitoring:** The microcontroller tracks average RSSI and detects sudden drops or unusual fluctuations that may indicate jamming.
  - **Latency Monitoring:** The microcontroller measures the round-trip time of test packets. Significant latency increases could signal interference or jamming.
  - **Watchdog Timer and Heartbeat Mechanism:** A watchdog timer paired with periodic heartbeat signals detects network outages. If the heartbeat is missed, the system triggers an alert, indicating possible jamming.
The alarm will only trigger if any two of the three detection methods activate, ensuring more reliable detection of potential jamming.

## Testing

### Testing Strategy

### Validation Criteria