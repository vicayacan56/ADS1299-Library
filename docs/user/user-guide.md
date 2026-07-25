# ADS1299Plus User Guide

This guide explains the recommended way to use the HAL-only `portable-core-hal` branch from Arduino IDE or Arduino CLI.

Use the `main` branch if you need the stable classic Arduino/SafeSPI API.

## Recommended Starting Point

This branch uses one public device path:

```cpp
#include <ADS1299_Device.h>
#include <arduino/ADS1299_ArduinoHAL.h>

ADS1299_ArduinoHAL adsHal(PIN_CS, PIN_START, PIN_RESET, PIN_PWDN, PIN_DRDY);
ADS1299_Device ads(adsHal);
```

Arduino is the first backend. The ADS1299 behavior is driven through the neutral HAL contract instead of direct Arduino SPI calls in the device facade.

## Supported ADS1299 Variants

ADS1299Plus supports:

- ADS1299-4
- ADS1299-6
- ADS1299

The library detects the channel count during `begin()` by reading the ADS1299 ID register.

Always allocate channel arrays with `ADS1299_Device::MAX_CHANNELS`, then loop only over `ads.channelCount()`:

```cpp
int32_t channels[ADS1299_Device::MAX_CHANNELS] = {0};

for (uint8_t i = 0; i < ads.channelCount(); ++i) {
  Serial.println(channels[i]);
}
```

## Basic Wiring

At minimum, the driver expects these ADS1299 signals:

| ADS1299 signal | MCU signal |
| --- | --- |
| CS | GPIO output |
| SCLK | hardware SPI SCK |
| DIN | hardware SPI MOSI |
| DOUT | hardware SPI MISO |
| DRDY | GPIO input |
| START | GPIO output |
| RESET | GPIO output |
| PWDN | GPIO output or tied to VDD |

If PWDN is tied directly to VDD, use:

```cpp
ADS1299_ArduinoHAL::PIN_UNUSED
```

## Bring-Up Flow

For new hardware, use this order:

1. Install the library in Arduino IDE or make it visible to Arduino CLI.
2. Compile `HalRegisterDump` without hardware errors.
3. Connect the ADS1299 board.
4. Upload `HalRegisterDump` and confirm the device ID is valid.
5. Confirm the detected channel count is correct.
6. Upload `HalBasicRead`.
7. Confirm stable STATUS sync and frame output.
8. Only then modify register settings or channel configuration.

This keeps hardware bring-up simple and makes wiring issues easier to isolate.

## Example: HalRegisterDump

Use `examples/HalRegisterDump` first.

It is meant to answer:

- Does the board respond over SPI?
- Is the ADS1299 ID valid?
- Are register reads working?
- Is the detected channel count reasonable?
- Do default register writes complete correctly?

If `HalRegisterDump` cannot read the device ID, check wiring, power, SPI pins, CS, RESET, and PWDN before trying acquisition.

## Example: HalBasicRead

Use `examples/HalBasicRead` after `HalRegisterDump` works.

It uses:

- `ADS1299_Device`
- `ADS1299_ArduinoHAL`
- `configureDefaults()`
- `startConversions()`
- `cmdRDATAC()`
- `dataReady()`
- `readFrameRDATAC()`

This is the normal acquisition reference example for the HAL-only branch.

## Reading Frames

Use `readFrameRDATAC()` while RDATAC mode is active:

```cpp
uint32_t status = 0;
int32_t channels[ADS1299_Device::MAX_CHANNELS] = {0};

if (ads.readFrameRDATAC(status, channels, ADS1299_Device::MAX_CHANNELS)) {
  for (uint8_t i = 0; i < ads.channelCount(); ++i) {
    Serial.println(channels[i]);
  }
}
```

The function returns `false` if:

- RDATAC is not active;
- the output pointer is null;
- the capacity is smaller than `ads.channelCount()`;
- the STATUS sync bits are invalid.

## Reading One Frame On Demand

Use `readDataOnDemand()` only when RDATAC is not active:

```cpp
uint32_t status = 0;
int32_t channels[ADS1299_Device::MAX_CHANNELS] = {0};

if (ads.readDataOnDemand(status, channels, ADS1299_Device::MAX_CHANNELS)) {
  // One frame was read using the ADS1299 RDATA command.
}
```

## Register Access

Register access must be done outside RDATAC mode.

The library rejects register reads and writes while RDATAC is active.

Use:

```cpp
ads.cmdSDATAC();

uint8_t value = 0;
ads.readReg(ADS_REG_CONFIG1, value);

ads.writeReg(ADS_REG_CONFIG1, ADS1299_Device::kCFG1_Default);
```

## Default Configuration

`configureDefaults()` applies a conservative EEG-oriented setup:

- 250 SPS;
- internal reference enabled;
- gain 24 channel defaults;
- normal differential inputs;
- bias drive disabled by default;
- lead-off comparators disabled by default;
- GPIO as inputs.

These defaults are intentionally conservative.

## Common Problems

### The sketch does not compile

Check that the repository is installed as an Arduino library named `ADS1299Plus`.

The includes should be:

```cpp
#include <ADS1299_Device.h>
#include <arduino/ADS1299_ArduinoHAL.h>
```

### `begin()` fails

Check:

- board power;
- GND shared with the microcontroller;
- CS wiring;
- SPI SCK/MOSI/MISO wiring;
- RESET wiring;
- PWDN state;
- selected Arduino board and SPI pins.

Run `HalRegisterDump` before `HalBasicRead`.

### Frames are not stable

Check:

- DRDY pin wiring;
- START pin wiring;
- sample rate expectations;
- power supply quality;
- ADS1299 clock/reference configuration;
- whether STATUS sync remains valid.

### Register writes fail

Make sure RDATAC is stopped first:

```cpp
ads.cmdSDATAC();
```

## Validation Without Hardware

You can verify compilation without a board:

- Arduino IDE `Verify/Compile` for `HalRegisterDump` and `HalBasicRead`.
- Arduino CLI compile checks for supported boards.
- Host-side tests with desktop `g++`.

See [Testing Without Hardware](testing-without-hardware.md).

## Hardware Validation Checklist

Before treating a setup as validated, check:

- `HalRegisterDump` reads a valid ADS1299 ID.
- `ads.channelCount()` matches the board variant.
- `HalBasicRead` reads stable frames.
- STATUS sync remains valid over time.
- Register access still works after stopping RDATAC.

## More Documentation

- [HAL Usage Guide](hal-usage-guide.md)
- [Testing Without Hardware](testing-without-hardware.md)
- [Documentation Index](../README.md)
