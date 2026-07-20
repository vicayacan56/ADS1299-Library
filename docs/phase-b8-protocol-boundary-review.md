# Phase B8.5 - Protocol Boundary Review

This phase reviews the `ADS1299_Protocol` object after B8.1 through B8.4.

It does not integrate the protocol object into `ADS1299Plus`.

## Review Goal

The goal is to decide whether the internal protocol boundary is coherent enough to become the next integration target.

B8 introduced a portable object that talks to `ADS1299_HAL` and owns ADS1299 protocol byte sequencing. Before routing production behavior through it, the project needs a clear record of what is covered, what remains outside the object, and what must be protected during integration.

## Current Protocol Responsibilities

`ADS1299_Protocol` currently owns:

- command dispatch for `WAKEUP`, `STANDBY`, `RESET`, `START`, `STOP`, `RDATAC`, `SDATAC`, and `RDATA`;
- RDATAC active-state tracking;
- single-register `RREG` / `WREG` sequencing;
- burst-register `RREG` / `WREG` sequencing;
- register-access rejection while RDATAC is active;
- RDATAC frame transfer using ADS1299 `NOP` bytes;
- on-demand `RDATA` frame transfer;
- frame decode through `ADS1299Core::decodeFrame()`;
- decode-delay placement after commands and register operations that require it.

This is the intended boundary for low-level ADS1299 protocol execution.

## What Remains Outside the Protocol

The following responsibilities intentionally remain outside `ADS1299_Protocol`:

- public Arduino-facing API and constructors;
- pin ownership and board setup policy;
- `ADS1299_SafeSPI` compatibility behavior;
- Arduino examples;
- validated default register policy in `configureDefaults()`;
- hardware reset/start/power sequencing policy;
- user-facing documentation and examples;
- choice of whether the classic Arduino path or HAL path is used.

These boundaries are appropriate. `ADS1299_Protocol` should stay internal and should not become the primary user-facing API.

## Coverage Matrix

| Area | Protocol coverage | Notes |
| --- | --- | --- |
| Construction | Covered | Object can be built with `ADS1299_HAL` and no Arduino/SPI stubs. |
| Commands | Covered | Exact command bytes, CS ordering, decode delays, and RDATAC state transitions are tested. |
| Register writes | Covered | Single and burst WREG byte order and count bytes are tested. |
| Register reads | Covered | Single and burst RREG byte order, count bytes, NOP reads, and returned values are tested. |
| RDATAC guards | Covered | Register access is blocked while RDATAC is active. |
| Frame transfer | Covered | RDATAC and RDATA frame reads are tested through HAL transfer calls. |
| Variant frame sizes | Covered | ADS1299-4, ADS1299-6, and ADS1299-8 frame byte counts are tested. |
| Invalid frame sync | Covered | Decode returns false after STATUS is decoded. |
| Capacity guards | Covered | Insufficient buffers are rejected before SPI traffic. |
| Invalid channel count | Covered | Invalid channel counts are rejected before SPI traffic. |
| Null pointers | Covered | Null frame output pointers are rejected before SPI traffic. |
| Hardware timing margins | Not covered | Requires real ADS1299 hardware. |
| Electrical SPI behavior | Not covered | Requires real ADS1299 hardware and board wiring. |

## Reference Alignment

The implementation remains aligned with the reference work already captured in `docs/hal-design-references.md` and `docs/phase-b2-integration-plan.md`:

- the protocol uses neutral `ADS1299_HAL` operations instead of Arduino APIs;
- command and register operations preserve ADS1299 byte sequencing;
- frame transfers preserve ADS1299 `NOP`-clocked reads;
- decode logic stays in portable `ADS1299Core`;
- the Arduino compatibility layer remains simple and visible to users;
- no external dependencies or test frameworks were introduced.

The current work is coherent with the Path B goal: keep the repository approachable for Arduino users while making the low-level ADS1299 protocol portable behind the scenes.

## Integration Readiness

`ADS1299_Protocol` is ready to become the target for a carefully planned integration phase, but it should not be wired into `ADS1299Plus` in the same phase as this review.

Reasons:

- B8 already added a meaningful amount of internal behavior.
- The next step changes production routing, not just internal test coverage.
- Keeping integration in B9 gives a clean rollback point if behavior differs on hardware.
- The classic `ADS1299_SafeSPI` path must remain untouched until HAL-backed equivalence is proven.

## Recommended Next Phase

The next phase should be:

```text
Phase B9.0 - HAL-backed protocol integration plan
```

That phase should decide exactly how `ADS1299Plus` will use `ADS1299_Protocol` in the optional HAL-backed path.

B9.0 should be planning-only.

## Recommended Integration Order

After B9.0, integration should proceed in small steps:

1. Route HAL-backed command and register operations through `ADS1299_Protocol`.
2. Preserve all public method names and return behavior.
3. Keep the classic `ADS1299_SafeSPI` path untouched.
4. Add host tests that compare expected HAL-backed sequences.
5. Only then route HAL-backed frame acquisition through `ADS1299_Protocol`.
6. Compile Arduino examples after each production-routing change.

## B8.5 Decision

B8 is complete as an unintegrated protocol-object phase.

The project should move integration work to B9 rather than expanding B8 further.
