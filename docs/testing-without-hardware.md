# Testing Without Hardware

You can validate several parts of ADS1299Plus without an ADS1299 board.

## Arduino sketch compilation

Arduino IDE `Verify/Compile` does not require a connected board. Select a board such as Arduino Uno and compile:

- `examples/BasicRead`
- `examples/RegisterDump`
- `examples/HalBasedRead`

This validates preprocessing, compilation, linking, public headers, and Arduino API compatibility.

## Host-side tests

The repository includes host-side tests in `tests/host`.

They use local stubs for `Arduino.h` and `SPI.h`, plus a `FakeHAL`, so they can run on a desktop with `g++`.

From the repository root:

```powershell
g++ -std=c++11 -I src tests/host/test_ads1299_core.cpp src/core/ADS1299_Core.cpp -o tests/host/test_ads1299_core.exe
.\tests\host\test_ads1299_core.exe

g++ -std=c++11 -I src -I src/hal tests/host/test_ads1299_protocol.cpp src/core/ADS1299_Protocol.cpp -o tests/host/test_ads1299_protocol.exe
.\tests\host\test_ads1299_protocol.exe

g++ -std=c++11 -I tests/host/arduino_stubs -I src -I src/hal tests/host/test_ads1299_host.cpp src/core/ADS1299_Core.cpp src/ADS1299Plus.cpp src/ADS1299_SafeSPI.cpp -o tests/host/test_ads1299_host.exe
.\tests\host\test_ads1299_host.exe
```

Expected output:

```text
core tests passed
protocol tests passed
host tests passed
```

## What host tests cover

- Device ID to channel count decoding.
- 24-bit signed sample unpacking.
- STATUS helper decoding.
- Portable raw frame decoding through `ADS1299Core::decodeFrame()`.
- Standalone portable core compilation without Arduino/SPI stubs.
- Unintegrated protocol object compilation with a local HAL fake.
- HAL-backed startup sequence with fake SPI responses.
- Register command sequencing.
- RDATAC and RDATA frame decoding.
- Rejection of invalid IDs, invalid sync, insufficient capacity, and register access during RDATAC.
- Shutdown sequencing through the HAL path.

## GitHub Actions

The workflow `.github/workflows/host-tests.yml` runs the host-side tests automatically on:

- pushes to `main`;
- pushes to `portable-core-hal`;
- pull requests.

The workflow `.github/workflows/arduino-examples.yml` compiles the Arduino examples with Arduino CLI for `arduino:avr:uno`:

- pushes to `main`;
- pushes to `portable-core-hal`;
- pull requests.

Together, these checks catch portable logic regressions and Arduino example compilation regressions before hardware testing.

## What still needs hardware

Hardware is still required to validate:

- SPI electrical behavior and timing margins.
- Actual ADS1299 ID reads.
- Register dump correctness on a real device.
- RDATAC frame stability over time.
- Noise floor, lead-off behavior, bias drive, and channel wiring.
