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

  bool isRDATACActive() const;

private:
  void sendCommand_(uint8_t command);
  void waitDecode_();

  ADS1299_HAL* hal_;
  bool rdatacActive_ = false;
};
