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

## Phase B2.1 - SPI config and transactions

**Status:** In progress (HAL extension only)

### What was added

- **`src/hal/ADS1299_HAL_Types.h`**: Platform-neutral SPI and GPIO types.
- **`ADS1299_SpiConfig`**: Neutral SPI transaction configuration containing clock frequency, bit order, and SPI mode, with ADS1299-oriented defaults (`2048000`, `MSB_FIRST`, `MODE1`).
- **`beginTransaction(const ADS1299_SpiConfig&)` / `endTransaction()`**: New HAL methods for explicit SPI transaction boundaries.
- **Arduino backend transaction support**: `ADS1299_ArduinoHAL` now translates neutral SPI configuration to `SPISettings` and calls `SPI.beginTransaction()` / `SPI.endTransaction()`.

### Important notes for Phase B2.1

1. **ADS1299Plus does not use these HAL transactions yet**. The current driver still uses `ADS1299Plus` and `ADS1299_SafeSPI` exactly as before.
2. **No acquisition behavior changes**. Frame reading, register values, SPI timing, and examples remain untouched.
3. **This is still passive infrastructure**. The new transaction API prepares the HAL for future core integration without changing the public Arduino-compatible API.

## Phase B2.2 - SafeSPI optional HAL path

**Status:** In progress (additive transport path)

### What was added

- **`ADS1299_SafeSPI(ADS1299_HAL& hal, uint32_t spiHz)`**: Optional constructor for HAL-backed SPI transport.
- **Dual-path transport methods**: `begin()`, `end()`, `select()`, `deselect()`, `xfer()`, and `waitDecode()` now dispatch to either the existing Arduino SPI path or the optional HAL path.
- **Preserved Arduino path**: Existing sketches that construct `ADS1299_SafeSPI` with a CS pin and `SPIClass` continue to use the same Arduino APIs and SPI settings.
- **Arduino HAL SPI lifecycle**: `ADS1299_ArduinoHAL::begin()` / `end()` now call `SPI.begin()` / `SPI.end()` so the HAL path owns the same SPI lifecycle as the existing SafeSPI Arduino path.

### Important notes for Phase B2.2

1. **ADS1299Plus still uses the same public constructor and acquisition code**. No ADS1299Plus integration has been done yet.
2. **The HAL path is opt-in** through the new SafeSPI constructor.
3. **Validated SPI behavior is preserved**: MSB-first, MODE1, and the caller-provided SPI clock remain the transport settings.
4. **Examples remain unchanged** and continue to exercise the Arduino path.

## Phase B2.3 - ADS1299Plus optional HAL constructor

**Status:** In progress (additive driver path)

### What was added

- **`ADS1299Plus(ADS1299_HAL& hal, const Pins& pins, uint32_t spiHz)`**: Optional constructor for HAL-backed use.
- **Internal HAL-backed SafeSPI transport**: The HAL constructor creates an internal `ADS1299_SafeSPI` transport using the B2.2 HAL path.
- **Dual-path GPIO and timing helpers**: START, RESET, PWDN, DRDY, microsecond delays, millisecond delays, and decode delays now dispatch to Arduino APIs or HAL APIs depending on the constructor used.
- **Preserved Arduino path**: Existing code that constructs `ADS1299Plus(ADS1299_SafeSPI&, Pins)` continues to use the same public API and examples.

### Important notes for Phase B2.3

1. **Frame acquisition logic remains unchanged**. `readFrameRDATAC()`, `readDataOnDemand()`, `unpack24()`, frame sizing, and STATUS decoding are preserved.
2. **Register defaults remain unchanged**. B2.3 does not modify validated register values or configuration helpers.
3. **HAL usage is opt-in** through the new constructor. Existing examples remain unchanged and continue to exercise the Arduino/SafeSPI path.
4. **The Arduino-compatible API remains backward compatible**.

## Phase B2.4 - Example compile validation

**Status:** Complete

### Validation results

- **`examples/BasicRead/BasicRead.ino`**: Compiles successfully for Arduino Uno without hardware attached.
- **`examples/RegisterDump/RegisterDump.ino`**: Compiles successfully for Arduino Uno without hardware attached.
- Existing examples remain unchanged and continue to exercise the Arduino/SafeSPI path.
- The HAL type naming was adjusted to avoid Arduino `LOW` / `HIGH` macro collisions.

### Important notes for Phase B2.4

1. **Compilation does not require a connected board**. Arduino IDE Verify/Compile validates preprocessing, compilation, and linking.
2. **Runtime behavior still requires hardware validation**. Device ID reads, register dumps, and RDATAC frame stability need a physical ADS1299 setup.
3. **B2.4 validates backward compatibility** for the existing public examples.

## Phase B3 - Optional HAL-based Arduino example

**Status:** In progress (new opt-in example)

