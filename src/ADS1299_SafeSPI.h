// ADS1299_SafeSPI.h
// Arduino-compatible SPI transport for ADS1299.
//
// This class is the Arduino backend of the driver: it owns CS timing,
// SPI transactions and byte transfers using an Arduino SPIClass instance.

#pragma once
#include <Arduino.h>
#include <SPI.h>
#include "hal/ADS1299_HAL.h"

class ADS1299_SafeSPI
{
public:
  static constexpr uint32_t DEFAULT_SPI_HZ = 2000000UL;

  // By default the driver uses the global Arduino SPI object. Boards with
  // more than one SPI peripheral can pass another SPIClass instance.
  explicit ADS1299_SafeSPI(uint8_t csPin,
                           SPIClass& spi = SPI,
                           uint32_t spiHz = DEFAULT_SPI_HZ);

  // Optional HAL-backed constructor. This is additive: the Arduino constructor
  // above remains the default path used by existing sketches.
  explicit ADS1299_SafeSPI(ADS1299_HAL& hal,
                           uint32_t spiHz = DEFAULT_SPI_HZ);

  // begin()/end() are idempotent to tolerate sketches that initialize the
  // transport manually before calling ADS1299Plus::begin().
  void begin();
  void end();

  void select();
  void deselect();

  uint8_t xfer(uint8_t data);
  void waitDecode(); // tSDECODE >= 4 tCLK (~2 us with fCLK=2.048 MHz)

private:
  uint8_t csPin_;
  SPIClass* spi_;
  ADS1299_HAL* hal_;
  uint32_t spiHz_;
  bool active_ = false;
  bool useHal_ = false;
};
