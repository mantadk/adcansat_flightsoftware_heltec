#include "umbrella.hpp"

//read digitally from VIRTUALINPUTLINE
//1 bit / VIRTALINPUTCLOCK cycle
//write high to VIRTUALINPUTRECEIVE while reading
char readCharFromVirtual() {
  digitalWrite(VIRTUALINPUTRECEIVE, HIGH);  // Set the receive pin high to indicate we're reading
  char receivedChar = 0;                    // Initialize a variable to store the character
  for (int i = 0; i < 8; ++i) {
    // Wait for a clock pulse
    while (digitalRead(VIRTUALINPUTCLOCK) == LOW) {
      // Wait for the rising edge of the clock signal
    }
    // Read the bit from the virtual input line
    if (digitalRead(VIRTUALINPUTLINE) == HIGH) {
      // Set the corresponding bit in the received character
      receivedChar |= (1 << i);
    }
    // Add a small delay if needed for stability
    // delayMicroseconds(1);
    // Wait for the clock to go back low to complete one cycle
    while (digitalRead(VIRTUALINPUTCLOCK) == HIGH) {
      // Wait for the falling edge of the clock signal
    }
  }
  // Set the receive pin low after reading
  digitalWrite(VIRTUALINPUTRECEIVE, LOW);
  return receivedChar;
}

bool istheredata() {
  return (digitalRead(VIRTUALINPUTNOTIFY) == HIGH);
}

void readline_from_virtual_uart(std::string* line) { //not uart related, lazy to change function name
  if (!istheredata()) return;
  std::chrono::milliseconds timeout(500);
  std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
  while (std::chrono::steady_clock::now() - start < timeout)
  {
    char c = readCharFromVirtual();
    if (c == '\n' || c == '\r')  // End of line
    {
      break;
    }

    line->push_back(c);  // Add character to string
  }
  line->push_back('\0');
}