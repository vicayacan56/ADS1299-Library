# Phase B9.5 - HAL Integration Review

This phase reviews the result of B9.1 through B9.4.

It does not change production code.

## Review Goal

B9 set out to route the optional HAL-backed `ADS1299Plus` path through the host-tested `ADS1299_Protocol` object while preserving the classic Arduino/SafeSPI user experience.

This review checks whether that goal was met and what should happen next.

## Current Routing

The current routing is deliberately split:

| Area | Classic `ADS1299_SafeSPI` constructor | Optional HAL constructor |
| --- | --- | --- |
| GPIO setup and SPI transaction lifecycle | Existing Arduino/SafeSPI path | Existing HAL-backed `ADS1299_SafeSPI` path |
| Commands | Existing `ADS1299Plus` command code | `ADS1299_Protocol` |
| Register reads/writes | Existing `ADS1299Plus` register code | `ADS1299_Protocol` |
| RDATAC frame reads | Existing `ADS1299Plus` frame loop | `ADS1299_Protocol` |
| RDATA on-demand frame reads | Existing `ADS1299Plus` frame loop | `ADS1299_Protocol` |
| Public API | Unchanged | Unchanged |

This is the intended B9 result.

## What B9 Achieved

B9 achieved the main integration goal:

- `ADS1299_Protocol` can be embedded without heap allocation.
- The classic constructor leaves the protocol object detached and unused.
- The HAL constructor attaches the protocol object to the provided `ADS1299_HAL`.
- HAL-backed commands now route through the protocol object.
- HAL-backed single and burst register access now route through the protocol object.
- HAL-backed RDATAC and RDATA frame acquisition now route through the protocol object.
- Public `ADS1299Plus::isRDATACActive()` behavior is preserved by synchronizing `rdatacActive_` after `RESET`, `RDATAC`, and `SDATAC`.
- The classic Arduino/SafeSPI path remains simple and visible to users.

## Public Behavior

Public behavior remains intentionally unchanged:

- no public `ADS1299Plus` method names changed;
- no examples changed;
- no default register values changed;
- no SPI mode, bit order, or clock default changed;
- no frame size constants changed;
- no acquisition API changed;
- no external dependencies were introduced.

The user-facing Arduino path is still:

```cpp
ADS1299_SafeSPI adsSpi(PIN_CS);
ADS1299Plus ads(adsSpi, adsPins);
```

Advanced HAL usage remains optional.

## State Ownership Review

There are still two RDATAC state variables:

- `ADS1299Plus::rdatacActive_`
- `ADS1299_Protocol::rdatacActive_`

This is acceptable for the current transitional design because:

- `ADS1299Plus::rdatacActive_` preserves public `isRDATACActive()` behavior;
- `ADS1299_Protocol::rdatacActive_` guards the protocol-backed HAL path internally;
- `ADS1299Plus` explicitly synchronizes public state after `cmdReset()`, `cmdRDATAC()`, and `cmdSDATAC()`;
- host tests verify RDATAC public state and blocking behavior through the HAL-backed path.

This duplication should remain visible and should not be hidden by comments or broad refactors.

Future cleanup may consider a single source of truth, but that should not happen until the classic path is reviewed separately.

## Test Coverage

The current tests cover the integration well enough for this phase:

- core-only helper tests;
- standalone protocol object tests;
- HAL-backed `ADS1299Plus::begin()`;
- command sequencing through begin, configure, and end flows;
- single-register write sequencing;
- configure-defaults register sequencing;
- register access blocking during RDATAC;
- RDATAC frame decode through HAL-backed `ADS1299Plus`;
- RDATA on-demand frame decode through HAL-backed `ADS1299Plus`;
- ADS1299-4, ADS1299-6, and ADS1299 frame sizes;
- invalid device ID rejection;
- invalid STATUS sync rejection;
- insufficient frame capacity rejection before SPI traffic;
- RDATA blocking while RDATAC is active;
- shutdown sequencing and HAL transaction release.

The tests do not replace hardware validation.

## Remaining Risks

The remaining risks are practical rather than architectural:

- Real ADS1299 hardware still needs to validate timing margins, DRDY behavior, and long-running acquisition stability.
- Arduino example compilation depends on Arduino CLI or IDE availability on the local machine.
- CI should remain the authority for Arduino example compile validation.
- The classic path and HAL path now intentionally have duplicate protocol code paths; this is safer than collapsing them too early.

## Reference Alignment

B9 remains aligned with the Path B references and earlier reviews:

- HAL is the platform boundary.
- Protocol byte sequencing is now isolated behind `ADS1299_Protocol` for HAL-backed operation.
- Pure decode and helper logic stays in `ADS1299Core`.
- The Arduino user experience remains simple.
- No PlatformIO-specific files or external dependencies were introduced.
- The work stayed gradual and test-driven.

## Recommendation

B9 should be considered complete after this review.

The next work should not be another immediate routing change unless hardware or CI exposes a defect.

Recommended next phase:

```text
Phase B10 - Path B closure review and release readiness
```

B10 should decide whether the portable-core/HAL work is ready to merge toward the stable branch or whether it needs:

- Arduino example compile verification from CI results;
- hardware smoke testing notes;
- README wording updates;
- a final architecture diagram or short summary;
- a list of known limitations for non-Arduino backends.

## B9.5 Decision

The HAL-backed protocol integration is coherent and complete for Path B at this stage.

Keep the classic path stable.

Do not remove `ADS1299_SafeSPI`.

Do not move files or reorganize the repository yet.

Proceed to B10 as a closure and release-readiness phase.