### What was added

- **`examples/HalBasedRead/HalBasedRead.ino`**: Example sketch that constructs `ADS1299_ArduinoHAL` and passes it to the new `ADS1299Plus(ADS1299_HAL&, Pins, spiHz)` constructor.

### Important notes for Phase B3

1. **The original examples remain unchanged**.
2. **HAL usage remains opt-in**. Users can keep using `ADS1299_SafeSPI` directly or choose the HAL-backed constructor.
3. **Hardware behavior should be compared against BasicRead** once an ADS1299 board is available.

## Phase B4.0 - Host-side validation scaffolding

**Status:** In progress (no hardware required)

### What was added

- **`tests/host/arduino_stubs/`**: Minimal `Arduino.h` and `SPI.h` stubs for desktop compilation.
- **`tests/host/test_ads1299_host.cpp`**: A lightweight test runner with a `FakeHAL`.
- **`tests/host/README.md`**: Build and run instructions for host-side tests.

### Initial host-side coverage

- Pure helpers: `channelsFromDeviceID()`, `unpack24()`, and STATUS decoding.
- HAL-backed `ADS1299Plus::begin()` path using a queued fake device ID.
- Register write command sequencing through the HAL path.
- RDATAC frame decoding from known bytes without an ADS1299 device.

### Important notes for Phase B4.0

1. **No external test framework is required**.
2. **These tests do not require Arduino IDE, a board, or an ADS1299 device**.
3. **Hardware validation remains required** for real SPI electrical behavior, timing margins, and long-running acquisition.

## Phase B4.1 - Expanded host-side coverage

**Status:** In progress (no hardware required)

### What was added

- `readDataOnDemand()` frame decode coverage with known bytes.
- Register-access guard checks while RDATAC is active.
- `configureDefaults()` command sequencing checks for key validated registers and channel setup.
- ADS1299-4, ADS1299-6, and ADS1299 variant detection plus frame-size checks.

### Important notes for Phase B4.1

1. **The tests still use only local stubs and FakeHAL**.
2. **No production source files or examples are changed by this phase**.
3. **The checks focus on regression protection for the HAL integration path**.

## Phase B4.2 - Negative host-side coverage

**Status:** In progress (no hardware required)

### What was added

- Invalid ADS1299 device ID rejection tests.
- Invalid STATUS sync rejection tests for RDATAC frames.
- Insufficient frame capacity rejection tests.
- `readDataOnDemand()` guard tests while RDATAC is active.
- `end()` sequencing checks to confirm STOP, SDATAC, HAL transaction release, and HAL shutdown.

### Important notes for Phase B4.2

1. **These tests protect failure paths and lifecycle behavior**.
2. **No production source files or examples are changed by this phase**.
3. **Hardware validation is still needed for electrical and timing behavior**.

## Phase B4.3 - Continuous host-side validation

**Status:** In progress (GitHub Actions)

### What was added

- **`.github/workflows/host-tests.yml`**: GitHub Actions workflow that builds and runs the host-side tests on Ubuntu.

### What the workflow checks

- `g++` can compile the host-side test runner with the Arduino/SPI stubs.
- The FakeHAL-based regression suite exits successfully.
- The check runs automatically on pushes to `main` and `portable-core-hal`, and on pull requests.

### Important notes for Phase B4.3

1. **No hardware is required in CI**.
2. **The workflow does not build Arduino sketches**; it only validates host-side regression tests.
3. **This gives an automated safety net before larger core extraction work**.

## Phase B5 - Documentation for HAL usage and validation

**Status:** In progress (documentation only)

### What was added

- **`docs/hal-usage-guide.md`**: Explains the classic Arduino/SafeSPI path and the optional Arduino HAL path.
- **`docs/testing-without-hardware.md`**: Explains Arduino compile validation, host-side tests, CI, and remaining hardware validation needs.
- **`README.md` updates**: Adds the HAL path, host-side test command, and links to the detailed documentation.

### Important notes for Phase B5

1. **No production source files are changed**.
2. **No examples are changed**.
3. **The documentation keeps the classic Arduino path as the default recommendation while exposing the HAL path as opt-in**.

## Phase B6 - Critical review of Path B

**Status:** Complete (review only)

### What was added

- **`docs/b6-critical-review.md`**: Critical review of the Path B work completed so far, including architecture, HAL direction, reference usage, validation coverage, user simplicity, and B7 readiness.

### Review outcome

Path B is coherent and remains aligned with the long-term goal:

- Keep the public Arduino library simple and intuitive.
- Preserve the classic `ADS1299Plus` / `ADS1299_SafeSPI` path.
- Keep HAL usage optional while it matures.
- Avoid broad refactors before the portable core boundary is clear.
- Treat the current HAL integration as a transitional bridge, not yet as a fully extracted portable core.

