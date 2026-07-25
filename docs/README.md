# ADS1299Plus Documentation

This folder is divided by audience.

If you only want to use the library from Arduino IDE, start with **User Documentation**.

## User Documentation

Start here:

- [User Guide](user/user-guide.md): installation, wiring, examples, acquisition flow, and common problems.
- [HAL Usage Guide](user/hal-usage-guide.md): classic Arduino path and optional Arduino HAL path.
- [Testing Without Hardware](user/testing-without-hardware.md): Arduino compile checks and host-side tests.
- [UNO Q EEG MIDI Notes](user/uno-q-eeg-midi.md): project-specific notes.

Recommended default Arduino usage:

```cpp
ADS1299_SafeSPI adsSpi(PIN_CS);
ADS1299Plus ads(adsSpi, adsPins);
```

## Architecture and Maintainer Notes

These documents explain the current internal design and release/readiness state:

- [Portability Roadmap](architecture/portability-roadmap.md)
- [Execution Paths and Release Strategy](architecture/execution-paths-and-release-strategy.md)
- [HAL-Only Branch Cleanup Plan](architecture/hal-only-branch-cleanup-plan.md)
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

ADS1299Plus is currently an Arduino-compatible library with:

- classic Arduino/SafeSPI usage as the default path;
- optional HAL-backed Arduino usage;
- portable helper logic in `src/core`;
- an internal protocol object for HAL-backed command/register/frame sequencing;
- host-side tests;
- GitHub Actions for host tests and Arduino example compilation.

It should not yet be described as a fully portable ADS1299 library for all embedded platforms. STM32, ESP-IDF, Zephyr, and bare-metal backends are future possibilities, not current supported backends.
