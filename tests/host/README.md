# Host-side tests

These tests validate portable ADS1299 logic without an Arduino board or an ADS1299 device.

`test_ads1299_core.cpp` compiles the portable core. `test_ads1299_protocol.cpp` compiles the protocol object against a local HAL fake. `test_ads1299_device.cpp` validates the HAL-only device facade against a local HAL fake.

## What is covered

- Portable `ADS1299Core` helpers
- `ADS1299_Protocol` command/register/frame sequencing
- Portable raw frame decoding through `ADS1299Core::decodeFrame()`
- HAL-only `ADS1299_Device` facade
- STATUS helpers and 24-bit sample unpacking
- HAL-only `ADS1299_Device::begin()`
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
- `end()` shutdown sequencing through the HAL-only path

## Build and run

From the repository root, with `g++` in `PATH`:

```powershell
g++ -std=c++11 -I src tests/host/test_ads1299_core.cpp src/core/ADS1299_Core.cpp -o tests/host/test_ads1299_core.exe
.\tests\host\test_ads1299_core.exe

g++ -std=c++11 -I src -I src/hal tests/host/test_ads1299_protocol.cpp src/core/ADS1299_Protocol.cpp src/core/ADS1299_Core.cpp -o tests/host/test_ads1299_protocol.exe
.\tests\host\test_ads1299_protocol.exe

g++ -std=c++11 -I src -I src/hal tests/host/test_ads1299_device.cpp src/core/ADS1299_Device.cpp src/core/ADS1299_Protocol.cpp src/core/ADS1299_Core.cpp -o tests/host/test_ads1299_device.exe
.\tests\host\test_ads1299_device.exe
```

Expected output:

```text
core tests passed
protocol tests passed
device tests passed
```

These tests do not replace hardware validation. They are a fast regression check for portable logic, protocol sequencing, and the HAL-only device facade.
