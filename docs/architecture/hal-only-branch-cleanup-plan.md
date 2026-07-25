# HAL-Only Branch Cleanup Plan

This document defines how `portable-core-hal` should evolve from a transitional two-path branch into a clean HAL-first branch.

The goal is not to change `main`. The goal is to make the portability branch understandable.

## Starting Point

Current `portable-core-hal` state:

```text
ADS1299Plus
  -> classic Arduino/SafeSPI path
  -> optional Arduino HAL-backed path
```

This state was useful for conservative validation:

- the original Arduino behavior was preserved;
- the HAL path was added without breaking examples;
- `HalBasedRead` proved the HAL-backed path on real UNO Q + ADS1299-4 hardware.

But this should not be the final shape of the portability branch.

## Desired End State

Recommended branch split:

```text
main
  -> stable Arduino/SafeSPI library
  -> public Arduino release
  -> RegisterDump and BasicRead as the primary examples

portable-core-hal
  -> HAL-first / HAL-only branch
  -> ADS1299 protocol runs through ADS1299_HAL
  -> ArduinoHAL is only one backend, not the architecture itself
```

In the final HAL branch, the classic `ADS1299_SafeSPI` route should not be the active internal path.

## What Must Stay In main

`main` should remain easy for Arduino users:

- `ADS1299Plus.h`
- `ADS1299Plus.cpp`
- `ADS1299_SafeSPI.h`
- `ADS1299_SafeSPI.cpp`
- `ADS1299_Registers.h`
- `examples/RegisterDump`
- `examples/BasicRead`
- user-facing Arduino documentation

This is the release-friendly Arduino library.

## What portable-core-hal Should Become

`portable-core-hal` should make the HAL path the normal path:

- `src/hal/ADS1299_HAL.h`
- `src/hal/ADS1299_HAL_Types.h`
- `src/core/ADS1299_Core.h`
- `src/core/ADS1299_Core.cpp`
- `src/core/ADS1299_Protocol.h`
- `src/core/ADS1299_Protocol.cpp`
- `src/arduino/ADS1299_ArduinoHAL.h`
- `src/arduino/ADS1299_ArduinoHAL.cpp`

The branch can still compile under Arduino, but Arduino should be represented as a backend, not as the central design.

## Main Design Decision

Before deleting or isolating `ADS1299_SafeSPI` from `portable-core-hal`, choose one of these API strategies.

### Option A - Keep ADS1299Plus As The HAL Facade

In this option, `ADS1299Plus` remains the public class in the portable branch, but the SafeSPI constructor and classic internals are removed or isolated.

Example:

```cpp
ADS1299_ArduinoHAL hal(PIN_CS, PIN_START, PIN_RESET, PIN_PWDN, PIN_DRDY);
ADS1299Plus ads(hal, adsPins);
```

Pros:

- familiar class name;
- less public API churn;
- existing HAL example already uses this shape.

Cons:

- `ADS1299Plus` still carries Arduino-library history in its name and design;
- removing the SafeSPI constructor would make this branch diverge from `main`;
- users may confuse the HAL branch with the stable Arduino release.

### Option B - Add A Portable Device Class

In this option, a new portable class becomes the HAL-first API.

Possible names:

```text
ADS1299_Device
ADS1299_Portable
ADS1299_HALDevice
```

Example:

```cpp
ADS1299_ArduinoHAL hal(PIN_CS, PIN_START, PIN_RESET, PIN_PWDN, PIN_DRDY);
ADS1299_Device ads(hal);
```

Pros:

- clean distinction from the Arduino release;
- easier to explain;
- future STM32/ESP-IDF/Zephyr backends can use the same class;
- less pressure to keep Arduino compatibility in the portable core.

Cons:

- more new API;
- requires careful migration of public helpers;
- needs new examples and docs.

### Recommendation

Use Option B for the long-term HAL-only branch.

Keep `ADS1299Plus` as the stable Arduino public class in `main`.

Create a new HAL-first portable class in `portable-core-hal` only after documenting exactly which API methods it should expose.

## Cleanup Sequence

### E3 - Define HAL-Only Public API

Create a small API plan for the future portable class.

Decide whether it must expose:

- `begin()`;
- `end()`;
- `configureDefaults()`;
- `readDeviceID()`;
- `channelCount()`;
- `bytesPerFrame()`;
- command helpers;
- register helpers;
- `readFrameRDATAC()`;
- `readDataOnDemand()`;
- lead-off helpers;
- channel configuration helpers.

Acceptance criteria:

- API list is explicit;
- no code moved yet;
- differences from `ADS1299Plus` are documented.

### E4 - Introduce Portable Device Class

Add the new HAL-only class beside the existing code.

Rules:

- no deletion yet;
- use `ADS1299_Protocol`;
- use `ADS1299_HAL`;
- do not include `Arduino.h`;
- do not include `ADS1299_SafeSPI.h`;
- keep tests passing.

Acceptance criteria:

- host tests compile;
- Arduino examples still compile;
- new HAL-only host tests compile;
- no classic path behavior changes.

### E5 - Add HAL-Only Examples

Add examples that represent the portable branch clearly.

Possible Arduino-backed examples:

```text
examples/HalRegisterDump
examples/HalBasicRead
```

These examples may still run on Arduino, but they should use only the HAL-backed API.

Acceptance criteria:

- examples compile with Arduino CLI;
- UNO Q can upload and run them;
- they replace the conceptual need for classic `RegisterDump` and `BasicRead` in the HAL branch.

### E6 - Isolate Or Remove SafeSPI From portable-core-hal

Only after E4 and E5 pass, isolate the classic path.

Possible approaches:

1. Move SafeSPI docs and examples out of the portable branch user path.
2. Mark SafeSPI as kept only for comparison.
3. Remove the SafeSPI constructor from the HAL-only facade.
4. Eventually remove `ADS1299_SafeSPI.*` from the HAL-only branch if `main` already preserves it.

Acceptance criteria:

- portable branch can be understood without SafeSPI;
- HAL examples replace classic examples;
- tests prove protocol behavior is still intact;
- `main` remains the stable SafeSPI release.

### E7 - First Native Backend Spike

After the HAL-only branch is clean, choose one non-Arduino backend.

Recommended candidates:

- STM32Cube HAL;
- ESP-IDF;
- Zephyr native.

Acceptance criteria:

- backend compiles in its native environment;
- no Arduino includes are required by the portable core;
- any missing HAL contract methods are identified explicitly.

## What Not To Do

Do not:

- delete SafeSPI from `main`;
- rewrite everything in one phase;
- add STM32/ESP-IDF/Zephyr before the HAL-only boundary is clean;
- claim universal IDE/microcontroller support before native backends exist;
- make ordinary Arduino users understand the HAL path.

## Success Definition

The cleanup is successful when:

```text
main is easy Arduino.
portable-core-hal is HAL-only.
SafeSPI is no longer part of the portable branch's main execution path.
ArduinoHAL is just one backend.
The ADS1299 protocol is not duplicated per platform.
```

That is the clean architecture target.
