# ADK CanSat Heltec Flight Software

### [Main Flight Software](adcansat_flightsoftware_heltec.ino)
 - Developed by - [@szg1](https://www.github.com/szg1) 
 - Handles
   - LoRa Communication
   - GPS reading
   - OLED display


### [virtualserial](virtualserial.cpp)
 - Developed by - [@szg1](https://www.github.com/szg1) 
 - Custom communication protocol, 4 wires
 - Used in every code, where Heltec WiFi Lora v3 communicates with another microcontroller / RPi

### [NMEA Parser](nmea_parser.cpp) - deprecated - pico records gps
 - Developed by - [@szg1](https://www.github.com/szg1) 
 - Parsing NMEA messages
 
