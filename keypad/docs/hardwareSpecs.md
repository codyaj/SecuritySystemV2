# Hardware Documentation


## Bill of Materials (BOM)
| Item | Description | Quantity | Part Number | Supplier |
| ESP8266 | Wi-Fi enabled microcontroller | 1 | ELEC-NODEMCU-ESP8266 | NodeMCU |
| Membrane Keypad | 4x4 matrix keypad | 1 | N/A | Amazon |
| Mini Piezo Buzzer 3-16VDC | DC piezoelectric buzzer | 1 | AB3462 | Jaycar |
| Project Case | Custom case for encolsing the components | 1 | N/A | Custom |


## System Overview
The system is designed to send and receive signals from the server about the status of the alarm system. Then through user input allows the system to be armed or disarmed. 


## Wiring Diagram
[ESP8266]      [Keypad]
   D0  <-------> Col 1
   D1  <-------> Col 2
   D2  <-------> Col 3
   D3  <-------> Col 4
   D4  <-------> Row 1
   D5  <-------> Row 2
   D6  <-------> Row 3
   D7  <-------> Row 4

   D8  <------> Buzzer +
  GND  <------> Buzzer -


## Hardware Setup


## Troubleshooting