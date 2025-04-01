#include "umbrella.hpp"

bool issatc(const std::string& sentence, const std::string& which) {
  // Check if sentence is long enough for a valid GNRMC sentence
  if (sentence.length() < 7) {
    return false;
  }

  // Check if the sentence starts with eg.: "$GPGSV"
  if (sentence.substr(0, 6) == which) {
    // Check if the sentence ends with the checksum (starting with '*')
    // if (sentence[sentence.length() - 3] == '*') {
    //   return true;
    // }
    return true;
  }

  // If it does not start with "$GNRMC" or ends with '*', return false
  return false;
}