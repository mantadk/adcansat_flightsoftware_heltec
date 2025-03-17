#include "umbrella.hpp"

//read digitally from VIRTUALINPUTLINE
//1 bit / VIRTALINPUTCLOCK cycle
//write high to VIRTUALINPUTRECEIVE while reading
char readCharFromVirtual() {
  sleep(200);
  //Notify slave, that it can send data (1 char)
  digitalWrite(VIRTUAL_ENABLE_LINE, HIGH);  //#arkeugbksuyrehg
  char receivedChar = 0;
  for (int i = 0; i < 8; ++i) {
    // Wait for a clock pulse
    while (digitalRead(VIRTUAL_CLOCK_LINE) == LOW) {  //#srleiuhsdkufgb
      // Wait for the rising edge of the clock signal
    }
    sleep(100);
    // Read the bit from the virtual input line

    // Set the corresponding bit in the received character
    bool bit = digitalRead(VIRTUAL_DATA_LINE);
    if (bit)
      receivedChar |= 1 << i;
    else
      receivedChar &= ~(1 << i);

    // Add a small delay if needed for stability
    // delayMicroseconds(1);
    // Wait for the clock to go back low to complete one cycle
    while (digitalRead(VIRTUAL_CLOCK_LINE) == HIGH) {  //#seruibgiuserh
      // Wait for the falling edge of the clock signal
    }
  }
  // Set the receive pin low after reading
  digitalWrite(VIRTUAL_ENABLE_LINE, LOW);
  return receivedChar;
}

bool istheredata() {
  std::chrono::milliseconds timeout(500);
  std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
  digitalWrite(LED_BUILTIN,HIGH);
  while (std::chrono::steady_clock::now() - start < timeout) {
    if (digitalRead(VIRTUAL_NOTIFY_LINE) != HIGH) {
      digitalWrite(LED_BUILTIN, LOW);
      return true;
    }
  }
  digitalWrite(LED_BUILTIN, LOW);
  return false;
}

int readline_from_virtual_uart(std::string* line) {  //not uart related, lazy to change function name
  // Wait until slave says there is data #srieughvbe
  try {
    bool data_high = istheredata();
    if (!data_high) {
      return 1;
    }
    // std::chrono::milliseconds timeout(500);
    // std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    // while (std::chrono::steady_clock::now() - start < timeout) {
    while (1) {
      char c = readCharFromVirtual();
      if (c == '\n' || c == '\r')  // End of line
      {
        break;
      }
      line->push_back(c);  // Add character to string
      sleep(10);
    }
    line->push_back('\0');
    SENDDATA("data received from slave");
    digitalWrite(LED_BUILTIN, LOW);
    sleep(100);
    return 0;
  } catch (const std::exception& e) {
    SENDDATA(e.what());
    return 1;
  }
}