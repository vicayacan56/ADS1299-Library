#include <Arduino.h>
#include <ADS1299_Device.h>
#include <ADS1299_Registers.h>
#include <arduino/ADS1299_ArduinoHAL.h>

/*
  HalRegisterDump
  ---------------
  HAL-only diagnostic example for ADS1299-4, ADS1299-6 or ADS1299.

  Arduino is used here only as the backend implementation of ADS1299_HAL.
  The ADS1299 protocol path is:

    ADS1299_Device -> ADS1299_Protocol -> ADS1299_ArduinoHAL
*/

static constexpr uint8_t PIN_CS    = 10;
static constexpr uint8_t PIN_DRDY  = 7;
static constexpr uint8_t PIN_START = 9;
static constexpr uint8_t PIN_RESET = 8;
static constexpr uint8_t PIN_PWDN  = ADS1299_ArduinoHAL::PIN_UNUSED;

ADS1299_ArduinoHAL adsHal(PIN_CS, PIN_START, PIN_RESET, PIN_PWDN, PIN_DRDY);
ADS1299_Device ads(adsHal);

static void printReg(const char* name, uint8_t addr) {
  uint8_t value = 0;
  Serial.print(name);
  Serial.print(" (0x");
  if (addr < 0x10) Serial.print('0');
  Serial.print(addr, HEX);
  Serial.print(") = ");

  if (!ads.readReg(addr, value)) {
    Serial.println("ERROR");
    return;
  }

  Serial.print("0x");
  if (value < 0x10) Serial.print('0');
  Serial.println(value, HEX);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("ADS1299_Device HalRegisterDump");

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
  Serial.print("Bytes per frame = ");
  Serial.println(ads.bytesPerFrame());

  if (!ads.configureDefaults()) {
    Serial.println("ERROR: configureDefaults() failed");
    while (true) delay(1000);
  }

  Serial.println();
  Serial.println("Register dump after configureDefaults():");
  printReg("ID",        ADS_REG_ID);
  printReg("CONFIG1",   ADS_REG_CONFIG1);
  printReg("CONFIG2",   ADS_REG_CONFIG2);
  printReg("CONFIG3",   ADS_REG_CONFIG3);
  printReg("LOFF",      ADS_REG_LOFF);

  for (uint8_t ch = 1; ch <= ads.channelCount(); ++ch) {
    char name[] = "CH0SET";
    name[2] = char('0' + ch);
    printReg(name, ADS_REG_CH1SET + (ch - 1));
  }

  printReg("BIAS_SENSP", ADS_REG_BIAS_SENSP);
  printReg("BIAS_SENSN", ADS_REG_BIAS_SENSN);
  printReg("LOFF_SENSP", ADS_REG_LOFF_SENSP);
  printReg("LOFF_SENSN", ADS_REG_LOFF_SENSN);
  printReg("LOFF_FLIP",  ADS_REG_LOFF_FLIP);
  printReg("GPIO",       ADS_REG_GPIO);
  printReg("MISC1",      ADS_REG_MISC1);
  printReg("CONFIG4",    ADS_REG_CONFIG4);

  Serial.println();
  Serial.println("Done. RDATAC was not started in this example.");
}

void loop() {
  delay(1000);
}
