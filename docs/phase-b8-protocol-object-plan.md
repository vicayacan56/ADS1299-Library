# Phase B8.0 - Portable Protocol Object Plan

This phase defines the next internal boundary after B7. It does not change production code.

The goal is to avoid a broad refactor while preparing a portable object that can own ADS1299 command, register, and frame protocol execution through `ADS1299_HAL`.

## Why B8 Exists

B7 extracted deterministic helper logic into `ADS1299Core`, but `ADS1299Plus` still owns protocol execution:

- command dispatch;
- register read/write byte sequences;
- RDATAC/RDATA frame transfer loops;
- RDATAC state tracking;
- decode-delay placement.

Continuing to add free functions to `ADS1299Core` would make the core less coherent. The next useful boundary is an internal protocol object.

## Proposed Object

Working name:

```cpp
ADS1299_Protocol
```

Proposed future files:

```text
src/core/ADS1299_Protocol.h
src/core/ADS1299_Protocol.cpp
```

This object should be internal at first. It should not become the user-facing API.

## Boundary Decision

`ADS1299_Protocol` should talk directly to `ADS1299_HAL`.

Reasons:

- `ADS1299_HAL` already exposes CS, SPI transfer, timing, and control pins.
- Avoids introducing another transport interface before it is proven necessary.
- Keeps `ADS1299_SafeSPI` stable as the Arduino compatibility bridge.
- Keeps future STM32/ESP-IDF/Zephyr backends aligned around one HAL contract.

If direct HAL use becomes awkward, a smaller transport interface can be considered later. It should not be added preemptively.

## Non-Goals

B8 must not:

- Change public `ADS1299Plus` API.
- Change `BasicRead`, `RegisterDump`, or `HalBasedRead`.
- Change register defaults.
- Change SPI mode, bit order, or clock.
- Change RDATAC/RDATA byte counts.
- Change command byte order.
- Remove `ADS1299_SafeSPI`.
- Move the whole repository structure.
- Add dependencies.

## Initial Responsibilities

The future protocol object may own:

- `rdatacActive_` state.
- Command dispatch for `WAKEUP`, `STANDBY`, `RESET`, `START`, `STOP`, `RDATAC`, `SDATAC`, and `RDATA`.
- Single-register read/write sequencing.
- Burst register read/write sequencing.
- RDATA/RDATAC frame transfer loops.
- Calls to `ADS1299Core::decodeFrame()`.
- Decode delays after commands/register operations.

It should not own:

- Arduino pin definitions.
- Arduino constructors.
- `ADS1299_SafeSPI` compatibility behavior.
- User-facing examples.
- Policy-level defaults such as which channels to configure in `configureDefaults()` until that is explicitly moved later.

## Proposed Minimal Interface

The first version should be deliberately small:

```cpp
class ADS1299_Protocol {
public:
  explicit ADS1299_Protocol(ADS1299_HAL& hal);

  void cmdWakeup();
  void cmdStandby();
  void cmdReset();
  void cmdStart();
  void cmdStop();
  void cmdRDATAC();
  void cmdSDATAC();
  void cmdRDATA();

  bool writeReg(uint8_t addr, uint8_t value);
  bool readReg(uint8_t addr, uint8_t& value);
  bool writeRegs(uint8_t startAddr, const uint8_t* data, size_t n);
  bool readRegs(uint8_t startAddr, uint8_t* data, size_t n);

  bool readFrameRDATAC(uint8_t channelCount,
                       uint32_t& status24,
                       int32_t* channels,
                       size_t capacity);

  bool readDataOnDemand(uint8_t channelCount,
                        uint32_t& status24,
                        int32_t* channels,
                        size_t capacity);

  bool isRDATACActive() const;
};
```

This is a planning sketch, not a final API promise.

## Extraction Order

### B8.1 - Protocol Object Skeleton

Add `ADS1299_Protocol` with no integration into `ADS1299Plus`.

It should compile standalone with host tests and use `FakeHAL` directly.

Validation:

- Core-only tests still pass.
- Full host tests still pass.
- Arduino example CI still passes.

### B8.2 - Move Command Dispatch Into Protocol Tests

Implement command methods in `ADS1299_Protocol` and test emitted bytes through `FakeHAL`.

Do not wire `ADS1299Plus` to it yet.

Tests must verify:

- exact command byte;
- CS low/high around command;
- decode delay after commands that require it;
- RDATAC state changes for `RESET`, `RDATAC`, and `SDATAC`.

### B8.3 - Move Register Sequencing Into Protocol Tests

Implement single and burst register read/write in `ADS1299_Protocol`.

Do not wire `ADS1299Plus` to it yet.

Tests must verify:

- exact RREG/WREG command bytes;
- `n - 1` count byte;
- write payload order;
- NOP read count;
- rejection while RDATAC is active;
- range rejection.

### B8.4 - Move Frame Transfer Into Protocol Tests

Implement RDATA/RDATAC transfer loops in `ADS1299_Protocol`.

Tests must verify:

- NOP byte count equals detected frame size;
- invalid sync returns false after decoding status;
- capacity rejection happens before SPI traffic;
- `readDataOnDemand()` is rejected while RDATAC is active;
- `readFrameRDATAC()` is rejected unless RDATAC is active.

### B8.5 - Integrate HAL-Backed ADS1299Plus Path

Only after protocol tests are strong, consider using `ADS1299_Protocol` inside the HAL-backed `ADS1299Plus` constructor path.

The classic `ADS1299_SafeSPI` path should remain untouched until HAL-backed behavior has proven equivalent.

## Required Test Guards

Before any B8 integration into `ADS1299Plus`, tests must cover:

- all command bytes;
- register read/write sequences;
- register access blocked during RDATAC;
- RDATA/RDATAC frame decode;
- invalid STATUS sync;
- insufficient capacity;
- `end()` behavior;
- variant frame sizes for ADS1299-4/6/8.

Existing tests already cover many of these through `ADS1299Plus`; B8 should add protocol-level tests before changing production routing.

## Public User Experience Rule

Arduino users should not need to know `ADS1299_Protocol` exists.

The first-screen usage should remain:

```cpp
ADS1299_SafeSPI adsSpi(PIN_CS);
ADS1299Plus ads(adsSpi, adsPins);
```

Optional HAL usage may remain documented for advanced users, but examples should not be made more complex.

## B8.0 Decision

B8 should begin with an unintegrated, host-tested protocol object.

The first implementation step should be:

```text
Phase B8.1 - Add unintegrated ADS1299_Protocol skeleton and tests
```

No `ADS1299Plus` routing should change until the protocol object has independent tests for command and register byte sequences.
