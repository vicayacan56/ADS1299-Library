# HAL Usage Guide

ADS1299Plus currently supports two Arduino-compatible usage paths:

- The classic `ADS1299_SafeSPI` path.
- The optional HAL-backed path using `ADS1299_ArduinoHAL`.

The classic path remains the default for existing sketches. The HAL path is opt-in and is intended to prepare the library for future non-Arduino backends.

## Classic Arduino/SafeSPI path

Use this path for existing projects and for the `BasicRead` and `RegisterDump` examples.

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
```

## Optional Arduino HAL path

Use this path when you want to exercise the HAL-backed constructor on Arduino.

```cpp
#include <Arduino.h>
#include <ADS1299Plus.h>
#include <arduino/ADS1299_ArduinoHAL.h>

static constexpr uint8_t PIN_CS    = 10;
static constexpr uint8_t PIN_DRDY  = 7;
static constexpr uint8_t PIN_START = 9;
static constexpr uint8_t PIN_RESET = 8;
static constexpr uint8_t PIN_PWDN  = ADS1299_ArduinoHAL::PIN_UNUSED;

ADS1299_ArduinoHAL adsHal(PIN_CS, PIN_START, PIN_RESET, PIN_PWDN, PIN_DRDY);

ADS1299Plus::Pins adsPins = {
  PIN_CS,
  SCK,
  MOSI,
  MISO,
  PIN_DRDY,
  PIN_START,
  PIN_RESET,
  ADS1299Plus::ADS_PIN_UNUSED
};

ADS1299Plus ads(adsHal, adsPins);
```

See `examples/HalBasedRead` for a complete sketch.

## Choosing a path

- Use `ADS1299_SafeSPI` for the stable Arduino-compatible path.
- Use `ADS1299_ArduinoHAL` to test the HAL integration path.
- Keep `BasicRead` and `RegisterDump` as regression references for the classic path.
- Compare `HalBasedRead` against `BasicRead` when hardware is available.

## Hardware validation notes

Compiling a sketch checks API compatibility and linking. It does not prove electrical behavior.

With hardware attached, validate:

- Device ID detection.
- Register reads and writes.
- `configureDefaults()`.
- RDATAC frame sync.
- Stable channel count and frame size.
- `BasicRead` and `HalBasedRead` produce equivalent results.
