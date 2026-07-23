# Testing Without Hardware

You can validate several parts of ADS1299Plus without an ADS1299 board.

## Arduino sketch compilation

Arduino IDE `Verify/Compile` does not require a connected board.

Use this flow on Windows:

1. Close Arduino IDE.
2. Make sure the library is installed in your Arduino sketchbook libraries folder.
3. The usual Windows location is:

   ```text
   C:\Users\<YourUser>\Documents\Arduino\libraries\ADS1299Plus
   ```

4. The installed library folder must contain files such as:

   ```text
   library.properties
   src\ADS1299Plus.h
   examples\BasicRead\BasicRead.ino
   ```

5. If you are working directly from this repository, either copy the repository folder there or create a directory link named `ADS1299Plus`.
6. Restart Arduino IDE after installing or updating the library.
7. Select a board from `Tools > Board`, for example `Arduino Uno`.
8. You do not need to select a port when only compiling without hardware.
9. Open examples from `File > Examples > ADS1299Plus`, not by opening a loose `.ino` before the library is installed.
10. Click `Verify` / `Compile`.

Compile these examples:

- `examples/BasicRead`
- `examples/RegisterDump`
- `examples/HalBasedRead`

This validates preprocessing, compilation, linking, public headers, and Arduino API compatibility.

If Arduino IDE reports:

```text
fatal error: ADS1299Plus.h: No such file or directory
```

then the library is not installed where Arduino IDE expects it, or Arduino IDE was not restarted after installation.

If Arduino IDE finds an older copy of the library, remove or replace that old copy and restart Arduino IDE again.

## Host-side tests

The repository includes host-side tests in `tests/host`.

They use local stubs for `Arduino.h` and `SPI.h`, plus a `FakeHAL`, so they can run on a desktop with `g++`.

From the repository root:

```powershell
g++ -std=c++11 -I src tests/host/test_ads1299_core.cpp src/core/ADS1299_Core.cpp -o tests/host/test_ads1299_core.exe
.\tests\host\test_ads1299_core.exe

g++ -std=c++11 -I src -I src/hal tests/host/test_ads1299_protocol.cpp src/core/ADS1299_Protocol.cpp src/core/ADS1299_Core.cpp -o tests/host/test_ads1299_protocol.exe
.\tests\host\test_ads1299_protocol.exe

g++ -std=c++11 -I tests/host/arduino_stubs -I src -I src/hal tests/host/test_ads1299_host.cpp src/core/ADS1299_Core.cpp src/core/ADS1299_Protocol.cpp src/ADS1299Plus.cpp src/ADS1299_SafeSPI.cpp -o tests/host/test_ads1299_host.exe
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
