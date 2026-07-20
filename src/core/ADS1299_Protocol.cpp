// ADS1299_Protocol.cpp

#include "ADS1299_Protocol.h"
#include "../ADS1299_Registers.h"
#include "ADS1299_Core.h"

ADS1299_Protocol::ADS1299_Protocol(ADS1299_HAL& hal)
    : hal_(&hal)
{
}

void ADS1299_Protocol::sendCommand_(uint8_t command)
{
  hal_->csLow();
  hal_->spiTransfer(command);
  hal_->csHigh();
}

void ADS1299_Protocol::waitDecode_()
{
  hal_->delayMicroseconds(3);
}

void ADS1299_Protocol::cmdWakeup()
{
  sendCommand_(ADS_CMD_WAKEUP);
  waitDecode_();
}

void ADS1299_Protocol::cmdStandby()
{
  sendCommand_(ADS_CMD_STANDBY);
  waitDecode_();
}

void ADS1299_Protocol::cmdReset()
{
  sendCommand_(ADS_CMD_RESET);
  rdatacActive_ = false;
  hal_->delayMicroseconds(20);
}

void ADS1299_Protocol::cmdStart()
{
  sendCommand_(ADS_CMD_START);
  waitDecode_();
}

void ADS1299_Protocol::cmdStop()
{
  sendCommand_(ADS_CMD_STOP);
  waitDecode_();
}

void ADS1299_Protocol::cmdRDATAC()
{
  sendCommand_(ADS_CMD_RDATAC);
  rdatacActive_ = true;
  waitDecode_();
}

void ADS1299_Protocol::cmdSDATAC()
{
  sendCommand_(ADS_CMD_SDATAC);
  rdatacActive_ = false;
  waitDecode_();
}

void ADS1299_Protocol::cmdRDATA()
{
  sendCommand_(ADS_CMD_RDATA);
}

bool ADS1299_Protocol::writeReg(uint8_t addr, uint8_t value)
{
  if (rdatacActive_ || addr > ADS_REG_CONFIG4)
    return false;

  hal_->csLow();
  hal_->spiTransfer(ADS1299Core::writeRegisterCommand(addr));
  hal_->spiTransfer(0x00);
  hal_->spiTransfer(value);
  hal_->csHigh();
  waitDecode_();
  return true;
}

bool ADS1299_Protocol::readReg(uint8_t addr, uint8_t& value)
{
  if (rdatacActive_ || addr > ADS_REG_CONFIG4)
    return false;

  hal_->csLow();
  hal_->spiTransfer(ADS1299Core::readRegisterCommand(addr));
  hal_->spiTransfer(0x00);
  value = hal_->spiTransfer(ADS_CMD_NOP);
  hal_->csHigh();
  waitDecode_();
  return true;
}

bool ADS1299_Protocol::writeRegs(uint8_t startAddr, const uint8_t* data, size_t n)
{
  if (rdatacActive_ || data == nullptr || !ADS1299Core::validRegisterRange(startAddr, n))
    return false;

  hal_->csLow();
  hal_->spiTransfer(ADS1299Core::writeRegisterCommand(startAddr));
  hal_->spiTransfer((uint8_t)(n - 1));
  for (size_t i = 0; i < n; ++i) {
    hal_->spiTransfer(data[i]);
  }
  hal_->csHigh();
  waitDecode_();
  return true;
}

bool ADS1299_Protocol::readRegs(uint8_t startAddr, uint8_t* data, size_t n)
{
  if (rdatacActive_ || data == nullptr || !ADS1299Core::validRegisterRange(startAddr, n))
    return false;

  hal_->csLow();
  hal_->spiTransfer(ADS1299Core::readRegisterCommand(startAddr));
  hal_->spiTransfer((uint8_t)(n - 1));
  for (size_t i = 0; i < n; ++i) {
    data[i] = hal_->spiTransfer(ADS_CMD_NOP);
  }
  hal_->csHigh();
  waitDecode_();
  return true;
}

bool ADS1299_Protocol::readFrameBytes_(uint8_t channelCount, uint32_t& status24, int32_t* channels, size_t capacity)
{
  if (channelCount < ADS1299Core::MIN_CHANNELS ||
      channelCount > ADS1299Core::MAX_CHANNELS ||
      channels == nullptr ||
      capacity < channelCount)
    return false;

  const size_t nbytes = ADS1299Core::bytesPerFrame(channelCount);
  if (nbytes > ADS1299Core::BYTES_PER_FRAME_MAX)
    return false;

  uint8_t rxBuf[ADS1299Core::BYTES_PER_FRAME_MAX] = {0};
  hal_->csLow();
  for (size_t i = 0; i < nbytes; ++i) {
    rxBuf[i] = hal_->spiTransfer(ADS_CMD_NOP);
  }
  hal_->csHigh();

  return ADS1299Core::decodeFrame(rxBuf, channelCount, status24, channels, capacity);
}

bool ADS1299_Protocol::readFrameRDATAC(uint8_t channelCount, uint32_t& status24, int32_t* channels, size_t capacity)
{
  if (!rdatacActive_)
    return false;

  return readFrameBytes_(channelCount, status24, channels, capacity);
}

bool ADS1299_Protocol::readDataOnDemand(uint8_t channelCount, uint32_t& status24, int32_t* channels, size_t capacity)
{
  if (rdatacActive_)
    return false;

  if (channelCount < ADS1299Core::MIN_CHANNELS ||
      channelCount > ADS1299Core::MAX_CHANNELS ||
      channels == nullptr ||
      capacity < channelCount)
    return false;

  cmdRDATA();
  return readFrameBytes_(channelCount, status24, channels, capacity);
}

bool ADS1299_Protocol::isRDATACActive() const
{
  return rdatacActive_;
}
