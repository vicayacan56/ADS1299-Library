# HAL-Only Public API

This document defines the proposed public API for the future HAL-only branch.

It is a design document only. It does not change source code.

## Decision

Use a new HAL-first class name:

```text
ADS1299_Device
```

Keep `ADS1299Plus` as the Arduino-friendly public class for `main`.

Reason:

- `ADS1299Plus` already carries the classic Arduino/SafeSPI API history.
- A new class makes the portable branch easier to understand.
- Future STM32Cube, ESP-IDF, Zephyr, or bare-metal users should not need to understand `ADS1299_SafeSPI`.
- The branch can become HAL-only without making the stable Arduino release confusing.

## Intended Construction

The HAL-only API should be constructed with an `ADS1299_HAL` reference:

```cpp
ADS1299_Device ads(hal);
```

For Arduino-backed validation, that would look like:

```cpp
ADS1299_ArduinoHAL hal(PIN_CS, PIN_START, PIN_RESET, PIN_PWDN, PIN_DRDY);
ADS1299_Device ads(hal);
```

For a future STM32Cube backend:

```cpp
ADS1299_STM32CubeHAL hal(...);
ADS1299_Device ads(hal);
```

For a future ESP-IDF backend:

```cpp
ADS1299_ESP_IDF_HAL hal(...);
ADS1299_Device ads(hal);
```

## Pin Ownership

The portable class should not own Arduino pin numbers.

Pin mapping belongs to the backend:

```text
ADS1299_ArduinoHAL     owns Arduino pin numbers
ADS1299_STM32CubeHAL   owns GPIO ports and pins
ADS1299_ESP_IDF_HAL    owns gpio_num_t pins and SPI device handles
ADS1299_ZephyrHAL      owns devicetree devices/specs
```

Therefore `ADS1299_Device` should not expose `ADS1299Plus::Pins`.

This is one of the main differences from `ADS1299Plus`.

## Proposed API Surface

The HAL-only device class should initially expose the same practical ADS1299 operations that were validated in the Arduino library.

### Constants

```cpp
static constexpr uint8_t MIN_CHANNELS;
static constexpr uint8_t MAX_CHANNELS;
static constexpr uint8_t NUM_CHANNELS;
static constexpr uint16_t STATUS_BYTES;
static constexpr uint16_t BYTES_PER_CHANNEL;
static constexpr uint16_t BYTES_PER_FRAME_MAX;
```

Purpose:

- preserve safe array allocation;
- keep channel-count handling familiar;
- avoid Arduino-specific types or pins.

### Lifecycle

```cpp
bool begin(uint32_t spiHz = DEFAULT_SPI_HZ);
void end();
bool configureDefaults();
```

`begin()` should:

1. initialize the HAL;
2. configure the SPI transaction settings;
3. reset/wake the ADS1299;
4. read the device ID;
5. set the detected channel count.

`end()` should:

1. stop RDATAC if active;
2. stop conversions when appropriate;
3. let the HAL release platform resources.

### Device Information

```cpp
bool readDeviceID(uint8_t& id);
uint8_t channelCount() const;
uint16_t bytesPerFrame() const;
static uint8_t channelsFromDeviceID(uint8_t id);
```

These methods are portable and should remain.

### Commands

```cpp
void cmdWakeup();
void cmdStandby();
void cmdReset();
void cmdStart();
void cmdStop();
void cmdRDATAC();
void cmdSDATAC();
void cmdRDATA();
```

These map directly to ADS1299 SPI commands and belong in the portable API.

### Register Access

```cpp
bool writeReg(uint8_t addr, uint8_t value);
bool readReg(uint8_t addr, uint8_t& value);
bool writeRegs(uint8_t startAddr, const uint8_t* data, size_t n);
bool readRegs(uint8_t startAddr, uint8_t* data, size_t n);
```

Rules:

- reject null buffers;
- reject invalid ranges;
- reject register access while RDATAC is active;
- keep behavior aligned with `ADS1299Plus`.

### Configuration Helpers

Initial HAL-only API should include:

