#include <Arduino.h>
#include <ADS1299_Device.h>
#include <arduino/ADS1299_ArduinoHAL.h>

/*
  HalBasicRead
  ------------
  HAL-only RDATAC acquisition example for ADS1299-4, ADS1299-6 or ADS1299.

  Arduino is used here only as the backend implementation of ADS1299_HAL.
  The ADS1299 protocol path is:

    ADS1299_Device -> ADS1299_Protocol -> ADS1299_ArduinoHAL
*/

static constexpr uint8_t PIN_CS    = 10;
static constexpr uint8_t PIN_DRDY  = 7;   // ADS1299 DRDY, active low
static constexpr uint8_t PIN_START = 9;
static constexpr uint8_t PIN_RESET = 8;
static constexpr uint8_t PIN_PWDN  = ADS1299_ArduinoHAL::PIN_UNUSED;

ADS1299_ArduinoHAL adsHal(PIN_CS, PIN_START, PIN_RESET, PIN_PWDN, PIN_DRDY);
ADS1299_Device ads(adsHal);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("ADS1299_Device HalBasicRead");

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

  ads.startConversions();
  delay(10);
  ads.cmdRDATAC();

  Serial.println("RDATAC started through HAL-only path");
}

void loop() {
  if (!ads.dataReady()) {
    return;
  }

  uint32_t status = 0;
  int32_t channels[ADS1299_Device::MAX_CHANNELS] = {0};

  if (!ads.readFrameRDATAC(status, channels, ADS1299_Device::MAX_CHANNELS)) {
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
