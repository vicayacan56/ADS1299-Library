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

- **`docs/user/hal-usage-guide.md`**: Explains the classic Arduino/SafeSPI path and the optional Arduino HAL path.
- **`docs/user/testing-without-hardware.md`**: Explains Arduino compile validation, host-side tests, CI, and remaining hardware validation needs.
- **`README.md` updates**: Adds the HAL path, host-side test command, and links to the detailed documentation.

### Important notes for Phase B5

1. **No production source files are changed**.
2. **No examples are changed**.
3. **The documentation keeps the classic Arduino path as the default recommendation while exposing the HAL path as opt-in**.

## Phase B6 - Critical review of Path B

**Status:** Complete (review only)

### What was added

- **`docs/architecture/b6-critical-review.md`**: Critical review of the Path B work completed so far, including architecture, HAL direction, reference usage, validation coverage, user simplicity, and B7 readiness.

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

- **`docs/history/phase-b7-core-boundary-plan.md`**: Defines the intended boundary between the public Arduino facade, future portable ADS1299 core logic, `ADS1299_HAL`, and platform backends.

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

## Phase B7.5 - Standalone portable core validation

**Status:** Complete (validation only)

### What was added

- **`tests/host/test_ads1299_core.cpp`**: Standalone host test for `src/core/ADS1299_Core.*`.

### What changed

- GitHub Actions now builds and runs a core-only test without Arduino stubs before the full host-side ADS1299Plus/FakeHAL test.
- Testing documentation now includes both the standalone core test command and the full host test command.

### Compatibility notes

- Production source behavior is unchanged.
- Public API and examples are unchanged.
- The Arduino-compatible host test remains in place.

### Validation notes

- The core-only test proves `src/core` can compile with `g++` using only `-I src`, without `Arduino.h`, `SPI.h`, or local Arduino stubs.

## Phase B7.6 - Arduino example compile CI

**Status:** Complete (CI validation only)

### What was added

- **`.github/workflows/arduino-examples.yml`**: GitHub Actions workflow that installs Arduino CLI, installs the Arduino AVR core, and compiles `BasicRead`, `RegisterDump`, and `HalBasedRead` for `arduino:avr:uno`.

### What changed

- CI now checks both host-side portable logic and Arduino example compilation.

### Compatibility notes

- No production source files are changed.
- No examples are changed.
- No PlatformIO configuration is introduced.

### Validation notes

- The Arduino workflow validates preprocessing, public includes, linking, and Arduino compatibility without requiring connected hardware.

## Phase B7.7 - Current architecture review

**Status:** Complete (review only)

### What was added

- **`docs/architecture/b7-current-architecture-review.md`**: Review of the architecture after B7 helper extraction and CI expansion.

### Review outcome

B7 succeeded as a narrow extraction phase:

- Public Arduino API preserved.
- Examples preserved.
- SPI command execution and acquisition behavior preserved.
- Portable helper core introduced without platform dependencies.
- Core-only and full host tests available.
- Arduino example compile CI available.

### Recommendation

The next phase should pause before deeper extraction and define the next internal boundary first.

Recommended direction:

- Keep `ADS1299Plus` as the Arduino-facing facade.
- Keep `ADS1299_SafeSPI` stable for compatibility.
- Plan a small portable protocol object that talks to `ADS1299_HAL`.
- Move register/frame protocol execution only after that boundary is explicit and test-protected.

## Phase B8.0 - Portable protocol object plan

**Status:** Complete (planning only)

### What was added

- **`docs/history/phase-b8-protocol-object-plan.md`**: Design plan for an internal `ADS1299_Protocol` object behind `ADS1299Plus`.

### Planning outcome

B8 should not continue adding free helper functions to `ADS1299Core`.

The next internal boundary should be an unintegrated, host-tested protocol object that talks directly to `ADS1299_HAL`.

### Recommended next step

Start with:

```text
Phase B8.1 - Add unintegrated ADS1299_Protocol skeleton and tests
```

Rules:

- Do not change public `ADS1299Plus` API.
- Do not change examples.
- Do not change `ADS1299_SafeSPI`.
- Do not route production behavior through the new object until command/register protocol tests exist.
- Preserve host tests and Arduino example compile CI.

