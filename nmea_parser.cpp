#include "umbrella.hpp" 

bool parseGNRMC(const std::string& gnrmc, double* latitude, double* longitude, double* speed, double* direction) {
  std::istringstream stream(gnrmc);
  std::string field;
  int fieldIndex = 0;
  std::string latField, latDirection, lonField, lonDirection, speedField, directionField;
  while (std::getline(stream, field, ',')) {
    fieldIndex++;
    switch (fieldIndex) {
      case 4:  // Latitude field
        latField = field;
        break;
      case 5:  // Latitude direction (N/S)
        latDirection = field;
        break;
      case 6:  // Longitude field
        lonField = field;
        break;
      case 7:  // Longitude direction (E/W)
        lonDirection = field;
        break;
      case 8:  // Speed field (knots)
        speedField = field;
        break;
      case 9:  // Direction field (degrees)
        directionField = field;
        break;
    }
  }

  // Validate extracted fields
  if (latField.empty() || lonField.empty() || latDirection.empty() || lonDirection.empty()) {
    return false;  // Missing fields
  }

  try {
    // Convert latitude to decimal degrees
    double latDegrees = std::stod(latField.substr(0, 2));
    double latMinutes = std::stod(latField.substr(2));
    *latitude = latDegrees + (latMinutes / 60.0);
    if (latDirection == "S") {
      *latitude = -*latitude;
    }

    // Convert longitude to decimal degrees
    double lonDegrees = std::stod(lonField.substr(0, 3));
    double lonMinutes = std::stod(lonField.substr(3));
    *longitude = lonDegrees + (lonMinutes / 60.0);
    if (lonDirection == "W") {
      *longitude = -*longitude;
    }

    // Parse speed (knots)
    *speed = speedField.empty() ? 0.0 : std::stod(speedField);

    // Parse direction (degrees)
    *direction = directionField.empty() ? 0.0 : std::stod(directionField);

  } catch (const std::exception& e) {
    return false;  // Conversion error
  }

  return true;  // Successfully parsed
}

bool parseGNGLL(const std::string& gngll, double* latitude, double* longitude) {
  std::istringstream stream(gngll);
  std::string field;
  int fieldIndex = 0;
  std::string latField, latDirection, lonField, lonDirection;

  while (std::getline(stream, field, ',')) {
    fieldIndex++;
    switch (fieldIndex) {
      case 2:  // Latitude field
        latField = field;
        break;
      case 3:  // Latitude direction (N/S)
        latDirection = field;
        break;
      case 4:  // Longitude field
        lonField = field;
        break;
      case 5:  // Longitude direction (E/W)
        lonDirection = field;
        break;
    }
  }

  // Validate extracted fields
  if (latField.empty() || lonField.empty() || latDirection.empty() || lonDirection.empty()) {
    return false;  // Missing fields
  }

  try {
    // Convert latitude to decimal degrees
    double latDegrees = std::stod(latField.substr(0, 2));
    double latMinutes = std::stod(latField.substr(2));
    *latitude = latDegrees + (latMinutes / 60.0);
    if (latDirection == "S") {
      *latitude = -*latitude;
    }

    // Convert longitude to decimal degrees
    double lonDegrees = std::stod(lonField.substr(0, 3));
    double lonMinutes = std::stod(lonField.substr(3));
    *longitude = lonDegrees + (lonMinutes / 60.0);
    if (lonDirection == "W") {
      *longitude = -*longitude;
    }
  } catch (const std::exception& e) {
    return false;  // Conversion error
  }

  return true;  // Successfully parsed
}