# HAL Branch Status

This document describes the current state of the `portable-core-hal` branch.

It is intentionally short. Older phase notes were removed from this branch so the active documentation stays easy to follow.

## Branch Purpose

`portable-core-hal` is the HAL-first development branch.

Its goal is to keep ADS1299 register, command, and frame logic independent from any one microcontroller framework.

The current public path is:

```text
ADS1299_Device
  -> ADS1299_Protocol
  -> ADS1299_HAL
  -> platform backend
```

The first backend is:

```text
ADS1299_ArduinoHAL
```

## What This Branch Supports Now

- HAL-only public API through `ADS1299_Device`.
- Arduino backend through `ADS1299_ArduinoHAL`.
- ADS1299-4, ADS1299-6, and ADS1299 ID detection.
- Register reads and writes.
- Conservative default configuration.
- RDATAC continuous frame reads.
- RDATA on-demand frame reads.
- Host-side tests with fake HAL objects.
- Arduino CLI/IDE compilation of HAL examples.

## What This Branch Does Not Claim Yet

This branch does not yet provide native production backends for:

- STM32 HAL;
- ESP-IDF;
- Zephyr native APIs;
- bare-metal projects.

Those should be added one backend at a time.

## Current Examples

- `examples/HalRegisterDump`: first bring-up and register diagnostic.
- `examples/HalBasicRead`: basic RDATAC acquisition through the HAL path.

Use `HalRegisterDump` before trying acquisition.

## Current Validation

Expected local validation:

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

Arduino CLI compile checks:

```powershell
C:\Tools\ArduinoCLI\arduino-cli.exe compile --fqbn arduino:avr:uno examples\HalRegisterDump
C:\Tools\ArduinoCLI\arduino-cli.exe compile --fqbn arduino:avr:uno examples\HalBasicRead
C:\Tools\ArduinoCLI\arduino-cli.exe compile --fqbn arduino:zephyr:unoq examples\HalRegisterDump
C:\Tools\ArduinoCLI\arduino-cli.exe compile --fqbn arduino:zephyr:unoq examples\HalBasicRead
```

## Next Work

The next useful step is a first non-Arduino backend spike.

Recommended order:

1. Choose one target framework.
2. Add one backend under `src/<platform>/`.
3. Keep `src/core` unchanged.
4. Keep `src/hal` unchanged unless the backend exposes a genuine missing abstraction.
5. Add a minimal compile check for that backend.
6. Run hardware smoke tests if hardware is available.
7. Document exactly which board, SDK, pins, and commands were validated.

Start with [HAL Backend Porting Guide](hal-backend-porting-guide.md).

## Release Relationship

`main` should remain the simple Arduino/SafeSPI release branch.

`portable-core-hal` should stay focused on HAL portability and should not reintroduce the classic Arduino/SafeSPI route.