## Phase B8.1 - Unintegrated protocol object skeleton

**Status:** Complete (skeleton and tests only)

### What was added

- **`src/core/ADS1299_Protocol.h`** and **`src/core/ADS1299_Protocol.cpp`**: Internal portable protocol object skeleton.
- **`tests/host/test_ads1299_protocol.cpp`**: Host test that constructs the protocol object against a local HAL fake.

### What changed

- GitHub Actions now builds and runs the protocol skeleton test between the standalone core test and the full host-side ADS1299Plus test.

### Compatibility notes

- `ADS1299Plus` is not integrated with `ADS1299_Protocol` yet.
- `ADS1299_SafeSPI` is unchanged.
- Public API and examples are unchanged.
- No command, register, or acquisition behavior is routed through the new object yet.

### Validation notes

- The protocol skeleton test proves the new object can compile and be constructed without Arduino/SPI stubs.

## Phase B8.2 - Protocol command dispatch

**Status:** Complete (unintegrated protocol behavior)

### What was added

- Command methods on `ADS1299_Protocol` for `WAKEUP`, `STANDBY`, `RESET`, `START`, `STOP`, `RDATAC`, `SDATAC`, and `RDATA`.

### What changed

- The unintegrated protocol object can now emit ADS1299 command bytes through `ADS1299_HAL`.
- `RDATAC`, `SDATAC`, and `RESET` update protocol RDATAC state.

### Compatibility notes

- `ADS1299Plus` is not integrated with `ADS1299_Protocol` yet.
- `ADS1299_SafeSPI` is unchanged.
- Public API and examples are unchanged.
- Register access and acquisition are unchanged.

### Validation notes

- Protocol tests verify exact command bytes, CS low/high ordering, decode delays, `RDATA` without decode delay, and RDATAC state transitions.

## Phase B8.3 - Protocol register access

**Status:** Complete (unintegrated protocol behavior)

### What was added

- Register methods on `ADS1299_Protocol`: `writeReg()`, `readReg()`, `writeRegs()`, and `readRegs()`.

### What changed

- The unintegrated protocol object can now emit single and burst RREG/WREG sequences through `ADS1299_HAL`.

### Compatibility notes

- `ADS1299Plus` is not integrated with `ADS1299_Protocol` yet.
- `ADS1299_SafeSPI` is unchanged.
- Public API and examples are unchanged.
- Acquisition behavior is unchanged.

### Validation notes

- Protocol tests verify exact RREG/WREG command bytes, `n - 1` count bytes, write payload order, NOP read count, returned read values, invalid range rejection, null pointer rejection, and register access blocking while RDATAC is active.

## Phase B8.4 - Protocol frame transfer

**Status:** Complete (unintegrated protocol behavior)

### What was added

- `ADS1299_Protocol::readFrameRDATAC()`
- `ADS1299_Protocol::readDataOnDemand()`

### What changed

- The unintegrated protocol object can now transfer raw ADS1299 frames through `ADS1299_HAL` and decode them through the portable `ADS1299Core::decodeFrame()` helper.
- `readFrameRDATAC()` only runs while protocol RDATAC state is active.
- `readDataOnDemand()` emits `RDATA` first, then reads one frame with ADS1299 `NOP` transfers.

### Compatibility notes

- `ADS1299Plus` is not integrated with `ADS1299_Protocol` yet.
- `ADS1299_SafeSPI` is unchanged.
- Public API and examples are unchanged.
- Existing acquisition behavior remains routed through the current driver path.

### Validation notes

- Protocol tests verify frame transfer for ADS1299-4, ADS1299-6, and ADS1299-8 frame sizes.
- Tests verify exact `NOP` transfer counts, CS ordering, STATUS decoding, signed 24-bit sample decoding, invalid sync rejection, insufficient capacity rejection, invalid channel-count rejection, null pointer rejection, and RDATAC/RDATA state guards.

## Phase B8.5 - Protocol boundary review

**Status:** Complete (review only)

### What was added

- **`docs/architecture/phase-b8-protocol-boundary-review.md`**: Review of the internal `ADS1299_Protocol` boundary after B8.1 through B8.4.

### Review outcome

