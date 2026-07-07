# ADS1299Plus

ADS1299Plus is an Arduino-compatible C++ driver for the Texas Instruments ADS1299-x family of 24-bit biopotential analog-to-digital converters.

## Features

- SPI communication with the ADS1299-x.
- Register-level configuration.
- Dynamic channel detection for ADS1299-4, ADS1299-6 and ADS1299.
- EEG-oriented default configuration.
- Lead-off helper functions.
- Continuous data acquisition using RDATAC mode.

## Supported devices

- ADS1299-4
- ADS1299-6
- ADS1299

## Current status

This version is Arduino-compatible.

It is intended for boards and environments that provide:

- Arduino framework
- SPI library
- GPIO functions
- delay and delayMicroseconds functions

The library is not yet a fully platform-independent C++ driver. Future versions may separate the ADS1299 logic from the Arduino backend using a HAL interface.

## Examples

See:

- `examples/RegisterDump`
- `examples/BasicRead`

## PWDN pin

If the ADS1299 PWDN pin is connected directly to VDD, use:

```cpp
ADS1299Plus::ADS_PIN_UNUSED


If PWDN is connected to a microcontroller GPIO, pass the GPIO pin to the library.

Repository structure
src/
  ADS1299Plus.h
  ADS1299Plus.cpp
  ADS1299_SafeSPI.h
  ADS1299_SafeSPI.cpp
  ADS1299_Registers.h

examples/
  BasicRead/
  RegisterDump/

docs/
  portability-roadmap.md
  uno-q-eeg-midi.md
Roadmap

The current implementation depends on Arduino APIs.

A future version will separate the ADS1299 core driver from the platform layer using a HAL interface.