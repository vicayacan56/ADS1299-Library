# ADS1299Plus

ADS1299Plus is an Arduino-compatible C++ driver for the Texas Instruments ADS1299-x family of 24-bit biopotential analog-to-digital converters.

## Features

- SPI communication with the ADS1299-x.
- Register-level configuration.
- Dynamic channel detection for ADS1299-4, ADS1299-6 and ADS1299.
- EEG-oriented default configuration.
- Lead-off helper functions.
- Continuous data acquisition using RDATAC mode.

## Supported devices

- ADS1299-4
- ADS1299-6
- ADS1299

## Current status

This version is Arduino-compatible and now includes an optional HAL-backed Arduino path.

It is intended for boards and environments that provide:

- Arduino framework
- SPI library
- GPIO functions
- delay and delayMicroseconds functions

The classic `ADS1299_SafeSPI` path remains the default and is used by the original examples. The optional `ADS1299_ArduinoHAL` path is available for portability work and future backend development.

## Examples

See:

- `examples/RegisterDump`
- `examples/BasicRead`
- `examples/HalBasedRead`

`BasicRead` and `RegisterDump` use the classic Arduino/SafeSPI path. `HalBasedRead` exercises the optional Arduino HAL path.

## Usage paths

Classic Arduino path:

```cpp
ADS1299_SafeSPI adsSpi(PIN_CS);
ADS1299Plus ads(adsSpi, adsPins);
```

Optional Arduino HAL path:

```cpp
ADS1299_ArduinoHAL adsHal(PIN_CS, PIN_START, PIN_RESET, PIN_PWDN, PIN_DRDY);
ADS1299Plus ads(adsHal, adsPins);
```

See `docs/hal-usage-guide.md` for details.

## Testing without hardware

You can compile the Arduino examples without a connected board by using Arduino IDE `Verify/Compile`.

The repository also includes host-side tests that run with a desktop `g++` compiler:

```powershell
g++ -std=c++11 -I src tests/host/test_ads1299_core.cpp src/core/ADS1299_Core.cpp -o tests/host/test_ads1299_core.exe
.\tests\host\test_ads1299_core.exe

g++ -std=c++11 -I tests/host/arduino_stubs -I src -I src/hal tests/host/test_ads1299_host.cpp src/core/ADS1299_Core.cpp src/ADS1299Plus.cpp src/ADS1299_SafeSPI.cpp -o tests/host/test_ads1299_host.exe
.\tests\host\test_ads1299_host.exe
```

Expected output:

```text
core tests passed
host tests passed
```

See `docs/testing-without-hardware.md` for details.

## PWDN pin

If the ADS1299 PWDN pin is connected directly to VDD, use:

```cpp
ADS1299Plus::ADS_PIN_UNUSED
```


If PWDN is connected to a microcontroller GPIO, pass the GPIO pin to the library.

## Repository structure
src/
  ADS1299Plus.h
  ADS1299Plus.cpp
  ADS1299_SafeSPI.h
  ADS1299_SafeSPI.cpp
  ADS1299_Registers.h
  core/
  hal/
  arduino/

examples/
  BasicRead/
  RegisterDump/
  HalBasedRead/

docs/
  hal-usage-guide.md
  testing-without-hardware.md
  portability-roadmap.md

tests/
  host/

## Roadmap

The current public API remains Arduino-compatible. Portability work is tracked in `docs/portability-roadmap.md`.
