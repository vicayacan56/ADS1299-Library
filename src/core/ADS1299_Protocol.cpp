// ADS1299_Protocol.cpp

#include "ADS1299_Protocol.h"
#include "../ADS1299_Registers.h"

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

bool ADS1299_Protocol::isRDATACActive() const
{
  return rdatacActive_;
}