```cpp
bool setDataRate(uint8_t dr3b);
bool setClockOut(bool enable);
bool setMultipleReadbackMode(bool enable);
bool setDaisyEnable(bool enable);
bool setChannel(uint8_t ch, uint8_t chsetByte);
bool powerDownChannel(uint8_t ch, bool pd);
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
```

Reason:

- these are ADS1299 behaviors, not Arduino behaviors;
- keeping them avoids making the HAL-only branch less useful than the classic branch;
- they can be implemented through the same register helpers.

### Acquisition

```cpp
bool readFrameRDATAC(uint32_t& status24, int32_t* channels, size_t capacity);
bool readDataOnDemand(uint32_t& status24, int32_t* channels, size_t capacity);
bool dataReady() const;
bool isRDATACActive() const;
```

Rules:

- `readFrameRDATAC()` requires RDATAC active;
- `readDataOnDemand()` requires RDATAC inactive;
- both require `capacity >= channelCount()`;
- STATUS sync validation remains mandatory.

### STATUS Helpers

```cpp
static bool statusHasSync(uint32_t status);
static uint8_t statusLoffP(uint32_t status);
static uint8_t statusLoffN(uint32_t status);
static uint8_t statusGPIO(uint32_t status);
static int32_t unpack24(const uint8_t bytes[3]);
```

These remain portable and should delegate to `ADS1299Core`.

### Pin Helpers

The HAL-only class should expose ADS1299 device-level controls, not Arduino pin naming.

Recommended:

```cpp
void startConversions();
void stopConversions();
void resetPulse();
void powerDown(bool active);
```

Avoid Arduino-style names such as:

```cpp
pinStartHigh();
pinStartLow();
pinResetPulse();
pinPowerDown();
```

Those names belong to the Arduino-facing API.

## What ADS1299_Device Should Not Include

Do not include:

- `Arduino.h`;
- `SPI.h`;
- `ADS1299_SafeSPI.h`;
- Arduino pin numbers;
- PlatformIO-specific types;
- STM32/ESP-IDF/Zephyr-specific types;
- serial printing;
- example-only behavior.

## Relationship To Existing Pieces

Recommended internal shape:

```text
ADS1299_Device
  -> ADS1299_Protocol
  -> ADS1299_HAL
  -> backend
```

`ADS1299_Protocol` should remain responsible for:

- command transfer;
- register transfer;
- frame transfer;
- RDATAC state at protocol level.

`ADS1299_Device` should be responsible for:

- device lifecycle;
- device ID and channel count;
- default configuration;
- high-level register helper methods;
- user-facing acquisition methods;
- translating public API calls into protocol calls.

## Difference From ADS1299Plus

| Area | ADS1299Plus | ADS1299_Device |
| --- | --- | --- |
| Branch | `main` / Arduino release | `portable-core-hal` |
| Main purpose | Easy Arduino library | HAL-only portable device API |
| SafeSPI constructor | Yes | No |
| Arduino pins struct | Yes | No |
| HAL required | Optional | Required |
| Arduino includes | Yes | No |
| Future native backends | Not primary goal | Primary goal |

## E4 Acceptance Criteria

When implementation begins, E4 should pass only if:

- `ADS1299_Device.h` does not include Arduino headers;
- `ADS1299_Device.h` does not include `ADS1299_SafeSPI.h`;
- host tests compile;
- Arduino CLI examples still compile during transition;
- a new HAL-only host test proves construction and basic protocol calls;
- no public Arduino release behavior is changed by accident.

## E5 Example Direction

After `ADS1299_Device` exists, add examples that make the HAL-only branch obvious:

```text
examples/HalRegisterDump
examples/HalBasicRead
```

These examples should replace `RegisterDump` and `BasicRead` as the primary examples in `portable-core-hal`.

`RegisterDump` and `BasicRead` remain primary examples in `main`.

## Decision Summary

Use:

```text
ADS1299Plus   for the stable Arduino/SafeSPI release
ADS1299_Device for the HAL-only portability branch
```

This keeps the public Arduino story simple and gives the portable branch a clean identity.
