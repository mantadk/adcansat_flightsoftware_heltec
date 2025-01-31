#include "umbrella.hpp"

bool isgnrmc(std::string sentence) {
  // Check if sentence is long enough for a valid GNRMC sentence
  if (sentence.length() < 7) {
    return false;
  }

  // Check if the sentence starts with "$GNRMC"
  if (sentence.substr(0, 6) == "$GNRMC") {
    // Optional: Check if the sentence ends with the checksum (starting with '*')
    if (sentence[sentence.length() - 3] == '*') {
      return true;
    }
  }

  // If it does not start with "$GNRMC" or ends with '*', return false
  return false;
}
bool issatc(std::string sentence, std::string which) {
  // Check if sentence is long enough for a valid GNRMC sentence
  if (sentence.length() < 7) {
    return false;
  }

  // Check if the sentence starts with "$GPGSV"
  if (sentence.substr(0, 6) == which) {
    // Optional: Check if the sentence ends with the checksum (starting with '*')
    if (sentence[sentence.length() - 3] == '*') {
      return true;
    }
  }

  // If it does not start with "$GNRMC" or ends with '*', return false
  return false;
}