B8 is complete as an unintegrated protocol-object phase.

`ADS1299_Protocol` now has host-tested coverage for command dispatch, RDATAC state tracking, register access, burst register access, RDATAC frame reads, on-demand RDATA frame reads, invalid sync handling, buffer guards, and ADS1299-4/6/8 frame sizes.

### Integration decision

Production routing should not change in B8.5.

The next phase should be:

```text
Phase B9.0 - HAL-backed protocol integration plan
```

B9.0 should decide how the optional HAL-backed `ADS1299Plus` path will use `ADS1299_Protocol` while keeping the classic `ADS1299_SafeSPI` path stable.

## Phase B9.0 - HAL-backed protocol integration plan

**Status:** Complete (planning only)

### What was added

- **`docs/history/phase-b9-hal-protocol-integration-plan.md`**: Integration plan for routing the optional HAL-backed `ADS1299Plus` path through `ADS1299_Protocol`.

### Planning outcome

B9 should integrate in small steps and avoid routing production behavior before the protocol object can be safely embedded or attached without heap allocation.

The recommended next phase is:

```text
Phase B9.1 - Protocol attachability
```

### Important decisions

1. The classic Arduino/SafeSPI path remains the default user-facing path.
2. `ADS1299_Protocol` stays internal.
3. The HAL-backed path should use `ADS1299_Protocol` gradually: first attachability, then embedding, then commands/registers, then frames.
4. RDATAC state must not become two unsynchronized sources of truth.
5. No examples, public API, validated register defaults, SPI settings, or acquisition behavior should change during planning.

## Phase B9.1 - Protocol attachability

**Status:** Complete (protocol preparation only)

### What was added

- `ADS1299_Protocol()` default constructor.
- `ADS1299_Protocol::attach(ADS1299_HAL& hal)`.
- `ADS1299_Protocol::attached() const`.
- Detached guards so protocol methods fail safely before a HAL is attached.

### What changed

`ADS1299_Protocol` can now be embedded in another object before a HAL is available.

This prepares for a future `ADS1299Plus` private `ADS1299_Protocol` member without requiring heap allocation or immediate production routing.

### Compatibility notes

- `ADS1299Plus` is not integrated with `ADS1299_Protocol` yet.
- `ADS1299_SafeSPI` is unchanged.
- Public API and examples are unchanged.
- Existing acquisition behavior remains routed through the current driver path.

### Validation notes

- Protocol tests verify detached rejection, no-op command safety before attach, attach behavior, RDATAC state reset on attach, and normal command dispatch after attach.

## Phase B9.2 - Embed protocol in ADS1299Plus without routing

**Status:** Complete (storage only)

### What was added

- `ADS1299Plus` now has a private `ADS1299_Protocol protocol_` member.
- The classic `ADS1299_SafeSPI` constructor leaves the protocol object detached.
- The optional HAL-backed constructor attaches the protocol object to the provided `ADS1299_HAL`.
- Host-side build commands now link `src/core/ADS1299_Protocol.cpp` because `ADS1299Plus` owns a protocol member.

### What did not change

- No public `ADS1299Plus` API changed.
- No command, register, or frame method is routed through `ADS1299_Protocol` yet.
- The classic Arduino/SafeSPI path remains the active production path for classic construction.
- The HAL-backed path still uses the existing HAL-backed `ADS1299_SafeSPI` routing.
- Examples are unchanged.

### Validation notes

- Existing host tests continue to validate current `ADS1299Plus` behavior after embedding the protocol object.
- B9.3 should be the first phase that changes HAL-backed command/register routing.

## Phase B9.3 - Route HAL-backed commands and registers through protocol

**Status:** Complete (HAL-backed routing only)

### What changed

- HAL-backed command methods now route through `ADS1299_Protocol`.
- HAL-backed single-register reads and writes now route through `ADS1299_Protocol`.
- HAL-backed burst-register reads and writes now route through `ADS1299_Protocol`.
- Public `ADS1299Plus::rdatacActive_` remains synchronized after `cmdReset()`, `cmdRDATAC()`, and `cmdSDATAC()`.

### What did not change