### Important notes for Phase B6

1. **No production source files are changed**.
2. **No examples are changed**.
3. **No test behavior is changed**.
4. **The review recommends small metadata and CI improvements before or alongside B7**.
5. **B7 should be narrow, test-driven, and focused on the core/HAL boundary rather than a large file move**.

## Phase B7.0 - Portable core boundary plan

**Status:** Complete (planning only)

### What was added

- **`docs/phase-b7-core-boundary-plan.md`**: Defines the intended boundary between the public Arduino facade, future portable ADS1299 core logic, `ADS1299_HAL`, and platform backends.

### Planning outcome

B7 should use the existing `ADS1299_HAL` as the long-term platform boundary and avoid adding another transport abstraction unless direct core-to-HAL extraction proves awkward.

The recommended next step is B7.1:

- Extract pure, platform-independent ADS1299 helper logic first.
- Keep the public `ADS1299Plus` API stable.
- Do not move SPI command execution yet.
- Do not change examples.
- Extend or preserve host tests during each movement.

### Important notes for Phase B7.0

1. **No production source files are changed**.
2. **No examples are changed**.
3. **No test behavior is changed**.
4. **This phase exists to prevent B7 from becoming a broad refactor**.

## Phase B7.1 - Pure helper core extraction

**Status:** Complete (narrow core extraction)

### What was added

- **`src/core/ADS1299_Core.h`** and **`src/core/ADS1299_Core.cpp`**: Portable helper layer with no Arduino dependencies.

### What moved behind the core boundary

- Device ID to channel count decoding.
- Frame byte count calculation.
- Register range validation.
- Channel mask clipping.
- STATUS decoding helpers.
- 24-bit signed sample unpacking.

### Compatibility notes

- `ADS1299Plus` keeps the same public constants and helper names.
- Existing sketches can continue calling `ADS1299Plus::channelsFromDeviceID()`, `ADS1299Plus::unpack24()`, and STATUS helpers.
- SPI command execution, register access, RDATAC acquisition, examples, and validated register defaults are unchanged.

### Validation notes

- Host tests now exercise the new `ADS1299Core` helpers directly and confirm the `ADS1299Plus` compatibility wrappers.
- The host test build command now links `src/core/ADS1299_Core.cpp`.

## Phase B7.2 - Portable frame decode extraction

**Status:** Complete (narrow acquisition helper extraction)

### What was added

- **`ADS1299Core::decodeFrame()`**: Decodes a raw ADS1299 frame buffer into STATUS and signed channel samples without Arduino or SPI dependencies.

### What changed

- `ADS1299Plus::readFrameRDATAC()` and `ADS1299Plus::readDataOnDemand()` still perform the same SPI reads, with the same CS/NOP byte flow, but delegate raw frame parsing to `ADS1299Core::decodeFrame()`.

### Compatibility notes

- SPI command execution is unchanged.
- RDATAC/RDATA acquisition flow is unchanged.
- Public `ADS1299Plus` API is unchanged.
- Examples are unchanged.

### Validation notes

- Host tests now exercise `ADS1299Core::decodeFrame()` directly, including valid sync, invalid sync, sample sign extension, and capacity rejection.

## Phase B7.3 - Portable register and channel helpers

**Status:** Complete (narrow protocol helper extraction)

### What was added

- `ADS1299Core::isValidChannel()`
- `ADS1299Core::channelRegisterAddress()`
- `ADS1299Core::readRegisterCommand()`
- `ADS1299Core::writeRegisterCommand()`

### What changed

- `ADS1299Plus` now delegates channel validation, channel register addressing, and RREG/WREG opcode construction to the portable core.

### Compatibility notes

- SPI command execution remains in `ADS1299Plus`.
- Register read/write byte order is unchanged.
- RDATAC/RDATA acquisition flow is unchanged.
- Public API and examples are unchanged.

### Validation notes

- Host tests cover the new register/channel helpers directly.
- Existing register sequencing tests still verify the emitted SPI byte sequences.

## Phase B7.4 - Portable configuration byte helpers

**Status:** Complete (narrow configuration helper extraction)

### What was added

- Portable `ADS1299Core::with...()` helpers for deterministic register-byte mutations used by configuration methods.

### What changed

- `ADS1299Plus` still reads and writes registers through the same public methods, but delegates byte updates for CONFIG1, CONFIG3, CONFIG4, CHnSET, and MISC1 to the portable core.

### Compatibility notes

- Register read/write sequencing is unchanged.
- SPI traffic and acquisition behavior are unchanged.
- Public API and examples are unchanged.
- Validated default register values are unchanged.

### Validation notes

- Host tests cover the new configuration-byte helpers directly.
- Existing host tests still verify register command sequencing and `configureDefaults()` behavior.
