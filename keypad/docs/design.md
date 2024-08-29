# Design Documentation


## Introduction

### Purpose
The purpose of this microcontroller is to grant that user the ability to arm and disarm the system from a central physical location.

### Scope
This document covers the software acpects of the microcontroller, including:
* **Keypad Integration:** Interaction and handling of keypad inputs.
* **Communication Protocols:** Methods used for data exchange between the microcontroller and server.
* **Data Flow:** The process of sending and receiving data.
* **Security:** Measures for ensuring secure communication.
* **Testing:** Strategies for validating the functionality and reliability of the system.


## Keypad Integration

### Keypress Handling
  - **Initialization:** The Keypad class handles the initialization of the keypad by assigning the pin mode for each pin.
  - **Key Press Detection:** Key presses are detected when needed by calling the getChar function of a Keypad object. To manage debounce there should be atleast 200ms between each reading.

## Communication Protocols


## Security
rolling codes
### Data Encryption

### Authentication


## Testing

### Testing Strategy

### Validation Criteria