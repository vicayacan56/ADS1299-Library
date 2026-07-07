// ADS1299_SafeSPI.cpp

#include "ADS1299_SafeSPI.h"

ADS1299_SafeSPI::ADS1299_SafeSPI(uint8_t csPin, SPIClass& spi, uint32_t spiHz)
    : csPin_(csPin),
      spi_(&spi),
      hal_(nullptr),
      spiHz_(spiHz),
      active_(false),
      useHal_(false) {}

ADS1299_SafeSPI::ADS1299_SafeSPI(ADS1299_HAL& hal, uint32_t spiHz)
    : csPin_(0),
      spi_(nullptr),
      hal_(&hal),
      spiHz_(spiHz),
      active_(false),
      useHal_(true) {}

void ADS1299_SafeSPI::begin()
{
  if (useHal_) {
    if (!active_) {
      hal_->begin();

      ADS1299_SpiConfig config;
      config.clockHz = spiHz_;
      config.bitOrder = ADS1299_SpiBitOrder::MSB_FIRST;
      config.mode = ADS1299_SpiMode::MODE1;

      hal_->beginTransaction(config);
      active_ = true;
    }

    hal_->csHigh();
    return;
  }

  pinMode(csPin_, OUTPUT);
  digitalWrite(csPin_, HIGH);

  if (active_)
    return;

  spi_->begin();
  spi_->beginTransaction(SPISettings(spiHz_, MSBFIRST, SPI_MODE1));
  active_ = true;
}

void ADS1299_SafeSPI::end()
{
  if (!active_)
    return;

  if (useHal_) {
    hal_->endTransaction();
    hal_->end();
  } else {
    spi_->endTransaction();
    spi_->end();
  }

  active_ = false;
}

void ADS1299_SafeSPI::select()
{
  if (useHal_) {
    hal_->csLow();
  } else {
    digitalWrite(csPin_, LOW);
  }
}

void ADS1299_SafeSPI::deselect()
{
  if (useHal_) {
    hal_->csHigh();
  } else {
    digitalWrite(csPin_, HIGH);
  }
}

uint8_t ADS1299_SafeSPI::xfer(uint8_t data)
{
  if (useHal_) {
    return hal_->spiTransfer(data);
  }

  return spi_->transfer(data);
}

void ADS1299_SafeSPI::waitDecode()
{
  // tSDECODE >= 4 tCLK. With the nominal ADS1299 clock of 2.048 MHz,
  // 4 tCLK is about 1.96 us. Use 3 us as a conservative margin.
  if (useHal_) {
    hal_->delayMicroseconds(3);
  } else {
    delayMicroseconds(3);
  }
}
