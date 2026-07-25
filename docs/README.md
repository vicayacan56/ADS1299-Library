# ADS1299Plus Documentation

This folder is divided by audience.

If you only want to use the library from Arduino IDE or Arduino CLI, start with **User Documentation**.

## User Documentation

- [User Guide](user/user-guide.md): installation, wiring, examples, acquisition flow, and common problems.
- [HAL Usage Guide](user/hal-usage-guide.md): how the HAL-only public API is used with the Arduino backend.
- [Testing Without Hardware](user/testing-without-hardware.md): Arduino compile checks and host-side tests.
- [UNO Q EEG MIDI Notes](user/uno-q-eeg-midi.md): project-specific notes.

Recommended usage on this branch:

```cpp
#include <ADS1299_Device.h>
#include <arduino/ADS1299_ArduinoHAL.h>

ADS1299_ArduinoHAL adsHal(PIN_CS, PIN_START, PIN_RESET, PIN_PWDN, PIN_DRDY);
ADS1299_Device ads(adsHal);
```

## Architecture and Maintainer Notes

These documents explain the current internal design and release/readiness state:

- [Portability Roadmap](architecture/portability-roadmap.md)
- [Execution Paths and Release Strategy](architecture/execution-paths-and-release-strategy.md)
- [HAL-Only Branch Cleanup Plan](architecture/hal-only-branch-cleanup-plan.md)
- [HAL-Only Public API](architecture/hal-only-public-api.md)
- [Final Usability Audit](architecture/final-usability-audit.md)
- [UNO Q Hardware Smoke Test](architecture/phase-c7-hardware-smoke-test.md)
- [Path B Closure Review](architecture/phase-b10-path-b-closure-review.md)
- [HAL Integration Review](architecture/phase-b9-hal-integration-review.md)
- [Protocol Boundary Review](architecture/phase-b8-protocol-boundary-review.md)
- [Current Architecture Review](architecture/b7-current-architecture-review.md)
- [Critical Review of Path B](architecture/b6-critical-review.md)

## Historical Design Notes

These files explain why earlier decisions were made. They are not required reading for normal use.

- [HAL Design References](history/hal-design-references.md)
- [Phase B2 Integration Plan](history/phase-b2-integration-plan.md)
- [Phase B7 Core Boundary Plan](history/phase-b7-core-boundary-plan.md)
- [Phase B8 Protocol Object Plan](history/phase-b8-protocol-object-plan.md)
- [Phase B9 HAL Protocol Integration Plan](history/phase-b9-hal-protocol-integration-plan.md)

## Current Project Status

This branch is HAL-only at the public library surface:

- `ADS1299_Device` is the public device facade.
- `ADS1299_HAL` is the backend contract.
- `ADS1299_ArduinoHAL` is the first validated backend.
- Portable logic lives in `src/core`.
- Arduino examples compile through the HAL-only path.
- Host-side tests validate core logic, protocol sequencing, and device behavior.

The branch should not yet be described as having production-ready STM32, ESP-IDF, Zephyr, or bare-metal backends. Those are future backend implementations.
