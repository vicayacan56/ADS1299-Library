# Portability Roadmap

The current version of ADS1299Plus is Arduino-compatible.

That means it currently depends on:

- `Arduino.h`
- `SPI.h`
- `pinMode()`
- `digitalWrite()`
- `digitalRead()`
- `delay()`
- `delayMicroseconds()`
- `SPI.transfer()`

## Future goal

The long-term goal is to separate the ADS1299 logic from the platform-specific code.

The future architecture should contain:

- A portable C++ core.
- A hardware abstraction layer, or HAL.
- An Arduino backend.
- Optional future backends for STM32 HAL, ESP-IDF, Zephyr or bare-metal C++.

## Proposed structure

```text
src/
  core/
    ADS1299Core.h
    ADS1299Core.cpp
    ADS1299_Registers.h
    ADS1299_Types.h

  hal/
    ADS1299_HAL.h

  arduino/
    ADS1299_ArduinoHAL.h
    ADS1299_ArduinoHAL.cpp
    ADS1299Plus.h

```
## Core

The core should contain only ADS1299 logic:

- SPI command sequences.
- Register read and write logic.
- Frame decoding.
- Channel count detection.
- Configuration helpers.

The core should not include `Arduino.h` or `SPI.h`.

## HAL

The HAL should define the operations needed by the core:

- SPI transfer.
- CS control.
- START control.
- RESET control.
- DRDY read.
- Microsecond delay.
- Millisecond delay.

## Arduino backend

The Arduino backend will implement the HAL using Arduino GPIO, Arduino SPI and Arduino delay functions.

## Possible future backends

- STM32 HAL.
- ESP-IDF.
- Zephyr.
- Bare-metal C++.

## Phase B1 - HAL skeleton

**Status:** In progress (conservative approach)

### What was added

- **`src/hal/ADS1299_HAL.h`**: Abstract base class defining the minimal interface for hardware operations (SPI, GPIO, delay).
- **`src/arduino/ADS1299_ArduinoHAL.h`**: Header for Arduino-specific implementation.
- **`src/arduino/ADS1299_ArduinoHAL.cpp`**: Arduino implementation using Arduino GPIO, Arduino SPI, and Arduino delay functions.

### Important notes for Phase B1

1. **No refactoring of ADS1299Plus yet**. The current `ADS1299Plus.h` and `ADS1299Plus.cpp` still use Arduino APIs directly (this is not a breaking change).
2. **HAL is available but not integrated**. The driver will continue working as-is for all existing projects. This is a passive skeleton.
3. **API remains compatible**. Examples (`BasicRead`, `RegisterDump`) continue to work without modification.
4. **Future integration**: Phase B2+ will gradually refactor `ADS1299Plus` to use the HAL interface, allowing multiple backends.

### Phase B1 HAL interface

The `ADS1299_HAL` abstract class defines:

- `begin()` / `end()`: Initialization and cleanup.
- `csLow()` / `csHigh()`: Chip select control.
- `spiTransfer(uint8_t)`: SPI communication.
- `delayMicroseconds(uint32_t)` / `delayMilliseconds(uint32_t)`: Timing.
- `setStart(bool)` / `setReset(bool)` / `setPwdn(bool)`: GPIO control signals.
- `readDrdy()`: DRDY input monitoring.

### Why conservative?

- No breaking changes to the current API.
- No mass refactoring of existing code.
- The HAL is ready for future use, but integration will be done carefully in later phases.
- Existing projects will continue to compile and function normally.

### Phase B1.1 - Refinements

Minor corrections and enhancements to the HAL skeleton:

- **Include path correction**: Changed `#include "ADS1299_HAL.h"` to `#include "../hal/ADS1299_HAL.h"` for proper relative path handling.
- **Optional PWDN support**: Added `PIN_UNUSED` constant to allow boards that don't have a PWDN pin. Constructor can now receive `PIN_UNUSED` for `pwdnPin` parameter; PWDN operations are skipped if the pin is not assigned.
- **RESET and DRDY alignment**: Corrected RESET to start HIGH (inactive, since RESET is active-low); changed DRDY to use `INPUT_PULLUP` for consistency with existing Arduino-compatible driver.
- **Modern C++ conventions**: Switched from `virtual` to `override` keyword in derived class methods for better compile-time safety.

No API changes. ADS1299Plus and examples remain fully compatible.
