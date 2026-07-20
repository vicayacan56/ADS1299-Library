// ADS1299_Protocol.h
// Internal portable protocol object for future HAL-backed ADS1299 execution.

#pragma once

#include "../hal/ADS1299_HAL.h"

class ADS1299_Protocol {
public:
  explicit ADS1299_Protocol(ADS1299_HAL& hal);

  void cmdWakeup();
  void cmdStandby();
  void cmdReset();
  void cmdStart();
  void cmdStop();
  void cmdRDATAC();
  void cmdSDATAC();
  void cmdRDATA();

  bool writeReg(uint8_t addr, uint8_t value);
  bool readReg(uint8_t addr, uint8_t& value);
  bool writeRegs(uint8_t startAddr, const uint8_t* data, size_t n);
  bool readRegs(uint8_t startAddr, uint8_t* data, size_t n);
  bool readFrameRDATAC(uint8_t channelCount, uint32_t& status24, int32_t* channels, size_t capacity);
  bool readDataOnDemand(uint8_t channelCount, uint32_t& status24, int32_t* channels, size_t capacity);

  bool isRDATACActive() const;

private:
  void sendCommand_(uint8_t command);
  void waitDecode_();
  bool readFrameBytes_(uint8_t channelCount, uint32_t& status24, int32_t* channels, size_t capacity);

  ADS1299_HAL* hal_;
  bool rdatacActive_ = false;
};