- The classic `ADS1299_SafeSPI` path still uses the existing command and register code.
- Frame acquisition is not routed through `ADS1299_Protocol` yet.
- Public API and examples are unchanged.
- Validated register defaults, SPI settings, command byte order, and decode delays are unchanged.

### Validation notes

- Existing host tests verify HAL-backed begin, command sequencing, register sequencing, configure-defaults sequencing, RDATAC register guards, and shutdown behavior after the routing change.
- B9.4 should route only HAL-backed frame acquisition through `ADS1299_Protocol`.

## Phase B9.4 - Route HAL-backed frame acquisition through protocol

**Status:** Complete (HAL-backed routing only)

### What changed

- HAL-backed `readFrameRDATAC()` now routes through `ADS1299_Protocol`.
- HAL-backed `readDataOnDemand()` now routes through `ADS1299_Protocol`.
- Frame transfer, STATUS decode, 24-bit sample decode, capacity guards, invalid-sync behavior, and ADS1299-4/6/8 frame sizes are now exercised through the protocol-backed HAL path.

### What did not change

- The classic `ADS1299_SafeSPI` path still uses the existing frame acquisition loops.
- Public API and examples are unchanged.
- Validated SPI settings, `NOP` frame clocking, RDATAC/RDATA byte counts, and `ADS1299Core::decodeFrame()` behavior are unchanged.

### Validation notes

- Existing host tests verify HAL-backed RDATAC frame decode, RDATA on-demand frame decode, variant frame sizes, invalid STATUS sync rejection, insufficient capacity rejection before SPI traffic, and RDATA blocking while RDATAC is active.
- B9.5 should review the completed HAL-backed protocol routing before any further cleanup.

## Phase B9.5 - HAL integration review

**Status:** Complete (review only)

### What was added

- **`docs/architecture/phase-b9-hal-integration-review.md`**: Critical review of the completed HAL-backed protocol routing.

### Review outcome

B9 achieved its goal: the optional HAL-backed `ADS1299Plus` path now uses `ADS1299_Protocol` for commands, registers, and frame acquisition, while the classic Arduino/SafeSPI path remains stable and user-facing.

The remaining duplication between classic-path code and protocol-backed HAL routing is intentional for now.

### Remaining risks

- Real ADS1299 hardware validation is still required for timing, DRDY behavior, and long-running acquisition.
- Arduino example compile validation should be checked through Arduino IDE or CI when local Arduino CLI is unavailable.
- Future cleanup should not remove `ADS1299_SafeSPI` or reorganize files until the full Path B result is reviewed.

### Recommended next phase

```text
Phase B10 - Path B closure review and release readiness
```

## Phase B10 - Path B closure review and release readiness

**Status:** Complete (review only)

### What was added

- **`docs/architecture/phase-b10-path-b-closure-review.md`**: Final Path B closure and release-readiness review.

### Review outcome

Path B is complete as an incremental portability milestone.

The repository remains an Arduino-compatible ADS1299 library, now with:

- portable helper core;
- neutral HAL;
- Arduino HAL backend;
- internal protocol object;
- protocol-backed optional HAL path;
- host-side tests;
- Arduino example compile CI.

### Release-readiness decision

The branch is ready for review and CI-based merge consideration if GitHub Actions are green.

It should not be described as a hardware-validated release until real ADS1299 smoke tests are completed.

### Recommended next steps

1. Push B10 documentation.
2. Check host and Arduino CI results on GitHub.
3. Run hardware smoke tests when an ADS1299 board is available.
4. Decide whether to merge `portable-core-hal` toward the stable branch or prepare a release candidate.

## Phase C1 - Documentation triage and public docs index

**Status:** Complete (documentation organization only)

### What was added

- **`docs/README.md`**: Documentation index separating user-facing docs, maintainer docs, and historical design notes.
- Main README link to the documentation index.

### Cleanup decision

C1 does not move or delete documents.

The first cleanup step is to make the existing documentation navigable before reorganizing paths. Future cleanup phases can simplify the README, create a user guide, and optionally move historical phase documents after links are reviewed.

## Phase C2 - Simplify public README

**Status:** Complete (public documentation cleanup only)

### What changed

- The root `README.md` was rewritten as a user-facing entry point.
- The README now focuses on installation, quick start, examples, optional HAL usage, validation, and documentation links.
- Historical phase details were kept out of the public README and remain available through `docs/README.md`.

