# ADS1299Plus Documentation

This folder contains user documentation, validation notes, architecture notes, and historical design records.

If you only want to use the library from Arduino IDE, start with the short list below and ignore the phase/history documents.

## Start Here

- [Main README](../README.md): project overview, supported devices, examples, and quick usage.
- [HAL Usage Guide](hal-usage-guide.md): classic Arduino path and optional Arduino HAL path.
- [Testing Without Hardware](testing-without-hardware.md): how to compile examples and run host-side checks.

## User-Facing Documents

These are the useful documents for most users:

- [HAL Usage Guide](hal-usage-guide.md)
- [Testing Without Hardware](testing-without-hardware.md)
- [UNO Q EEG MIDI Notes](uno-q-eeg-midi.md)

The recommended default path for Arduino users remains:

```cpp
ADS1299_SafeSPI adsSpi(PIN_CS);
ADS1299Plus ads(adsSpi, adsPins);
```

The HAL path is optional and intended for advanced users or future backend work.

## Maintainer Documents

These documents explain the current architecture and validation direction:

- [Portability Roadmap](portability-roadmap.md)
- [Path B Closure Review](phase-b10-path-b-closure-review.md)
- [HAL Integration Review](phase-b9-hal-integration-review.md)
- [Protocol Boundary Review](phase-b8-protocol-boundary-review.md)
- [Current Architecture Review](b7-current-architecture-review.md)
- [Critical Review of Path B](b6-critical-review.md)

## Historical Design Notes

These files are useful for understanding why decisions were made. They are not required reading for normal library use.

- [HAL Design References](hal-design-references.md)
- [Phase B2 Integration Plan](phase-b2-integration-plan.md)
- [Phase B7 Core Boundary Plan](phase-b7-core-boundary-plan.md)
- [Phase B8 Protocol Object Plan](phase-b8-protocol-object-plan.md)
- [Phase B9 HAL Protocol Integration Plan](phase-b9-hal-protocol-integration-plan.md)

## Current Project Status

ADS1299Plus is currently an Arduino-compatible library with:

- classic Arduino/SafeSPI usage as the default path;
- optional HAL-backed Arduino usage;
- portable helper logic in `src/core`;
- an internal protocol object for HAL-backed command/register/frame sequencing;
- host-side tests;
- GitHub Actions for host tests and Arduino example compilation.

It should not yet be described as a fully portable ADS1299 library for all embedded platforms. STM32, ESP-IDF, Zephyr, and bare-metal backends are future possibilities, not current supported backends.

## Documentation Cleanup Direction

The documentation is being simplified in small steps:

1. Keep the README simple for users.
2. Keep this index as the navigation hub.
3. Keep architecture and historical notes available for maintainers.
4. Avoid moving many files at once until links are reviewed.
