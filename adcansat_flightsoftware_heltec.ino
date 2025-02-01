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
std::string latestMessage = "";

void setup() {
  Serial.begin(9600, SERIAL_8N1, U0RXD, U0TXD);
  delay(1000);
  // init_virtual_uart();
  // delay(1000);

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

void drawFreq() {
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, hrfr());
  display.drawLine(0, 20, 128, 20);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 54, latestMessage.c_str());
}

std::string GPStoStr(const std::string& nmea) {
  if (!nmea.empty()) {
    if (issatc(nmea, "$GNRMC")) {
      double lat, lon, spd, cog;
      if (parseGNRMC(nmea, &lat, &lon, &spd, &cog)) {
        knotsToMps(&spd);
        std::ostringstream returnval;
        returnval << "lat:" << std::fixed << std::setprecision(8) << lat
                  << ";lon:" << std::fixed << std::setprecision(8) << lon
                  << ";spd:" << std::fixed << std::setprecision(2) << spd
                  << ";cog:" << static_cast<int>(std::round(cog));
        return returnval.str();
      }
      return "Invalid GNRMC;" + nmea;
    } else if (issatc(nmea, "$GNGLL")) {
      double lat, lon;
      if (parseGNGLL(nmea, &lat, &lon)) {
        std::ostringstream returnval;
        returnval << "lat:" << std::fixed << std::setprecision(8) << lat
                  << ";lon:" << std::fixed << std::setprecision(8) << lon;
        return returnval.str();
      }
      return "Invalid GNGLL;" + nmea;

    } else if (issatc(nmea, "$GNVTG")) {
      double cog, spd;
      if (parseGNGLL(nmea, &cog, &spd)) {
        knotsToMps(&spd);
        std::ostringstream returnval;
        returnval << "cog:" << std::fixed << std::setprecision(8) << cog
                  << ";spd:" << std::fixed << std::setprecision(8) << spd;
        return returnval.str();
      }
      return "Invalid GNVTG;" + nmea;
    } else if (issatc(nmea, "$GNGGA")) {
      double lat, lon, alt;
      int cnt;
      if (parseGNGGA(nmea, &lat, &lon, &alt, &cnt)) {
        std::ostringstream returnval;
        returnval << "lat:" << std::fixed << std::setprecision(8) << lat
                  << ";lon:" << std::fixed << std::setprecision(8) << lon
                  << ";alt:" << std::fixed << std::setprecision(2) << alt
                  << ";cnt:" << cnt;
        return returnval.str();
      }
      return "Invalid GNGGA;" + nmea;
    } else if (issatc(nmea, "$GNTXT")) {
      std::string msg;
      if (parseGNTXT(nmea, &msg))
      {
        return "msg:"+msg;
      }
      return "Invalid GNTXT;" + nmea;
    } else if (issatc(nmea, "$GNGSA")) {
      return "NP-GNGSA";
    } else if (issatc(nmea, "$GPGSV") + issatc(nmea, "$GNGSV") + issatc(nmea, "$GLGSV")) {
      return "NP-GXGSV";
    } else {
      return "Not NMEA;" + nmea;
    }
  }
  return "No Data";
}
std::string latestreading = "",latestreading2 = "";
void loop() {
  display.clear();
  std::string dataJustRead = readLineFromSerial();
  display.setFont(ArialMT_Plain_10);
  if (dataJustRead != "") {
    latestreading = dataJustRead;
    display.drawString(0, 24, latestreading.c_str());
    SENDDATA(GPStoStr(dataJustRead));
  }
  std::string dataJustRead2 = "";
  readline_from_virtual_uart(&dataJustRead2); //not uart related, lazy to change function name
  if (dataJustRead2 != "") {
    latestreading2 = dataJustRead2;
    display.drawString(0, 39, latestreading.c_str());
    SENDDATA(dataJustRead2);
  }
  drawFreq();
  display.display();
  Radio.IrqProcess();
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
      latestMessage = message;
      messageQueue.pop();
      queueMutex.unlock();

      static float txNumber = 0;
      txNumber += 0.01;
      char txpacket[256];
      snprintf(txpacket, sizeof(txpacket), "%.2f - %s", txNumber, message.c_str());
      lora_idle = false;
      Radio.Send((uint8_t*)txpacket, strlen(txpacket));
    }
    vTaskDelay(pdMS_TO_TICKS(DELAYBETWEENTRANSMITS));  // Delay
  }
}

void startQueueingTask() {
  xTaskCreatePinnedToCore(
    LoRaSenderTask,  // Task function
    "LoRaSender",    // Name of the task
    STACK_SIZE,      // Stack size
    NULL,            // Task parameter
    1,               // Priority
    NULL,            // Task handle
    1                // Core to run on
  );
}