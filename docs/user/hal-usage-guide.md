# HAL Usage Guide

This branch exposes the ADS1299 driver through a HAL-only public path.

The device layer does not call Arduino SPI or GPIO APIs directly. It calls the neutral `ADS1299_HAL` interface, and `ADS1299_ArduinoHAL` translates those operations to Arduino.

## Public Path

Use:

```cpp
#include <ADS1299_Device.h>
#include <arduino/ADS1299_ArduinoHAL.h>

ADS1299_ArduinoHAL adsHal(PIN_CS, PIN_START, PIN_RESET, PIN_PWDN, PIN_DRDY);
ADS1299_Device ads(adsHal);
```

The runtime path is:

```text
ADS1299_Device -> ADS1299_Protocol -> ADS1299_HAL -> ADS1299_ArduinoHAL -> Arduino SPI/GPIO
```

## Arduino Backend

`ADS1299_ArduinoHAL` owns the Arduino-specific parts:

- `SPI.begin()`
- `SPI.beginTransaction(SPISettings(...))`
- `SPI.transfer(...)`
- `SPI.endTransaction()`
- `pinMode(...)`
- `digitalRead(...)`
- `digitalWrite(...)`
- `delay(...)`
- `delayMicroseconds(...)`

The neutral driver code above it only sees ADS1299 protocol operations and HAL calls.

## SPI Configuration

`ADS1299_Device` uses a neutral SPI configuration:

```cpp
ADS1299_SpiConfig config;
config.clockHz = 2048000UL;
config.bitOrder = ADS1299_SpiBitOrder::MSB_FIRST;
config.mode = ADS1299_SpiMode::MODE1;
```

On Arduino, `ADS1299_ArduinoHAL` converts that to:

- `MSBFIRST` or `LSBFIRST`
- `SPI_MODE0`, `SPI_MODE1`, `SPI_MODE2`, or `SPI_MODE3`
- `SPISettings(clockHz, bitOrder, mode)`

## PWDN Pin

If the ADS1299 PWDN pin is tied directly to VDD, use:

```cpp
static constexpr uint8_t PIN_PWDN = ADS1299_ArduinoHAL::PIN_UNUSED;
```

If PWDN is connected to a GPIO, pass that GPIO pin to `ADS1299_ArduinoHAL`.

## Register Access

Register access is performed through `ADS1299_Device`:

```cpp
uint8_t value = 0;
ads.readReg(ADS_REG_CONFIG1, value);
ads.writeReg(ADS_REG_CONFIG1, ADS1299_Device::kCFG1_Default);
```

Register reads and writes are rejected while RDATAC is active. Stop RDATAC first:

```cpp
ads.cmdSDATAC();
```

## Acquisition

The normal acquisition sequence is:

```cpp
ads.configureDefaults();
ads.startConversions();
delay(10);
ads.cmdRDATAC();
```

Then read frames only when DRDY is active:

```cpp
if (ads.dataReady()) {
  uint32_t status = 0;
  int32_t channels[ADS1299_Device::MAX_CHANNELS] = {0};
  ads.readFrameRDATAC(status, channels, ADS1299_Device::MAX_CHANNELS);
}
```

## Backend Porting

To support another embedded environment, create a new class that implements `ADS1299_HAL`.

The new backend must provide:

- SPI initialization and transactions.
- Byte transfer.
- GPIO mode, read, and write.
- Millisecond and microsecond delays.

The ADS1299 device API should remain unchanged when adding a new backend.

## Hardware Validation Notes

Compiling a sketch checks API compatibility and linking. It does not prove electrical behavior.

With hardware attached, validate:

- Device ID detection.
- Register reads and writes.
- `configureDefaults()`.
- RDATAC frame sync.
- Stable channel count and frame size.
- STATUS sync over time.
