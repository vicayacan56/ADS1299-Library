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
bool isValidChannel(uint8_t channel, uint8_t channelCount);
uint8_t channelRegisterAddress(uint8_t channel);
uint8_t clipChannelMask(uint8_t mask, uint8_t channelCount);
uint8_t readRegisterCommand(uint8_t address);
uint8_t writeRegisterCommand(uint8_t address);

uint8_t withDataRate(uint8_t config1, uint8_t dataRateBits);
uint8_t withClockOut(uint8_t config1, bool enable);
uint8_t withMultipleReadback(uint8_t config1, bool enable);
uint8_t withChannelPowerDown(uint8_t chset, bool powerDown);
uint8_t withChannelGain(uint8_t chset, uint8_t gainBits);
uint8_t withChannelMux(uint8_t chset, uint8_t muxBits);
uint8_t withSRB2(uint8_t chset, bool enable);
uint8_t withSRB1(uint8_t misc1, bool enable);
uint8_t withInternalRef(uint8_t config3, bool enableBuffer);
uint8_t withBiasInternalRef(uint8_t config3, bool enableInternal);
uint8_t withBiasBuffer(uint8_t config3, bool enable);
uint8_t withBiasLoffSense(uint8_t config3, bool enable);
uint8_t withBiasMeasure(uint8_t config3, bool enable);
uint8_t withSingleShot(uint8_t config4, bool enable);
uint8_t withLoffComparators(uint8_t config4, bool enable);

bool statusHasSync(uint32_t status);
uint8_t statusLoffP(uint32_t status);
uint8_t statusLoffN(uint32_t status);
uint8_t statusGPIO(uint32_t status);

int32_t unpack24(const uint8_t bytes[3]);
bool decodeFrame(const uint8_t* frame,
                 uint8_t channelCount,
                 uint32_t& status,
                 int32_t* channels,
                 size_t capacity);

} // namespace ADS1299Core
