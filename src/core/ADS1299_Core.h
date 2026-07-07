// ADS1299_Core.h
// Portable ADS1299 helper logic with no platform dependencies.

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace ADS1299Core {

static constexpr uint8_t MIN_CHANNELS = 4;
static constexpr uint8_t MAX_CHANNELS = 8;
static constexpr uint16_t STATUS_BYTES = 3;
static constexpr uint16_t BYTES_PER_CHANNEL = 3;
static constexpr uint16_t BYTES_PER_FRAME_MAX =
    STATUS_BYTES + BYTES_PER_CHANNEL * MAX_CHANNELS;

uint8_t channelsFromDeviceID(uint8_t id);
uint16_t bytesPerFrame(uint8_t channelCount);
bool validRegisterRange(uint8_t startAddr, size_t n);
uint8_t clipChannelMask(uint8_t mask, uint8_t channelCount);

bool statusHasSync(uint32_t status);
uint8_t statusLoffP(uint32_t status);
uint8_t statusLoffN(uint32_t status);
uint8_t statusGPIO(uint32_t status);

int32_t unpack24(const uint8_t bytes[3]);

} // namespace ADS1299Core
