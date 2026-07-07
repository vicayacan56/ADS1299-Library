#include <Arduino.h>
#include <ADS1299Plus.h>
#include <ADS1299_SafeSPI.h>

/*
  BasicRead
  ---------
  Minimal Arduino-compatible example for ADS1299-4, ADS1299-6 or ADS1299.

  Adjust the pins to match your board and wiring. SCK/MOSI/MISO are the
  hardware SPI pins of the selected Arduino core. PWDN is normally tied to VDD;
  in that case use ADS1299Plus::ADS_PIN_UNUSED.
*/

static constexpr uint8_t PIN_CS    = 10;
static constexpr uint8_t PIN_DRDY  = 7;   // ADS1299 DRDY, active low
static constexpr uint8_t PIN_START = 9;
static constexpr uint8_t PIN_RESET = 8;
static constexpr uint8_t PIN_PWDN  = ADS1299Plus::ADS_PIN_UNUSED;

ADS1299_SafeSPI adsSpi(PIN_CS);
ADS1299Plus::Pins adsPins = {
  PIN_CS,
  SCK,
  MOSI,
  MISO,
  PIN_DRDY,
  PIN_START,
  PIN_RESET,
  PIN_PWDN
};
ADS1299Plus ads(adsSpi, adsPins);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("ADS1299Plus BasicRead");

  if (!ads.begin()) {
    Serial.println("ERROR: ads.begin() failed");
    while (true) delay(1000);
  }

  uint8_t id = 0;
  if (ads.readDeviceID(id)) {
    Serial.print("ADS1299 ID = 0x");
    Serial.println(id, HEX);
  }

  Serial.print("Detected channels = ");
  Serial.println(ads.channelCount());

  if (!ads.configureDefaults()) {
    Serial.println("ERROR: configureDefaults() failed");
    while (true) delay(1000);
  }

  ads.pinStartHigh();
  delay(10);
  ads.cmdRDATAC();

  Serial.println("RDATAC started");
}

void loop() {
  if (!ads.dataReady()) {
    return;
  }

  uint32_t status = 0;
  int32_t channels[ADS1299Plus::MAX_CHANNELS] = {0};

  if (!ads.readFrameRDATAC(status, channels, ADS1299Plus::MAX_CHANNELS)) {
    Serial.println("ERROR: invalid frame or sync mismatch");
    return;
  }

  Serial.print("STATUS=0x");
  Serial.print(status, HEX);

  const uint8_t n = ads.channelCount();
  for (uint8_t i = 0; i < n; ++i) {
    Serial.print(" CH");
    Serial.print(i + 1);
    Serial.print('=');
    Serial.print(channels[i]);
  }

  Serial.println();
}
