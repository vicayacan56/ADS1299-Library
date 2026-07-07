# Phase B7.7 - Current Architecture Review

This document closes the current B7 sequence and records the architecture after the portable helper extractions.

It is a review and decision document, not an implementation step.

## Current Shape

The repository is now an Arduino-compatible ADS1299 library with:

- A stable public `ADS1299Plus` facade.
- The original `ADS1299_SafeSPI` Arduino transport path.
- An optional `ADS1299_ArduinoHAL` path.
- A small platform-independent `ADS1299Core` helper layer.
- Host tests for the full driver path.
- Standalone host tests for the portable core.
- GitHub Actions checks for host tests and Arduino example compilation.

This is still not a fully extracted portable driver core. That is intentional. B7 deliberately moved only deterministic logic that could be tested without touching SPI timing, acquisition sequencing, or public examples.

## What Is Portable Now

`src/core/ADS1299_Core.*` is platform-independent and does not depend on Arduino, SPI, stubs, HAL objects, or board pins.

It currently owns:

- Device ID to channel-count decoding.
- Frame size calculation.
- Register range validation.
- Channel validation.
- Channel register address calculation.
- Channel mask clipping.
- RREG/WREG opcode construction.
- CONFIG/CHnSET/MISC1 byte mutation helpers.
- STATUS decoding.
- 24-bit signed sample unpacking.
- Raw frame buffer decoding.

This is enough to prove that the ADS1299 protocol logic can start moving out of the Arduino facade safely.

## What Still Belongs To ADS1299Plus

`ADS1299Plus` still owns the user-facing Arduino-compatible behavior:

- Public constructors.
- Pin lifecycle and startup sequencing.
- Public command methods.
- Register read/write execution.
- RDATAC/RDATA SPI byte transfer loops.
- `configureDefaults()` flow.
- High-level configuration methods.
- Public compatibility wrappers for helpers now implemented in `ADS1299Core`.

This keeps existing sketches simple and avoids forcing normal Arduino users to understand the internal core/HAL split.

## What Still Belongs To SafeSPI

`ADS1299_SafeSPI` remains the compatibility transport bridge:

- Classic Arduino SPI path.
- Optional HAL-backed SPI path.
- CS select/deselect abstraction.
- Byte transfer abstraction.
- Transaction lifecycle.

It should remain stable while future work decides whether register and frame protocol execution moves directly into a HAL-backed core class.

## Validation Status

The current validation surface is much stronger than before B7:

- `tests/host/test_ads1299_core.cpp` compiles the portable core without Arduino/SPI stubs.
- `tests/host/test_ads1299_host.cpp` validates the `ADS1299Plus` HAL path with FakeHAL.
- `.github/workflows/host-tests.yml` runs both host test binaries.
- `.github/workflows/arduino-examples.yml` compiles `BasicRead`, `RegisterDump`, and `HalBasedRead` with Arduino CLI.

Hardware validation is still required for electrical behavior, ADS1299 timing margins, real register reads, and sustained RDATAC acquisition.

## B7 Result

B7 succeeded as a narrow extraction phase:

- Public Arduino API preserved.
- Examples preserved.
- Validated register defaults preserved.
- SPI mode and command byte order preserved.
- RDATAC/RDATA acquisition behavior preserved.
- Portable core introduced without adding external dependencies.
- CI coverage expanded.

The main architectural risk now is adding too many small free functions to `ADS1299Core` without defining the next object boundary.

## Recommended Next Phase

The next implementation phase should not be another broad extraction.

Recommended next step:

1. Define a small portable protocol object that owns register/frame protocol state but talks to `ADS1299_HAL`.
2. Keep `ADS1299Plus` as the Arduino-facing facade.
3. Move only one behavior at a time, starting with register read/write sequencing or command dispatch.
4. Keep `ADS1299_SafeSPI` stable for existing sketches.
5. Preserve both host tests and Arduino example compile CI at every step.

Do not move files into a new public structure yet. The safer path is to introduce the next internal boundary first, prove it with host tests, and only then consider layout cleanup.

## Decision

Path B remains coherent and should continue, but the repository should now pause before deeper protocol extraction.

The next phase should be planned as a new boundary step rather than a continuation of helper extraction. A good name would be:

```text
Phase B8 - Portable protocol object behind ADS1299Plus
```

The first B8 subphase should be design-only, similar to B7.0, with explicit rules for what moves, what remains public, and which tests must fail if byte ordering or acquisition behavior changes.
