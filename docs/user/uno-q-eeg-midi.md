# Arduino UNO Q EEG/MIDI Application

ADS1299Plus on this branch is a HAL-first Arduino-compatible library for the ADS1299-x family.

The Arduino UNO Q EEG/MIDI project is an advanced application built on top of this library.

That application may use board-specific features such as:

- Arduino_RouterBridge.
- Monitor.
- Serial1.
- MCU/MPU communication.
- MIDI output.
- EEG streaming.
- Digital filters.
- Benchmarking tools.

These features are not required by the ADS1299Plus core library.

For generic usage, see:

- `examples/HalRegisterDump`
- `examples/HalBasicRead`

## Purpose of this document

This document clarifies that the UNO Q EEG/MIDI application is a real-world validation use case, but the library itself is not specific to Arduino UNO Q.
