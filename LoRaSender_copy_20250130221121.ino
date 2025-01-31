#include "umbrella.hpp"

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

double txNumber;

bool lora_idle = true;

static RadioEvents_t RadioEvents;
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);  // addr , freq , i2c group , resolution , rst

void OnTxDone(void);
void OnTxTimeout(void);

std::string latestGPGSV = "";
std::string latestGLGSV = "";
std::string latestGNRMC = "";
std::string latestSatsInView = "";
bool latestGPGSVchanged = false;
bool latestGLGSVchanged = false;
bool latestGNRMCchanged = false;
double latestLat, latestLon, latestSpeed, latestDir;

void setup() {
  Serial.begin(9600, SERIAL_8N1, U0RXD, U0TXD);
  delay(1000);

  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  txNumber = 0;

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;

  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000);
  VextON();
  delay(1000);
  display.init();
  delay(1000);
  startQueueingTask();
  delay(1000);
}
void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

String hrfr() {
  if (RF_FREQUENCY >= 1000000000) {
    float frf = RF_FREQUENCY / 1000000000.0;
    String end = "GHz";
    return String(frf, 3) + end;
  } else if (RF_FREQUENCY >= 1000000) {
    float frf = RF_FREQUENCY / 1000000.0;
    String end = "MHz";
    return String(frf, 3) + end;
  } else if (RF_FREQUENCY >= 1000) {
    float frf = RF_FREQUENCY / 1000.0;
    String end = "kHz";
    return String(frf, 3) + end;
  } else {
    return String(RF_FREQUENCY) + "Hz";
  }
}


std::string toTransmit = "";
void drawFreq() {
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, hrfr());
  display.drawLine(0, 20, 128, 20);
  display.setFont(ArialMT_Plain_10);
  if (latestSatsInView != "")
    toTransmit = latestSatsInView;
  else if (latestGNRMC != "")
    toTransmit = latestGNRMC;
  else if (latestGLGSV != "")
    toTransmit = latestGLGSV;
  else if (latestGPGSV != "")
    toTransmit = latestGPGSV;
  else
    toTransmit = "Check GPS connection I guess";
  display.drawString(0, 44, toTransmit.c_str());
}

std::string GPStoStr(std::string nmea) {
  if (!nmea.empty()) {
    if (isgnrmc(nmea) && parseGNRMC(nmea, &latestLat, &latestLon, &latestSpeed, &latestDir)) {
      latestGNRMC = nmea;
      latestGNRMCchanged = true;
      knotsToMps(&latestSpeed);
    }
    if (issatc(nmea, "$GPGSV")) {
      latestGPGSV = nmea;
      latestGPGSVchanged = true;
    }
    if (issatc(nmea, "$GLGSV")) {
      latestGLGSV = nmea;
      latestGLGSVchanged = true;
    }
  }
  if (latestGNRMCchanged * latestGLGSVchanged * latestGPGSVchanged) {
    latestGNRMCchanged = false;
    latestGLGSVchanged = false;
    latestGPGSVchanged = false;
    std::ostringstream returnval;
    returnval << satsinview(latestGPGSV, latestGLGSV)
              << ";lat:" << std::fixed << std::setprecision(8) << latestLat
              << ";lon:" << std::fixed << std::setprecision(8) << latestLon
              << ";spd:" << std::fixed << std::setprecision(2) << latestSpeed
              << ";dir:" << static_cast<int>(std::round(latestDir));
    return returnval.str();
  }
  return "No GPS";
}

void loop() {
  std::string dataJustRead = readLineFromSerial();
  if (dataJustRead != "")
    SENDDATA(GPStoStr(dataJustRead));
  display.clear();
  drawFreq();
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 24, txpacket);
  display.display();
  Radio.IrqProcess();
}

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
std::string satsinview(std::string gpsSentence, std::string glonassSentence) {
  auto parseSatCount = [](const std::string& sentence) -> int {
    if (sentence.empty()) return 0;

    std::istringstream iss(sentence);
    std::string field;
    int fieldIndex = 0;

    try {
      while (std::getline(iss, field, ',')) {
        fieldIndex++;
        if (fieldIndex == 4) {  // Satellites in view field
          return std::stoi(field);
        }
      }
    } catch (const std::exception& e) {
      //Serial.printf("GSV parse error: %s\n", e.what());
    }
    return 0;
  };

  int gpsCount = parseSatCount(gpsSentence);
  int glonassCount = parseSatCount(glonassSentence);
  int total = gpsCount + glonassCount;
  std::stringstream lsivss;
  lsivss << total;
  latestSatsInView = lsivss.str();


  // Format with ASCII art and additional details
  std::ostringstream result;
  result << "gps:" << gpsCount
         << ";gls:" << glonassCount
         << ";sum:" << total;
  SENDDATA(result.str());
  return result.str();
}

bool parseGNRMC(const std::string& gnrmc, double* latitude, double* longitude, double* speed, double* direction) {
  // Split the sentence into fields
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

void knotsToMps(double* spdtc) {
  *spdtc = *spdtc * 0.514444;
}


void OnTxDone(void) {
  lora_idle = true;
}

void OnTxTimeout(void) {
  Radio.Sleep();
  display.clear();
  display.println("TX Timeout......");
  display.display();
  delay(500);
  lora_idle = true;
}

std::queue<std::string> messageQueue;
std::mutex queueMutex;

bool SENDDATA(const std::string& datatosend) {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (messageQueue.size() * sizeof(std::string) >= STACK_SIZE) {
        return false;
    }
    messageQueue.push(datatosend);
    return true;
}

void LoRaSenderTask(void* parameter) {
  while (true) {
    if (!messageQueue.empty() && lora_idle) {
      queueMutex.lock();
      std::string message = messageQueue.front();
      messageQueue.pop();
      queueMutex.unlock();

      static float txNumber = 0;
      txNumber += 0.01;
      char txpacket[256];
      snprintf(txpacket, sizeof(txpacket), "%.2f - %s", txNumber, message.c_str());

      lora_idle = false;
      Radio.Send((uint8_t*)txpacket, strlen(txpacket));
    }
    vTaskDelay(pdMS_TO_TICKS(1000));  // Delay 1 second
  }
}

void startQueueingTask() {
  xTaskCreatePinnedToCore(
    LoRaSenderTask,  // Task function
    "LoRaSender",    // Name of the task
    STACK_SIZE,            // Stack size
    NULL,            // Task parameter
    1,               // Priority
    NULL,            // Task handle
    1                // Core to run on
  );
}

// void SENDDATA(std::string datatosend) {
//   if (lora_idle == true) {
//     txNumber += 0.01;
//     sprintf(txpacket, "%d - %s", txNumber, datatosend.c_str());       //start a package
//     Radio.Send((uint8_t*)txpacket, strlen(txpacket));  //send the package out
//     lora_idle = false;
//   }
// }