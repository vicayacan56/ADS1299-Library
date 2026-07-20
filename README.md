# ADS1299Plus

Arduino-compatible C++ driver for the Texas Instruments ADS1299-x family of 24-bit biopotential ADCs.

The library is intended to be easy to use from Arduino IDE while keeping a portable HAL foundation for future embedded backends.

## Supported Devices

- ADS1299-4
- ADS1299-6
- ADS1299

The device variant is detected automatically from the ADS1299 ID register during `begin()`.

## Features

- ADS1299 SPI command and register access.
- Dynamic 4/6/8-channel support.
- Conservative EEG-oriented default configuration.
- RDATAC continuous acquisition.
- On-demand `RDATA` frame reads.
- Lead-off and channel configuration helpers.
- Optional PWDN pin handling.
- Optional HAL-backed Arduino path for portability work.

## Installation

Install this repository as an Arduino library:

1. Download or clone the repository.
2. Place it in your Arduino libraries folder as `ADS1299Plus`.
3. Restart Arduino IDE.
4. Open one of the examples from `File > Examples > ADS1299Plus`.

The public include is:

```cpp
#include <ADS1299Plus.h>
```

## Quick Start

The recommended path for normal Arduino projects is the classic SafeSPI path:

```cpp
#include <Arduino.h>
#include <ADS1299Plus.h>
#include <ADS1299_SafeSPI.h>

static constexpr uint8_t PIN_CS    = 10;
static constexpr uint8_t PIN_DRDY  = 7;
static constexpr uint8_t PIN_START = 9;
static constexpr uint8_t PIN_RESET = 8;
static constexpr uint8_t PIN_PWDN  = ADS1299Plus::ADS_PIN_UNUSED;

ADS1299_SafeSPI adsSpi(PIN_CS);

ADS1299Plus::Pins adsPins = {
  PIN_CS,
  SCK,
  MOSI,
  MISO,
  PIN_DRDY,
  PIN_START,
  PIN_RESET,
  PIN_PWDN
};

ADS1299Plus ads(adsSpi, adsPins);

void setup() {
  Serial.begin(115200);

  if (!ads.begin()) {
    Serial.println("ADS1299 not detected");
    while (true) {}
  }

  ads.configureDefaults();
  ads.cmdStart();
  ads.cmdRDATAC();
}

void loop() {
  if (!ads.dataReady()) {
    return;
  }

  uint32_t status = 0;
  int32_t channels[ADS1299Plus::MAX_CHANNELS] = {0};

  if (ads.readFrameRDATAC(status, channels, ADS1299Plus::MAX_CHANNELS)) {
    for (uint8_t i = 0; i < ads.channelCount(); ++i) {
      Serial.print(channels[i]);
      Serial.print(i + 1 == ads.channelCount() ? '\n' : ',');
    }
  }
}
```

For complete sketches, use the examples.

## Examples

- `examples/BasicRead`: basic RDATAC acquisition using the classic Arduino/SafeSPI path.
- `examples/RegisterDump`: device/register inspection using the classic Arduino/SafeSPI path.
- `examples/HalBasedRead`: optional Arduino HAL-backed path for portability testing.

Start with `RegisterDump` when bringing up new hardware. Use `BasicRead` for the normal acquisition path.

## Optional HAL Path

The HAL path is available for advanced users and future backend work:

```cpp
#include <arduino/ADS1299_ArduinoHAL.h>

ADS1299_ArduinoHAL adsHal(PIN_CS, PIN_START, PIN_RESET, PIN_PWDN, PIN_DRDY);
ADS1299Plus ads(adsHal, adsPins);
```

Most Arduino users should keep using `ADS1299_SafeSPI`. See [docs/user/hal-usage-guide.md](docs/user/hal-usage-guide.md) for details.

## PWDN Pin

If the ADS1299 PWDN pin is connected directly to VDD, use:

```cpp
ADS1299Plus::ADS_PIN_UNUSED
```

If PWDN is connected to a microcontroller GPIO, pass that GPIO pin in `ADS1299Plus::Pins`.

## Validation Without Hardware

You can compile the examples without a connected ADS1299 board by using Arduino IDE `Verify/Compile`.

The repository also includes host-side tests for portable logic and HAL sequencing. See [docs/user/testing-without-hardware.md](docs/user/testing-without-hardware.md).

GitHub Actions also builds:

- host-side tests;
- Arduino examples for `arduino:avr:uno`.

## Hardware Validation

Compilation is not a substitute for hardware testing.

With a real ADS1299 board, validate:

- `RegisterDump` reads a valid ADS1299 ID.
- `BasicRead` reaches stable RDATAC frame acquisition.
- Detected channel count matches ADS1299-4, ADS1299-6, or ADS1299.
- STATUS sync remains stable during acquisition.
- `HalBasedRead` behaves equivalently to `BasicRead` if using the optional HAL path.

## Documentation

Start with [docs/README.md](docs/README.md) for a guided documentation index.

Most users only need:

- this README;
- [docs/user/user-guide.md](docs/user/user-guide.md);
- `examples/RegisterDump`;
- `examples/BasicRead`;
- [docs/user/hal-usage-guide.md](docs/user/hal-usage-guide.md);
- [docs/user/testing-without-hardware.md](docs/user/testing-without-hardware.md).

The phase documents in `docs/architecture/` and `docs/history/` are maintainer notes and historical design records.

## Current Status

ADS1299Plus is currently:

- Arduino-compatible;
- usable through the classic Arduino/SafeSPI path;
- optionally usable through an Arduino HAL-backed path;
- prepared internally for future portable backend work.

It is not yet a fully portable ADS1299 library for STM32, ESP-IDF, Zephyr, or bare-metal targets. Those backends are future work.
