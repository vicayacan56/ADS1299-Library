# Execution Paths and Release Strategy

This document explains the current ADS1299Plus structure in plain terms.

The repository currently contains one transitional library with two execution paths:

- the classic Arduino/SafeSPI path;
- the optional Arduino HAL-backed path.

This was intentional during validation, but it should not be the final shape of the `portable-core-hal` branch.

The desired split is:

```text
main
  -> simple Arduino/SafeSPI public library

portable-core-hal
  -> HAL-first / HAL-only portability branch
```

## The Simple Public Goal

The original public goal remains:

```text
An easy Arduino-compatible ADS1299 library with register helpers, configuration helpers, and acquisition examples.
```

For that goal, the recommended user path is still:

```cpp
ADS1299_SafeSPI adsSpi(PIN_CS);
ADS1299Plus ads(adsSpi, adsPins);
```

This is the path used by:

- `examples/RegisterDump`
- `examples/BasicRead`

## Why There Are Two Paths

The HAL work was added conservatively.

That means the existing Arduino/SafeSPI path was not removed or rewritten at first. Instead, a second path was added beside it so the portable architecture could be tested without breaking the stable public API.

The result is a transitional architecture:

```text
Current transitional state:
One branch with two internal execution paths.

Desired final split:
main keeps the classic path.
portable-core-hal keeps the HAL path.
```

## Path 1 - Classic Arduino/SafeSPI

```text
ADS1299Plus
  -> ADS1299_SafeSPI
  -> Arduino SPI
  -> ADS1299
```

Main files:

- `src/ADS1299Plus.h`
- `src/ADS1299Plus.cpp`
- `src/ADS1299_SafeSPI.h`
- `src/ADS1299_SafeSPI.cpp`
- `examples/RegisterDump`
- `examples/BasicRead`

Purpose:

- stable Arduino user path;
- simple public examples;
- compatibility with the original library direction;
- best path for a normal Arduino IDE user.

## Path 2 - Optional Arduino HAL

```text
ADS1299Plus
  -> ADS1299_Protocol
  -> ADS1299_ArduinoHAL
  -> Arduino SPI/GPIO/delay
  -> ADS1299
```

Main files:

- `src/hal/ADS1299_HAL.h`
- `src/hal/ADS1299_HAL_Types.h`
- `src/core/ADS1299_Protocol.h`
- `src/core/ADS1299_Protocol.cpp`
- `src/arduino/ADS1299_ArduinoHAL.h`
- `src/arduino/ADS1299_ArduinoHAL.cpp`
- `examples/HalBasedRead`

Purpose:

- validate the HAL contract;
- provide a working Arduino reference backend;
- prepare for future non-Arduino backends;
- test protocol execution through a platform-neutral interface.

## What Is Shared

Both paths share ADS1299 concepts:

- register definitions;
- channel count detection;
- 24-bit sample unpacking;
- frame decoding helpers;
- default register values.

Shared files include:

- `src/ADS1299_Registers.h`
- `src/core/ADS1299_Core.h`
- `src/core/ADS1299_Core.cpp`

## Why It Feels Duplicated

Some operations appear in both paths during this transition.

For example, register reads can happen through:

```text
ADS1299Plus -> ADS1299_SafeSPI
```

or:

```text
ADS1299Plus -> ADS1299_Protocol -> ADS1299_HAL
```

This is temporary duplication caused by conservative migration. It keeps the Arduino library stable while the HAL path is proven.

## What The HAL Has Proven

The HAL path has now passed:

- host-side tests with `FakeHAL`;
- Arduino CLI compilation;
- Arduino UNO Q hardware smoke test with ADS1299-4;
- `HalBasedRead` hardware execution with RDATAC frames.

This means `ADS1299_ArduinoHAL` is a valid reference backend for the current HAL contract.

It does not yet mean that STM32Cube, ESP-IDF, Zephyr-native, or bare-metal backends exist.

## Recommended Branch Strategy

The public release should stay simple, and the portable branch should become conceptually clean.

Recommended `main` focus:

```text
Arduino-compatible ADS1299Plus library.
Use RegisterDump first.
Use BasicRead for acquisition.
No HAL complexity in the main user story.
```

Recommended branch split:

| Branch | Purpose |
| --- | --- |
| `main` | Stable Arduino/SafeSPI public release |
| `portable-core-hal` | HAL-first portability development branch |

In other words:

```text
main should be simple.
portable-core-hal should not keep the classic route forever.
```

## What Should Be Published First

The first polished public release should emphasize:

- Arduino IDE compatibility;
- Arduino CLI compatibility;
- `RegisterDump`;
- `BasicRead`;
- ADS1299-4/6/8 support;
- hardware smoke test on Arduino UNO Q + ADS1299-4.

The HAL should be described carefully from `main`:

```text
HAL work has been validated on Arduino UNO Q in the portable-core-hal branch and is being developed toward future portable backends.
```

Do not claim:

```text
Works with every IDE and microcontroller.
```

until native backends and hardware validation exist for those environments.

## Future Direction

If HAL development continues, the next clean architecture should avoid keeping two full paths in `portable-core-hal`.

The long-term shape should be:

```text
main:
  ADS1299Plus
    -> Arduino-friendly SafeSPI facade

portable-core-hal:
  ADS1299 portable device/core
    -> HAL-only protocol engine
    -> ArduinoHAL / STM32CubeHAL / ESPIDFHAL / ZephyrHAL
```

That should be a future portability branch or major version, not a rushed change to the stable Arduino release.

## Decision

For clarity and maintainability:

- keep `main` focused on the simple Arduino library;
- evolve `portable-core-hal` toward a HAL-only branch;
- treat the current two-path state as transitional, not final;
- do not add more native HAL backends until the HAL-only branch boundary is clean;
- document the two paths explicitly so users and maintainers know why both exist today and why they should be separated.

## Practical Next Step

Before deleting the classic path from `portable-core-hal`, create a focused plan:

1. Define the HAL-only public API for the portable branch.
2. Decide whether `ADS1299Plus` remains the facade or a new portable class is introduced.
3. Convert `RegisterDump` and `BasicRead` equivalents to HAL-backed examples.
4. Keep `main` untouched as the Arduino/SafeSPI release.
5. Remove or isolate `ADS1299_SafeSPI` from the portable branch only after HAL-backed examples and tests fully replace it.