### Cleanup decision

C2 does not change code, examples, tests, CI, or library metadata.

The README should stay focused on ordinary Arduino users. Maintainer notes, Path B history, HAL architecture details, and release-readiness reviews should stay in `docs/`.

## Phase C3 - User guide and docs subfolders

**Status:** Complete (documentation organization only)

### What changed

- `docs/user/` now contains user-facing documentation.
- `docs/architecture/` now contains roadmap and architecture/review documents.
- `docs/history/` now contains historical plans and design references.
- **`docs/user/user-guide.md`** was added as the main practical usage guide.
- README and docs index links were updated for the new structure.

### Cleanup decision

C3 keeps all documentation available while making the folder easier to navigate.

The historical phase documents remain in the repository, but they are no longer mixed with user-facing guides at the top level of `docs/`.

## Phase C4 - Final repository usability audit

**Status:** Complete (review only)

### What was added

- **`docs/architecture/final-usability-audit.md`**: Critical repository usability audit after Path B and documentation cleanup.

### Review outcome

The repository is now coherent and navigable for Arduino users.

The main user path is:

1. Read `README.md`.
2. Open `docs/user/user-guide.md`.
3. Compile and run `examples/RegisterDump`.
4. Move to `examples/BasicRead`.
5. Use `examples/HalBasedRead` only when validating the optional HAL-backed path.

### Remaining blockers before a polished release

- Real ADS1299 hardware smoke tests are still required.
- Some HAL source comments still describe earlier Phase B skeleton status and should be refreshed.
- Release metadata should remain conservative until hardware validation is complete.

### Recommended next phase

```text
Phase C5 - Source comment and metadata polish
```

## Phase C5 - Source comment and metadata polish

**Status:** Complete (comment and metadata review only)

### What changed

- Refreshed stale HAL comments in `src/hal/ADS1299_HAL.h`.
- Refreshed stale Arduino HAL comments in `src/arduino/ADS1299_ArduinoHAL.h`.
- Refreshed stale Arduino HAL comments in `src/arduino/ADS1299_ArduinoHAL.cpp`.
- Removed the accidental leading blank line in `src/arduino/ADS1299_ArduinoHAL.h`.
- Cleaned minor whitespace in touched comment-adjacent areas.

### Metadata decision

`library.properties` was reviewed and left unchanged.

The branch should not bump the public version until hardware smoke tests and a deliberate release decision are complete.

### Behavior decision

C5 does not change API, register values, SPI behavior, examples, tests, or acquisition behavior.

### Recommended next phase

```text
Phase C6 - Final validation pass
```

## Phase C6 - Final validation pass

**Status:** Complete (local host validation and Arduino IDE instructions)

### What changed

- Expanded `docs/user/testing-without-hardware.md` with step-by-step Arduino IDE verification instructions for compiling examples without a connected board.

### Local validation

Host-side tests were built and run locally with `g++`.

Expected passing output:

```text
core tests passed
protocol tests passed
host tests passed
```

### Arduino validation decision

`arduino-cli` was not available in the local shell used for this phase.

Arduino example compilation should therefore be completed through one of these paths:

- Arduino IDE `Verify/Compile` for `BasicRead`, `RegisterDump`, and `HalBasedRead`;
- GitHub Actions after push;
- a future local Arduino CLI installation.

### Recommended next phase

```text
Phase C7 - Hardware smoke test
```

## Phase C6.1 - Arduino CLI AVR compile fix

**Status:** Complete (compile fix)

### What changed

- Added the missing standard `<stddef.h>` include to `src/core/ADS1299_Protocol.h`.

### Reason

Arduino AVR compilation builds `ADS1299_Protocol.cpp` in a context where `size_t` was not declared by the existing includes.

Host-side tests had not exposed this because desktop builds received `size_t` through other include paths.

### Validation

Arduino CLI compilation now passes for:

- `examples/RegisterDump`
- `examples/BasicRead`
- `examples/HalBasedRead`

Host-side tests also pass:

```text
core tests passed
protocol tests passed
host tests passed
```

### Recommended next phase

```text
Phase C7 - Hardware smoke test
```
