// ADS1299_Core.cpp

#include "ADS1299_Core.h"
#include "../ADS1299_Registers.h"

namespace ADS1299Core {

uint8_t channelsFromDeviceID(uint8_t id)
{
  if (!ADS_ID_DEV_IS_1299(id))
    return 0;

  switch (id & ADS_ID_NU_CH_MASK)
  {
    case 0b00: return 4; // ADS1299-4
    case 0b01: return 6; // ADS1299-6
    case 0b10: return 8; // ADS1299
    default:   return 0; // reserved / unsupported
  }
}

uint16_t bytesPerFrame(uint8_t channelCount)
{
  return STATUS_BYTES + BYTES_PER_CHANNEL * channelCount;
}

bool validRegisterRange(uint8_t startAddr, size_t n)
{
  return n > 0 &&
         startAddr <= ADS_REG_CONFIG4 &&
         (size_t)startAddr + n - 1 <= ADS_REG_CONFIG4;
}

uint8_t clipChannelMask(uint8_t mask, uint8_t channelCount)
{
  static const uint8_t lut[9] = {
    0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF
  };

  if (channelCount > MAX_CHANNELS)
    channelCount = MAX_CHANNELS;

  return (uint8_t)(mask & lut[channelCount]);
}

bool statusHasSync(uint32_t status)
{
  return (status & ADS_STATUS_SYNC_MASK) == ADS_STATUS_SYNC_VAL;
}

uint8_t statusLoffP(uint32_t status)
{
  return ADS_STATUS_LOFFP(status);
}

uint8_t statusLoffN(uint32_t status)
{
  return ADS_STATUS_LOFFN(status);
}

uint8_t statusGPIO(uint32_t status)
{
  return ADS_STATUS_GPIO4_1(status);
}

int32_t unpack24(const uint8_t bytes[3])
{
  uint32_t value =
      ((uint32_t)bytes[0] << 16) |
      ((uint32_t)bytes[1] << 8) |
      bytes[2];

  if (value & 0x00800000UL)
    value |= 0xFF000000UL;

  return (int32_t)value;
}

bool decodeFrame(const uint8_t* frame,
                 uint8_t channelCount,
                 uint32_t& status,
                 int32_t* channels,
                 size_t capacity)
{
  if (frame == nullptr ||
      channels == nullptr ||
      channelCount < MIN_CHANNELS ||
      channelCount > MAX_CHANNELS ||
      capacity < channelCount) {
    return false;
  }

  status =
      ((uint32_t)frame[0] << 16) |
      ((uint32_t)frame[1] << 8) |
      frame[2];

  for (uint8_t i = 0; i < channelCount; ++i) {
    channels[i] = unpack24(&frame[STATUS_BYTES + BYTES_PER_CHANNEL * i]);
  }

  for (size_t i = channelCount; i < capacity && i < MAX_CHANNELS; ++i) {
    channels[i] = 0;
  }

  return statusHasSync(status);
}

} // namespace ADS1299Core
