// ADS1299_Protocol.cpp

#include "ADS1299_Protocol.h"

ADS1299_Protocol::ADS1299_Protocol(ADS1299_HAL& hal)
    : hal_(&hal)
{
}

bool ADS1299_Protocol::isRDATACActive() const
{
  return rdatacActive_;
}
