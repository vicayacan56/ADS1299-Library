# Host-side tests

These tests validate portable ADS1299Plus logic without an Arduino board or an ADS1299 device.

They use minimal local stubs for `Arduino.h` and `SPI.h`, plus a `FakeHAL` that records SPI/GPIO/timing calls and returns queued SPI bytes.

## What is covered

- Portable `ADS1299Core` helpers
- `ADS1299Plus` compatibility wrappers for pure helpers
- STATUS helpers and 24-bit sample unpacking
- HAL-backed `ADS1299Plus::begin()`
- Register write command sequencing
- RDATAC frame decode with known bytes
- `readDataOnDemand()` frame decode
- Register access blocking while RDATAC is active
- `configureDefaults()` command sequencing
- ADS1299-4, ADS1299-6, and ADS1299 frame sizes
- invalid device ID rejection
- invalid STATUS sync rejection
- insufficient frame capacity rejection
- `readDataOnDemand()` blocking while RDATAC is active
- `end()` shutdown sequencing through the HAL path

## Build and run

From the repository root, with `g++` in `PATH`:

```powershell
g++ -std=c++11 -I tests/host/arduino_stubs -I src -I src/hal tests/host/test_ads1299_host.cpp src/core/ADS1299_Core.cpp src/ADS1299Plus.cpp src/ADS1299_SafeSPI.cpp -o tests/host/test_ads1299_host.exe
.\tests\host\test_ads1299_host.exe
```

Expected output:

```text
host tests passed
```

These tests do not replace hardware validation. They are a fast regression check for portable logic and HAL sequencing.
