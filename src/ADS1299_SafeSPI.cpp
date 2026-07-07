// ADS1299_SafeSPI.cpp

#include "ADS1299_SafeSPI.h"

ADS1299_SafeSPI::ADS1299_SafeSPI(uint8_t csPin, SPIClass& spi, uint32_t spiHz)
    : csPin_(csPin), spi_(spi), spiHz_(spiHz) {}

void ADS1299_SafeSPI::begin()
{
  pinMode(csPin_, OUTPUT);
  digitalWrite(csPin_, HIGH);

  if (active_)
    return;

  spi_.begin();
  spi_.beginTransaction(SPISettings(spiHz_, MSBFIRST, SPI_MODE1));
  active_ = true;
}

void ADS1299_SafeSPI::end()
{
  if (!active_)
    return;

  spi_.endTransaction();
  spi_.end();
  active_ = false;
}

void ADS1299_SafeSPI::select()
{
  digitalWrite(csPin_, LOW);
}

void ADS1299_SafeSPI::deselect()
{
  digitalWrite(csPin_, HIGH);
}

uint8_t ADS1299_SafeSPI::xfer(uint8_t data)
{
  return spi_.transfer(data);
}

void ADS1299_SafeSPI::waitDecode()
{
  // tSDECODE >= 4 tCLK. With the nominal ADS1299 clock of 2.048 MHz,
  // 4 tCLK is about 1.96 us. Use 3 us as a conservative margin.
  delayMicroseconds(3);
}
