# ADS1299Plus

HAL-first C++ driver for the Texas Instruments ADS1299-x family of 24-bit biopotential ADCs.

This branch is the portable HAL branch. Its public device API is `ADS1299_Device`; Arduino support is provided through the `ADS1299_ArduinoHAL` backend.

If you want the stable classic Arduino/SafeSPI release, use the `main` branch.

## Supported Devices

- ADS1299-4
- ADS1299-6
- ADS1299

The device variant is detected automatically from the ADS1299 ID register during `begin()`.

## Current Backend Support

Validated:

- Arduino framework through `ADS1299_ArduinoHAL`
- Arduino AVR compile checks
- Arduino UNO Q compile and hardware smoke checks
- Host-side C++ tests using fake HAL objects

Planned, not yet implemented:

- STM32 HAL backend
- ESP-IDF backend
- Zephyr backend
- bare-metal backend examples

## Features

- HAL-only public device facade.
- Neutral SPI/GPIO/timing interface in `src/hal`.
- ADS1299 SPI command and register access.
- Conservative EEG-oriented default configuration.
- Dynamic 4/6/8-channel support.
- RDATAC continuous acquisition.
- On-demand `RDATA` frame reads.
- Lead-off and channel configuration helpers.
- Optional PWDN pin handling.
- Host-side tests for portable core, protocol sequencing, and device behavior.

## Installation In Arduino IDE

Install this repository as an Arduino library:

1. Download or clone the repository.
2. Place it in your Arduino libraries folder as `ADS1299Plus`.
3. Restart Arduino IDE.
4. Open an example from `File > Examples > ADS1299Plus`.

The public include for this branch is:

```cpp
#include <ADS1299_Device.h>
#include <arduino/ADS1299_ArduinoHAL.h>
```

## Quick Start

```cpp
#include <Arduino.h>
#include <ADS1299_Device.h>
#include <arduino/ADS1299_ArduinoHAL.h>

static constexpr uint8_t PIN_CS    = 10;
static constexpr uint8_t PIN_DRDY  = 7;
static constexpr uint8_t PIN_START = 9;
static constexpr uint8_t PIN_RESET = 8;
static constexpr uint8_t PIN_PWDN  = ADS1299_ArduinoHAL::PIN_UNUSED;

ADS1299_ArduinoHAL adsHal(PIN_CS, PIN_START, PIN_RESET, PIN_PWDN, PIN_DRDY);
ADS1299_Device ads(adsHal);

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!ads.begin()) {
    Serial.println("ADS1299 not detected");
    while (true) delay(1000);
  }

  ads.configureDefaults();
  ads.startConversions();
  delay(10);
  ads.cmdRDATAC();
}

void loop() {
  if (!ads.dataReady()) {
    return;
  }

  uint32_t status = 0;
  int32_t channels[ADS1299_Device::MAX_CHANNELS] = {0};

  if (ads.readFrameRDATAC(status, channels, ADS1299_Device::MAX_CHANNELS)) {
    for (uint8_t i = 0; i < ads.channelCount(); ++i) {
      Serial.print(channels[i]);
      Serial.print(i + 1 == ads.channelCount() ? '\n' : ',');
    }
  }
}
```

## Examples

- `examples/HalRegisterDump`: first hardware diagnostic. It reads the ADS1299 ID, detects channel count, applies defaults, and dumps key registers.
- `examples/HalBasicRead`: basic RDATAC acquisition through the HAL-only path.

Start with `HalRegisterDump` when bringing up new hardware. Use `HalBasicRead` after register access is confirmed.

## Hardware Validation

Compilation is useful, but it does not prove electrical behavior.

With a real ADS1299 board, validate:

- `HalRegisterDump` reads a valid ADS1299 ID.
- Detected channel count matches ADS1299-4, ADS1299-6, or ADS1299.
- Register values after `configureDefaults()` match the expected defaults.
- `HalBasicRead` reaches stable RDATAC frame acquisition.
- STATUS sync remains stable during acquisition.

## Documentation

Start with [docs/README.md](docs/README.md) for the documentation index.

Most users need:

- [docs/user/user-guide.md](docs/user/user-guide.md)
- [docs/user/hal-usage-guide.md](docs/user/hal-usage-guide.md)
- [docs/user/testing-without-hardware.md](docs/user/testing-without-hardware.md)
- `examples/HalRegisterDump`
- `examples/HalBasicRead`

Maintainer notes and historical phase documents live under `docs/architecture/` and `docs/history/`.
