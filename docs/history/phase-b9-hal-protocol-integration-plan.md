# Phase B9.0 - HAL-Backed Protocol Integration Plan

This phase defines how `ADS1299_Protocol` should be integrated into the optional HAL-backed `ADS1299Plus` path.

It does not change production routing.

## Goal

The goal of B9 is to let the optional HAL-backed `ADS1299Plus` path use the host-tested `ADS1299_Protocol` object for ADS1299 command, register, and frame byte sequencing.

The classic Arduino-compatible path must remain simple and stable:

```cpp
ADS1299_SafeSPI adsSpi(PIN_CS);
ADS1299Plus ads(adsSpi, adsPins);
```

Arduino users should not need to know that `ADS1299_Protocol` exists.

## Current State

After B8:

- `ADS1299Core` owns pure portable helper logic.
- `ADS1299_Protocol` owns ADS1299 command, register, and frame protocol sequencing through `ADS1299_HAL`.
- `ADS1299Plus` still owns all production routing.
- The optional HAL constructor currently works through HAL-backed `ADS1299_SafeSPI`.
- The classic `ADS1299_SafeSPI` constructor path remains the default user-facing path.

## Integration Constraints

B9 must preserve:

- public `ADS1299Plus` API;
- `BasicRead`, `RegisterDump`, and `HalBasedRead` examples;
- validated register defaults;
- SPI mode, bit order, and clock configuration;
- command byte order;
- RDATAC/RDATA byte counts;
- frame decode behavior;
- Arduino/SafeSPI compatibility path;
- no external dependencies;
- no heap allocation unless a later review explicitly proves it is necessary.

## Important Design Issue: RDATAC State

Both `ADS1299Plus` and `ADS1299_Protocol` currently track RDATAC state.

This must not become two independent sources of truth.

B9 should choose one effective owner for HAL-backed protocol state. The recommended direction is:

- keep `ADS1299Plus::rdatacActive_` for public compatibility and classic-path behavior;
- let `ADS1299_Protocol` own HAL-backed protocol guards internally;
- keep the two synchronized only in the HAL-backed routing methods;
- avoid exposing protocol state to users.

During integration, tests must verify that `ADS1299Plus::isRDATACActive()` continues to report the same public behavior.

## Recommended Storage Model

Avoid dynamic allocation.

The preferred integration is to make `ADS1299_Protocol` embeddable in `ADS1299Plus` without requiring a HAL reference at construction time for the classic path.

Recommended preparatory change:

```cpp
class ADS1299_Protocol {
public:
  ADS1299_Protocol();
  explicit ADS1299_Protocol(ADS1299_HAL& hal);
  void attach(ADS1299_HAL& hal);
  bool attached() const;
  ...
};
```

Then `ADS1299Plus` can hold:

```cpp
ADS1299_Protocol protocol_;
```

Classic constructor:

```cpp
protocol_(); // detached, unused
```

HAL constructor:

```cpp
protocol_(hal); // attached, used only when useHal_ is true
```

If constructor initialization becomes awkward, the HAL constructor can default-construct `protocol_` and call `protocol_.attach(hal)`.

Protocol methods should fail safely if called while detached, but production code should avoid calling them unless `useHal_` is true.

## Recommended Phase Breakdown

### B9.1 - Protocol Attachability

Make `ADS1299_Protocol` default-constructible and attachable to an `ADS1299_HAL`.

Scope:

- add detached construction support;
- add `attach(ADS1299_HAL&)`;
- add `attached() const`;
- guard protocol methods against null HAL use;
- add host protocol tests for detached rejection and attach behavior.

Do not touch `ADS1299Plus` yet.

Validation:

- core tests pass;
- protocol tests pass;
- full host tests pass;
- Arduino examples compile.

### B9.2 - Embed Protocol in ADS1299Plus Without Routing

Add a private `ADS1299_Protocol protocol_` member to `ADS1299Plus`.

Scope:

- include the protocol header in `ADS1299Plus.h`;
- initialize or attach `protocol_` only for the HAL-backed constructor;
- leave all command/register/frame methods routed exactly as before;
- add host coverage proving existing HAL-backed behavior still passes.

This phase is intentionally boring. It proves storage and construction are safe before behavior changes.

Validation:

- full host tests pass;
- protocol tests pass;
- Arduino examples compile.

### B9.3 - Route HAL-Backed Commands and Registers Through Protocol

Route only the optional HAL-backed path through `ADS1299_Protocol` for:

- `cmdWakeup()`;
- `cmdStandby()`;
- `cmdReset()`;
- `cmdStart()`;
- `cmdStop()`;
- `cmdRDATAC()`;
- `cmdSDATAC()`;
- `cmdRDATA()`;
- `writeOne_()`;
- `readOne_()`;
- `writeBurst_()`;
- `readBurst_()`.

Classic path must continue using existing `ADS1299_SafeSPI` code.

Public `rdatacActive_` must remain synchronized with protocol state after `cmdReset()`, `cmdRDATAC()`, and `cmdSDATAC()`.

Validation:

- compare command/register sequencing in existing host tests;
- add targeted HAL-backed tests if needed;
- verify register access is still rejected during RDATAC;
- Arduino examples compile.

### B9.4 - Route HAL-Backed Frame Acquisition Through Protocol

Route only the optional HAL-backed path through `ADS1299_Protocol` for:

- `readFrameRDATAC()`;
- `readDataOnDemand()`.

Classic path must continue using existing frame loops.

Validation:

- full host tests pass;
- protocol tests pass;
- RDATAC frame decode still matches known bytes;
- RDATA frame decode still matches known bytes;
- invalid sync behavior is unchanged;
- insufficient capacity rejects before SPI traffic;
- Arduino examples compile.

### B9.5 - HAL Integration Review

Review the result after routing commands, registers, and frames.

This should answer:

- Is public behavior still unchanged?
- Is the classic Arduino path still simple?
- Is the HAL path now meaningfully protocol-backed?
- Is state ownership clear enough?
- Are the tests strong enough for future backend work?
- Should any duplicate code remain temporarily for compatibility?

This phase should be review/documentation unless a small defect is found.

## Explicit Non-Goals for B9

B9 should not:

- remove `ADS1299_SafeSPI`;
- move `ADS1299Plus` out of `src/`;
- change public method names;
- change examples;
- change default register values;
- add STM32, ESP-IDF, Zephyr, or PlatformIO backend files;
- add a package manager or external test framework;
- change repository layout broadly.

## End Condition

B9 is complete when:

- the optional HAL-backed `ADS1299Plus` path uses `ADS1299_Protocol` for commands, registers, and frames;
- the classic Arduino/SafeSPI path remains working and unchanged from the user's perspective;
- host tests pass;
- protocol tests pass;
- Arduino example compile validation passes;
- documentation clearly explains that `ADS1299_Protocol` is internal.

## B9.0 Decision

Proceed with B9.1 first.

Do not integrate `ADS1299Plus` directly in B9.0.

The next implementation step should be:

```text
Phase B9.1 - Protocol attachability
```
