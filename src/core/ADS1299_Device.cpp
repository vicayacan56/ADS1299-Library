// ADS1299_Device.cpp

#include "ADS1299_Device.h"

ADS1299_Device::ADS1299_Device(ADS1299_HAL& hal, uint32_t spiHz)
    : hal_(hal),
      protocol_(hal),
      spiConfig_()
{
  spiConfig_.clockHz = spiHz;
}

uint8_t ADS1299_Device::channelsFromDeviceID(uint8_t id)
{
  return ADS1299Core::channelsFromDeviceID(id);
}

bool ADS1299_Device::begin()
{
  hal_.begin();
  hal_.beginTransaction(spiConfig_);
  hal_.delayMilliseconds(5);

  cmdReset();
  cmdSDATAC();
  cmdStop();

  uint8_t id = 0;
  if (!readReg(ADS_REG_ID, id))
    return false;

  if (!ADS_ID_DEV_IS_1299(id))
    return false;

  const uint8_t detectedChannels = channelsFromDeviceID(id);
  if (detectedChannels == 0)
    return false;

  numChannels_ = detectedChannels;
  return true;
}

void ADS1299_Device::end()
{
  cmdStop();
  cmdSDATAC();
  hal_.endTransaction();
  hal_.end();
}

bool ADS1299_Device::configureDefaults()
{
  cmdSDATAC();
  cmdStop();

  if (!writeReg(ADS_REG_CONFIG1, kCFG1_Default))
    return false;
  if (!writeReg(ADS_REG_CONFIG2, kCFG2_Default))
    return false;
  if (!writeReg(ADS_REG_CONFIG3, kCFG3_Default))
    return false;
  if (!writeReg(ADS_REG_LOFF, kLOFF_Default))
    return false;

  for (uint8_t ch = 1; ch <= numChannels_; ++ch) {
    if (!setChannel(ch, kCH_Default()))
      return false;
  }

  if (!writeReg(ADS_REG_BIAS_SENSP, 0x00))
    return false;
  if (!writeReg(ADS_REG_BIAS_SENSN, 0x00))
    return false;

  const uint8_t activeMask = ADS1299Core::clipChannelMask(0xFF, numChannels_);
  if (!enableLeadOffSenseP(activeMask))
    return false;
  if (!enableLeadOffSenseN(activeMask))
    return false;

  if (!writeReg(ADS_REG_LOFF_FLIP, 0x00))
    return false;
  if (!writeReg(ADS_REG_GPIO, kGPIO_Default))
    return false;
  if (!writeReg(ADS_REG_MISC1, 0x00))
    return false;
  if (!writeReg(ADS_REG_CONFIG4, kCFG4_Default))
    return false;

  return true;
}

void ADS1299_Device::cmdWakeup() { protocol_.cmdWakeup(); }
void ADS1299_Device::cmdStandby() { protocol_.cmdStandby(); }
void ADS1299_Device::cmdReset() { protocol_.cmdReset(); }
void ADS1299_Device::cmdStart() { protocol_.cmdStart(); }
void ADS1299_Device::cmdStop() { protocol_.cmdStop(); }
void ADS1299_Device::cmdRDATAC() { protocol_.cmdRDATAC(); }
void ADS1299_Device::cmdSDATAC() { protocol_.cmdSDATAC(); }
void ADS1299_Device::cmdRDATA() { protocol_.cmdRDATA(); }

bool ADS1299_Device::writeReg(uint8_t addr, uint8_t value)
{
  return protocol_.writeReg(addr, value);
}

bool ADS1299_Device::readReg(uint8_t addr, uint8_t& value)
{
  return protocol_.readReg(addr, value);
}

bool ADS1299_Device::writeRegs(uint8_t startAddr, const uint8_t* data, size_t n)
{
  return protocol_.writeRegs(startAddr, data, n);
}

bool ADS1299_Device::readRegs(uint8_t startAddr, uint8_t* data, size_t n)
{
  return protocol_.readRegs(startAddr, data, n);
}

bool ADS1299_Device::setDataRate(uint8_t dr3b)
{
  uint8_t cfg1 = 0;
  if (!readReg(ADS_REG_CONFIG1, cfg1))
    return false;
  return writeReg(ADS_REG_CONFIG1, ADS1299Core::withDataRate(cfg1, dr3b));
}

bool ADS1299_Device::setClockOut(bool enable)
{
  uint8_t cfg1 = 0;
  if (!readReg(ADS_REG_CONFIG1, cfg1))
    return false;
  return writeReg(ADS_REG_CONFIG1, ADS1299Core::withClockOut(cfg1, enable));
}

bool ADS1299_Device::setMultipleReadbackMode(bool enable)
{
  uint8_t cfg1 = 0;
  if (!readReg(ADS_REG_CONFIG1, cfg1))
    return false;
  return writeReg(ADS_REG_CONFIG1, ADS1299Core::withMultipleReadback(cfg1, enable));
}

bool ADS1299_Device::setDaisyEnable(bool enable)
{
  return setMultipleReadbackMode(enable);
}

bool ADS1299_Device::setChannel(uint8_t ch, uint8_t chsetByte)
{
  if (!validCh_(ch))
    return false;
  return writeReg(chRegAddr_(ch), chsetByte);
}

bool ADS1299_Device::powerDownChannel(uint8_t ch, bool powerDown)
{
  if (!validCh_(ch))
    return false;
  uint8_t value = 0;
  if (!readReg(chRegAddr_(ch), value))
    return false;
  return writeReg(chRegAddr_(ch), ADS1299Core::withChannelPowerDown(value, powerDown));
}

