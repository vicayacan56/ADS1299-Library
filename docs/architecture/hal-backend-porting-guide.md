# HAL Backend Porting Guide

This guide explains how to add a new platform backend to the HAL-only branch without duplicating ADS1299 protocol logic.

The goal is simple:

```text
ADS1299_Device
  -> ADS1299_Protocol
  -> ADS1299_HAL
  -> your backend
```

Only the last layer should change between Arduino, STM32 HAL, ESP-IDF, Zephyr, or bare-metal projects.

## Current Reference Backend

The first validated backend is:

```text
src/arduino/ADS1299_ArduinoHAL.h
src/arduino/ADS1299_ArduinoHAL.cpp
```

Use it as the behavioral reference, not as a file to copy blindly.

The Arduino backend translates neutral HAL operations to:

- Arduino SPI setup and transactions;
- Arduino GPIO mode/read/write calls;
- Arduino millisecond and microsecond delays.

## Backend Contract

A backend must implement `ADS1299_HAL`:

```cpp
class MyBackend : public ADS1299_HAL {
public:
  void begin() override;
  void end() override;
  void beginTransaction(const ADS1299_SpiConfig& config) override;
  void endTransaction() override;
  void csLow() override;
  void csHigh() override;
  uint8_t spiTransfer(uint8_t data) override;
  void delayMicroseconds(uint32_t us) override;
  void delayMilliseconds(uint32_t ms) override;
  void setStart(bool high) override;
  void setReset(bool high) override;
  void setPwdn(bool high) override;
  bool readDrdy() override;
};
```

The backend owns platform resources. `ADS1299_Device` owns ADS1299 behavior.

## Required SPI Behavior

The ADS1299 device path expects:

- SPI mode `ADS1299_SpiMode::MODE1` by default.
- MSB-first byte order by default.
- Clock frequency from `ADS1299_SpiConfig::clockHz`.
- Blocking byte transfers.
- CS low during each command/register/frame transfer.
- CS high when each transfer group completes.

The backend must convert `ADS1299_SpiConfig` to native platform settings.

Do not hard-code Arduino symbols such as `SPI_MODE1`, `MSBFIRST`, `LOW`, or `HIGH` outside Arduino-specific files.

## Required GPIO Behavior

The backend must provide:

- CS output.
- START output.
- RESET output.
- optional PWDN output.
- DRDY input.

`readDrdy()` returns the electrical level of DRDY:

```text
true  -> DRDY is high, no new frame
false -> DRDY is low, data ready
```

`ADS1299_Device::dataReady()` converts that to the user-facing ready state.

## Delay Behavior

The backend must provide delays long enough for ADS1299 command timing.

For the first backend spike on a new platform, prefer simple blocking delays. Non-blocking or RTOS-aware timing can be added later only after the blocking backend is validated.

## Suggested File Layout

Keep new backends isolated:

```text
src/<platform>/ADS1299_<Platform>HAL.h
src/<platform>/ADS1299_<Platform>HAL.cpp
```

Examples:

```text
src/stm32/ADS1299_STM32HAL.h
src/espidf/ADS1299_EspIdfHAL.h
src/zephyr/ADS1299_ZephyrHAL.h
```

Do not put platform-specific headers in `src/core` or `src/hal`.

## Minimal Bring-Up Sequence

For a new backend:

1. Implement the backend class.
2. Compile a minimal sketch or app that constructs `ADS1299_Device`.
3. Run a register-dump style test first.
4. Confirm ADS1299 ID readback.
5. Confirm channel count detection.
6. Confirm `configureDefaults()`.
7. Start RDATAC only after register reads/writes are working.
8. Compare frame STATUS sync against the Arduino reference behavior.

## Host Test Expectations

Host tests should remain platform-neutral.

They should keep testing:

- `ADS1299Core` helper logic;
- `ADS1299_Protocol` command sequencing with fake HAL objects;
- `ADS1299_Device` behavior with fake HAL objects.

Avoid making host tests depend on Arduino, STM32, ESP-IDF, Zephyr, or any board SDK.

## Backend Acceptance Checklist

A backend is ready for early use when:

- it compiles in its native toolchain;
- it does not require changes to `ADS1299_Device`;
- it does not require changes to `ADS1299_Protocol`;
- it converts `ADS1299_SpiConfig` correctly;
- it keeps CS behavior correct around transfers;
- it can read a valid ADS1299 ID on hardware;
- it can dump registers after `configureDefaults()`;
- it can read stable RDATAC frames;
- it documents wiring assumptions and tested board/SDK versions.

## What Not To Do

Do not:

- duplicate ADS1299 command/register/frame logic in a backend;
- add platform includes to `src/core`;
- change validated ADS1299 register defaults as part of a backend;
- change frame decoding as part of a backend;
- claim broad platform support from compile-only checks.

Port one backend at a time and keep every phase small enough to review.
