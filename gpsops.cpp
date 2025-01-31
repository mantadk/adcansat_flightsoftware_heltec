
std::string readLineFromSerial() {
  static std::string buffer;
  const unsigned long timeout = 100;  // Timeout in milliseconds
  static unsigned long lastCharTime = millis();
  while (Serial.available()) {
    char c = Serial.read();
    buffer += c;
    lastCharTime = millis();
    if (c == '\n') {
      std::string line = buffer;
      buffer.clear();
      return line;
    }
  }
  if (buffer.length() > 0 && (millis() - lastCharTime > timeout)) {
    // Return incomplete line after timeout (optional)
    std::string line = buffer;
    buffer.clear();
    latestSerial = line;
    return line;
  }
  return "";  // No complete line yet
} 