bool ADS1299_Device::setChannelGain(uint8_t ch, uint8_t gain3b)
{
  if (!validCh_(ch))
    return false;
  uint8_t value = 0;
  if (!readReg(chRegAddr_(ch), value))
    return false;
  return writeReg(chRegAddr_(ch), ADS1299Core::withChannelGain(value, gain3b));
}

bool ADS1299_Device::setChannelMux(uint8_t ch, uint8_t mux3b)
{
  if (!validCh_(ch))
    return false;
  uint8_t value = 0;
  if (!readReg(chRegAddr_(ch), value))
    return false;
  return writeReg(chRegAddr_(ch), ADS1299Core::withChannelMux(value, mux3b));
}

bool ADS1299_Device::setSRB2(uint8_t ch, bool enable)
{
  if (!validCh_(ch))
    return false;
  uint8_t value = 0;
  if (!readReg(chRegAddr_(ch), value))
    return false;
  return writeReg(chRegAddr_(ch), ADS1299Core::withSRB2(value, enable));
}

bool ADS1299_Device::enableSRB1(bool enable)
{
  uint8_t misc1 = 0;
  if (!readReg(ADS_REG_MISC1, misc1))
    return false;
  return writeReg(ADS_REG_MISC1, ADS1299Core::withSRB1(misc1, enable));
}

bool ADS1299_Device::useInternalRef(bool enableBuffer)
{
  uint8_t cfg3 = 0;
  if (!readReg(ADS_REG_CONFIG3, cfg3))
    return false;
  return writeReg(ADS_REG_CONFIG3, ADS1299Core::withInternalRef(cfg3, enableBuffer));
}

bool ADS1299_Device::useBiasInternalRef(bool enableInternal)
{
  uint8_t cfg3 = 0;
  if (!readReg(ADS_REG_CONFIG3, cfg3))
    return false;
  return writeReg(ADS_REG_CONFIG3, ADS1299Core::withBiasInternalRef(cfg3, enableInternal));
}

bool ADS1299_Device::enableBiasBuffer(bool enable)
{
  uint8_t cfg3 = 0;
  if (!readReg(ADS_REG_CONFIG3, cfg3))
    return false;
  return writeReg(ADS_REG_CONFIG3, ADS1299Core::withBiasBuffer(cfg3, enable));
}

bool ADS1299_Device::routeBiasSense(bool enable)
{
  uint8_t cfg3 = 0;
  if (!readReg(ADS_REG_CONFIG3, cfg3))
    return false;
  return writeReg(ADS_REG_CONFIG3, ADS1299Core::withBiasLoffSense(cfg3, enable));
}

bool ADS1299_Device::enableBiasMeasure(bool enable)
{
  uint8_t cfg3 = 0;
  if (!readReg(ADS_REG_CONFIG3, cfg3))
    return false;
  return writeReg(ADS_REG_CONFIG3, ADS1299Core::withBiasMeasure(cfg3, enable));
}

bool ADS1299_Device::configureLeadOff(uint8_t loffByte)
{
  return writeReg(ADS_REG_LOFF, loffByte);
}

bool ADS1299_Device::enableLeadOffSenseP(uint8_t chMask)
{
  return writeReg(ADS_REG_LOFF_SENSP, clipMask_(chMask));
}

bool ADS1299_Device::enableLeadOffSenseN(uint8_t chMask)
{
  return writeReg(ADS_REG_LOFF_SENSN, clipMask_(chMask));
}

bool ADS1299_Device::setLeadOffFlip(uint8_t chMask)
{
  return writeReg(ADS_REG_LOFF_FLIP, clipMask_(chMask));
}

bool ADS1299_Device::setSingleShot(bool singleShot)
{
  uint8_t cfg4 = 0;
  if (!readReg(ADS_REG_CONFIG4, cfg4))
    return false;
  return writeReg(ADS_REG_CONFIG4, ADS1299Core::withSingleShot(cfg4, singleShot));
}

bool ADS1299_Device::enableLoffComparators(bool enable)
{
  uint8_t cfg4 = 0;
  if (!readReg(ADS_REG_CONFIG4, cfg4))
    return false;
  return writeReg(ADS_REG_CONFIG4, ADS1299Core::withLoffComparators(cfg4, enable));
}

bool ADS1299_Device::setBiasDeriveP(uint8_t chMask)
{
  return writeReg(ADS_REG_BIAS_SENSP, clipMask_(chMask));
}

bool ADS1299_Device::setBiasDeriveN(uint8_t chMask)
{
  return writeReg(ADS_REG_BIAS_SENSN, clipMask_(chMask));
}

bool ADS1299_Device::readFrameRDATAC(uint32_t& status24, int32_t* channels, size_t capacity)
{
  return protocol_.readFrameRDATAC(numChannels_, status24, channels, capacity);
}

bool ADS1299_Device::readDataOnDemand(uint32_t& status24, int32_t* channels, size_t capacity)
{
  return protocol_.readDataOnDemand(numChannels_, status24, channels, capacity);
}

bool ADS1299_Device::dataReady() const
{
  return !hal_.readDrdy();
}

bool ADS1299_Device::readDeviceID(uint8_t& id)
{
  return readReg(ADS_REG_ID, id);
}

void ADS1299_Device::startConversions()
{
  hal_.setStart(true);
}

void ADS1299_Device::stopConversions()
{
  hal_.setStart(false);
}

void ADS1299_Device::resetPulse()
{
  hal_.setReset(false);
  hal_.delayMicroseconds(10);
  hal_.setReset(true);
  hal_.delayMicroseconds(20);
}

void ADS1299_Device::powerDown(bool active)
{
  hal_.setPwdn(!active);
}
