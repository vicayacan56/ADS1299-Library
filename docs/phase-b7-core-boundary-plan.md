# Phase B7.0 - Portable Core Boundary Plan

This phase defines the boundary for the future portable ADS1299 core. It does not move production code yet.

The goal is to avoid architectural drift before extracting code from the current Arduino-compatible driver.

## Design Goal

The final repository should remain simple for Arduino users while also allowing non-Arduino environments to reuse the ADS1299 protocol logic.

The intended user experience remains:

1. Install the library.
2. Open `BasicRead` or `RegisterDump`.
3. Select a board in Arduino IDE or a compatible environment.
4. Compile and use the ADS1299 driver without learning the internal HAL architecture.

The portability goal is:

1. Keep ADS1299 command, register, frame, and configuration logic independent from Arduino APIs.
2. Keep platform-specific GPIO, SPI, and timing inside HAL/backends.
3. Preserve the existing Arduino API while the internal structure evolves.

## Current State

The repository is currently an Arduino-compatible library with an optional HAL-backed path.

This is not yet a pure portable core because:

- `ADS1299Plus.h` includes `Arduino.h`.
- `ADS1299Plus.h` includes `ADS1299_SafeSPI.h`.
- `ADS1299_SafeSPI.h` includes Arduino SPI types.
- The classic public path is still Arduino-first.

This is acceptable as a transitional state. B7 should make the next separation narrow and reversible.

## Boundary Decision

B7 should use the existing `ADS1299_HAL` as the long-term platform boundary.

Reasoning:

- It already models the hardware operations the ADS1299 driver needs.
- It avoids introducing another abstraction layer too early.
- It keeps the repository easier to understand.
- It can map to Arduino, STM32 HAL, ESP-IDF, Zephyr, or bare-metal code later.
- It keeps `ADS1299_SafeSPI` as a compatibility bridge rather than making it the final core dependency.

A separate transport interface should not be added yet. It can be reconsidered only if direct core-to-HAL use becomes awkward during extraction.

## Proposed Future Layers

```text
User sketches
  |
  v
ADS1299Plus public Arduino-compatible facade
  |
  v
Portable ADS1299 core logic
  |
  v
ADS1299_HAL interface
  |
  v
Platform backend, initially ADS1299_ArduinoHAL
```

`ADS1299_SafeSPI` remains available for backward compatibility and existing sketches.

## What Belongs In The Portable Core

The future core should contain logic that is ADS1299-specific but not platform-specific:

- Device ID validation.
- ADS1299-4, ADS1299-6, ADS1299 channel detection.
- Register command sequencing.
- Register range validation.
- RDATAC state tracking.
- RDATA and RDATAC frame assembly.
- STATUS decoding.
- 24-bit sample unpacking.
- Frame size calculation.
- Register default application order.
- Channel mask clipping.
- Configuration helper logic.

The core may call HAL methods for:

- SPI byte transfer.
- CS assert/deassert.
- START, RESET, PWDN control.
- DRDY read.
- Microsecond and millisecond delay.

## What Must Stay Outside The Portable Core

The portable core must not include:

- `Arduino.h`
- `SPI.h`
- `SPIClass`
- `SPISettings`
- `pinMode`
- `digitalWrite`
- `digitalRead`
- `delay`
- `delayMicroseconds`
- Arduino pin constants such as `SCK`, `MOSI`, or `MISO`

The Arduino facade/backend should continue owning Arduino-specific setup and user convenience.

## Public API Rule

B7 must preserve the public Arduino API:

- Existing `ADS1299Plus(ADS1299_SafeSPI&, Pins)` sketches must keep compiling.
- The optional `ADS1299Plus(ADS1299_HAL&, Pins, spiHz)` path must keep compiling.
- `BasicRead`, `RegisterDump`, and `HalBasedRead` must keep compiling.
- `readFrameRDATAC()`, `readDataOnDemand()`, `unpack24()`, register defaults, and SPI settings must not change behavior.

If a helper is moved into core, `ADS1299Plus` should keep exposing the same public wrapper or static helper when compatibility requires it.

## Recommended B7 Subphases

### B7.1 - Extract Pure Logic First

Move only platform-independent helper logic into a small core header/source pair.

Candidate logic:

- `channelsFromDeviceID()`
- `unpack24()`
- STATUS helper logic
- frame-size constants or helper functions
- register range validation
- channel mask clipping if practical

Rules:

- Do not move SPI command execution yet.
- Do not change acquisition behavior.
- Keep public `ADS1299Plus` helpers intact as wrappers if needed.
- Extend host tests before or during the extraction.

### B7.2 - Extract Register/Frame Protocol Behind HAL

After B7.1 is stable, move command sequencing and frame decode into a core class that talks to `ADS1299_HAL`.

Candidate logic:

- command methods
- register read/write
- register burst read/write
- RDATAC state
- RDATA/RDATAC frame reads

Rules:

- Preserve command byte ordering exactly.
- Preserve decode delays.
- Preserve RDATAC guards.
- Run host tests after each small movement.

### B7.3 - Keep Arduino Facade Simple

Once the core exists, make `ADS1299Plus` act as the Arduino-friendly facade.

Responsibilities:

- Keep the public API stable.
- Own Arduino-style pin configuration or delegate it to `ADS1299_ArduinoHAL`.
- Keep examples unchanged unless a new optional example is useful.
- Hide core/HAL complexity from ordinary users.

## Validation Required At Each B7 Step

Every B7 subphase should pass:

```powershell
g++ -std=c++11 -I tests/host/arduino_stubs -I src -I src/hal tests/host/test_ads1299_host.cpp src/ADS1299Plus.cpp src/ADS1299_SafeSPI.cpp -o tests/host/test_ads1299_host.exe
.\tests\host\test_ads1299_host.exe
```

Expected output:

```text
host tests passed
```

Manual Arduino IDE compile checks should still be used for:

- `examples/BasicRead`
- `examples/RegisterDump`
- `examples/HalBasedRead`

Future CI should add Arduino CLI compile validation, but that can remain separate from B7.1 if needed.

## B7.0 Decision

B7 should proceed in small, test-protected steps.

The next implementation step should be B7.1, extracting pure ADS1299 logic first. This avoids touching SPI transactions, acquisition timing, or public examples while proving that a portable core can start to exist without making the library harder to use.
