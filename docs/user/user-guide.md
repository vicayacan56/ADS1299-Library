# ADS1299Plus User Guide

This guide explains the recommended way to use ADS1299Plus from Arduino IDE.

Use this guide if you want to connect an ADS1299 board, compile the examples, read registers, and start acquiring frames.

## Recommended Starting Point

Start with the classic Arduino/SafeSPI path.

This is the default and most stable path for normal Arduino projects:

```cpp
ADS1299_SafeSPI adsSpi(PIN_CS);
ADS1299Plus ads(adsSpi, adsPins);
```

The optional HAL path is useful for portability work, but most users do not need it for a first setup.

## Supported ADS1299 Variants

ADS1299Plus supports:

- ADS1299-4
- ADS1299-6
- ADS1299

The library detects the channel count during `begin()` by reading the ADS1299 ID register.

Always allocate channel arrays with `ADS1299Plus::MAX_CHANNELS`, then loop only over `ads.channelCount()`:

```cpp
int32_t channels[ADS1299Plus::MAX_CHANNELS] = {0};

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
ADS1299Plus::ADS_PIN_UNUSED
```

## Bring-Up Flow

For new hardware, use this order:

1. Install the library in Arduino IDE.
2. Compile `RegisterDump` without hardware errors.
3. Connect the ADS1299 board.
4. Run `RegisterDump` and confirm the device ID is valid.
5. Run `BasicRead`.
6. Confirm stable STATUS sync and frame output.
7. Only then modify register settings or channel configuration.

This keeps hardware bring-up simpler and makes wiring issues easier to isolate.

## Example: RegisterDump

Use `examples/RegisterDump` first.

It is meant to answer:

- Does the board respond over SPI?
- Is the ADS1299 ID valid?
- Are register reads working?
- Is the detected channel count reasonable?

If `RegisterDump` cannot read the device ID, check wiring, power, SPI pins, CS, RESET, and PWDN before trying acquisition.

## Example: BasicRead

Use `examples/BasicRead` after `RegisterDump` works.

It uses:

- the classic Arduino/SafeSPI path;
- `configureDefaults()`;
- `START`;
- `RDATAC`;
- `dataReady()`;
- `readFrameRDATAC()`.

This is the normal acquisition reference example.

## Example: HalBasedRead

Use `examples/HalBasedRead` only after the classic path works.

It exercises the optional Arduino HAL path:

```cpp
ADS1299_ArduinoHAL adsHal(PIN_CS, PIN_START, PIN_RESET, PIN_PWDN, PIN_DRDY);
ADS1299Plus ads(adsHal, adsPins);
```

With real hardware, `HalBasedRead` should behave like `BasicRead`.

## Reading Frames

Use `readFrameRDATAC()` while RDATAC mode is active:

```cpp
uint32_t status = 0;
int32_t channels[ADS1299Plus::MAX_CHANNELS] = {0};

if (ads.readFrameRDATAC(status, channels, ADS1299Plus::MAX_CHANNELS)) {
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
int32_t channels[ADS1299Plus::MAX_CHANNELS] = {0};

if (ads.readDataOnDemand(status, channels, ADS1299Plus::MAX_CHANNELS)) {
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

ads.writeReg(ADS_REG_CONFIG1, ADS1299Plus::kCFG1_Default);
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

The include should be:

```cpp
#include <ADS1299Plus.h>
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

Run `RegisterDump` before `BasicRead`.

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

- Arduino IDE `Verify/Compile` for `BasicRead`, `RegisterDump`, and `HalBasedRead`.
- Host-side tests with desktop `g++`.

See [Testing Without Hardware](testing-without-hardware.md).

## Hardware Validation Checklist

Before treating a setup as validated, check:

- `RegisterDump` reads a valid ADS1299 ID.
- `BasicRead` reads stable frames.
- STATUS sync remains valid over time.
- `ads.channelCount()` matches the board variant.
- `HalBasedRead` behaves like `BasicRead` if using HAL.

## More Documentation

- [HAL Usage Guide](hal-usage-guide.md)
- [Testing Without Hardware](testing-without-hardware.md)
- [Documentation Index](../README.md)
