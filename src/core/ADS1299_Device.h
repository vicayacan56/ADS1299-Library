// ADS1299_Device.h
// HAL-only ADS1299 device facade for portable backends.

#pragma once

#include "../ADS1299_Registers.h"
#include "../hal/ADS1299_HAL.h"
#include "ADS1299_Core.h"
#include "ADS1299_Protocol.h"
#include <stddef.h>
#include <stdint.h>

class ADS1299_Device {
public:
  static constexpr uint8_t MIN_CHANNELS = ADS1299Core::MIN_CHANNELS;
  static constexpr uint8_t MAX_CHANNELS = ADS1299Core::MAX_CHANNELS;
  static constexpr uint8_t NUM_CHANNELS = MAX_CHANNELS;

  static constexpr uint16_t STATUS_BYTES = ADS1299Core::STATUS_BYTES;
  static constexpr uint16_t BYTES_PER_CHANNEL = ADS1299Core::BYTES_PER_CHANNEL;
  static constexpr uint16_t BYTES_PER_FRAME_MAX = ADS1299Core::BYTES_PER_FRAME_MAX;

  static constexpr uint32_t DEFAULT_SPI_HZ = 2048000UL;

  static constexpr uint8_t kCFG1_Default = ADS_CFG1_250SPS;
  static constexpr uint8_t kCFG2_Default = ADS_CFG2_TEST_OFF;
  static constexpr uint8_t kCFG3_Default = ADS_CFG3_INTREF_NO_BIAS;
  static constexpr uint8_t kLOFF_Default = ADS_LOFF_AC_24NA_31HZ_87_5PCT_LEGACY;
  static inline uint8_t kCH_Default() { return ADS_CH_DEFAULT_GAIN24(); }
  static constexpr uint8_t kGPIO_Default = ADS_GPIO_ALL_INPUTS;
  static constexpr uint8_t kCFG4_Default = ADS_CFG4_CONT_LOFF_COMP_OFF;

  explicit ADS1299_Device(ADS1299_HAL& hal,
                          uint32_t spiHz = DEFAULT_SPI_HZ);

  bool begin();
  void end();
  bool configureDefaults();

  uint8_t channelCount() const { return numChannels_; }
  uint16_t bytesPerFrame() const { return ADS1299Core::bytesPerFrame(numChannels_); }
  static uint8_t channelsFromDeviceID(uint8_t id);

  void cmdWakeup();
  void cmdStandby();
  void cmdReset();
  void cmdStart();
  void cmdStop();
  void cmdRDATAC();
  void cmdSDATAC();
  void cmdRDATA();

  bool writeReg(uint8_t addr, uint8_t value);
  bool readReg(uint8_t addr, uint8_t& value);
  bool writeRegs(uint8_t startAddr, const uint8_t* data, size_t n);
  bool readRegs(uint8_t startAddr, uint8_t* data, size_t n);

  bool setDataRate(uint8_t dr3b);
  bool setClockOut(bool enable);
  bool setMultipleReadbackMode(bool enable);
  bool setDaisyEnable(bool enable);
  bool setChannel(uint8_t ch, uint8_t chsetByte);
  bool powerDownChannel(uint8_t ch, bool powerDown);
  bool setChannelGain(uint8_t ch, uint8_t gain3b);
  bool setChannelMux(uint8_t ch, uint8_t mux3b);
  bool setSRB2(uint8_t ch, bool enable);
  bool enableSRB1(bool enable);
  bool useInternalRef(bool enableBuffer);
  bool useBiasInternalRef(bool enableInternal);
  bool enableBiasBuffer(bool enable);
  bool routeBiasSense(bool enable);
  bool enableBiasMeasure(bool enable);
  bool configureLeadOff(uint8_t loffByte);
  bool enableLeadOffSenseP(uint8_t chMask);
  bool enableLeadOffSenseN(uint8_t chMask);
  bool setLeadOffFlip(uint8_t chMask);
  bool setSingleShot(bool singleShot);
  bool enableLoffComparators(bool enable);
  bool setBiasDeriveP(uint8_t chMask);
  bool setBiasDeriveN(uint8_t chMask);

  bool readFrameRDATAC(uint32_t& status24, int32_t* channels, size_t capacity);
  bool readDataOnDemand(uint32_t& status24, int32_t* channels, size_t capacity);
  bool dataReady() const;
  bool isRDATACActive() const { return protocol_.isRDATACActive(); }

  static inline bool statusHasSync(uint32_t status) {
    return ADS1299Core::statusHasSync(status);
  }
  static inline uint8_t statusLoffP(uint32_t status) { return ADS1299Core::statusLoffP(status); }
  static inline uint8_t statusLoffN(uint32_t status) { return ADS1299Core::statusLoffN(status); }
  static inline uint8_t statusGPIO(uint32_t status) { return ADS1299Core::statusGPIO(status); }
  static inline int32_t unpack24(const uint8_t bytes[3]) { return ADS1299Core::unpack24(bytes); }

  bool readDeviceID(uint8_t& id);

  void startConversions();
  void stopConversions();
  void resetPulse();
  void powerDown(bool active);

private:
  bool validCh_(uint8_t ch) const { return ADS1299Core::isValidChannel(ch, numChannels_); }
  static inline uint8_t chRegAddr_(uint8_t ch) { return ADS1299Core::channelRegisterAddress(ch); }
  uint8_t clipMask_(uint8_t mask) const { return ADS1299Core::clipChannelMask(mask, numChannels_); }

  ADS1299_HAL& hal_;
  ADS1299_Protocol protocol_;
  ADS1299_SpiConfig spiConfig_;
  uint8_t numChannels_ = MAX_CHANNELS;
};
