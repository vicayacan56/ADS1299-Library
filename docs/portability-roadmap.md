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











    