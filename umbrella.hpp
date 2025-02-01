#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include <string>
#include <sstream>
#include <iomanip>
#include <queue>
#include <mutex>
#include <chrono>

#define RF_FREQUENCY 868000000  // Hz

#define TX_OUTPUT_POWER 14  // dBm

#define LORA_BANDWIDTH 0         // [0: 125 kHz, \
                                 //  1: 250 kHz, \
                                 //  2: 500 kHz, \
                                 //  3: Reserved]
#define LORA_SPREADING_FACTOR 7  // [SF7..SF12]
#define LORA_CODINGRATE 1        // [1: 4/5, \
                                 //  2: 4/6, \
                                 //  3: 4/7, \
                                 //  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8   // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0    // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false

#define RX_TIMEOUT_VALUE 1000
#define BUFFER_SIZE 128  // Define the payload size here

#define U0RXD 44
#define U0TXD 43

#define VIRTUALINPUTRECEIVE 37
#define VIRTUALINPUTLINE 45
#define VIRTUALINPUTCLOCK 46
#define VIRTUALINPUTNOTIFY 42

#define STACK_SIZE 4096
#define DELAYBETWEENTRANSMITS 100

void init_virtual_uart();
void send_to_virtual_uart(const std::string& message);
void readline_from_virtual_uart(std::string* line); //not uart related, lazy to change function name

std::string readLineFromSerial();

bool issatc(const std::string& sentence, const std::string& which);

bool parseGNRMC(const std::string& gnrmc, double* latitude, double* longitude, double* speed, double* direction);
bool parseGNGLL(const std::string& gngll, double* latitude, double* longitude);
bool parseGNVTG(const std::string& gnvtg, double* course, double* speed);
bool parseGNGGA(const std::string& gngga, double* latitude, double* longitude, double* altitude, int* numSatellites);
bool parseGNTXT(const std::string& gntxt, std::string* messageContent);

