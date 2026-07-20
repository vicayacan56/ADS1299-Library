# Phase B2: HAL Integration Planning

**Status:** Historical Phase B2 Integration Plan

**Date:** 2026-07-07

**Branch:** `portable-core-hal`

**Current-use note:** This document records the Phase B2 implementation plan. Some code blocks are planning snippets, not a promise that the current source matches them line-for-line. For current usage and validation status, see `README.md`, `docs/user/hal-usage-guide.md`, `docs/user/testing-without-hardware.md`, and `docs/architecture/portability-roadmap.md`.

---

## Executive Summary

Phase B2 prepares the gradual integration of the HAL into ADS1299Plus and ADS1299_SafeSPI without breaking the existing API or examples. This is a conservative, step-by-step approach that preserves all validated behavior.

**Key Principles:**
- No breaking changes to public API
- Examples (BasicRead, RegisterDump) continue working without modification
- Validated hardware sequences remain untouched
- Critical functions (readFrameRDATAC, unpack24, SPI config) remain stable
- Gradual substitution of Arduino APIs, not immediate replacement

---

## Part 1: Dependency Analysis

### 1.1 Arduino Dependencies by File

#### ADS1299Plus.h
```cpp
#include <Arduino.h>       // Core Arduino definitions
#include "ADS1299_Registers.h"
```

**Direct Arduino Usage:**
- None in header (only includes Arduino.h indirectly via constructor parameter type reference)

**Critical Types:**
- `ADS1299_SafeSPI` reference (couples to SPI transport)
- `Pins` struct with GPIO pin numbers

**API Surface:**
- Struct `Pins`: cs, sclk, mosi, miso, drdy, start, reset, pwdn
- Constant: `ADS_PIN_UNUSED = 0xFF`
- Constructor: `ADS1299Plus(ADS1299_SafeSPI& spi, const Pins& pins)`

---

#### ADS1299Plus.cpp
```cpp
#include "ADS1299Plus.h"
#include "ADS1299_SafeSPI.h"
#include "ADS1299_Registers.h"
```

**Direct Arduino API Calls:**

| Function | Arduino API | Purpose | Criticality |
|----------|------------|---------|------------|
| `ads_wait_us()` | `delayMicroseconds(us)` | Timing (gaps in SPI protocol) | **HIGH** - timing-critical |
| `ads_wait_ms()` | `delay(ms)` | Timing (power-up, reset) | **HIGH** - power sequencing |
| `ads_wait_decode()` | `delayMicroseconds(3)` | tSDECODE delay (4 tCLK ≈ 2 µs) | **CRITICAL** - protocol timing |
| `pinStartHigh()` | `digitalWrite(pins_.start, HIGH)` | START control | **HIGH** - data conversion control |
| `pinStartLow()` | `digitalWrite(pins_.start, LOW)` | START control | **HIGH** - data conversion control |
| `pinResetPulse()` | `digitalWrite(pins_.reset, LOW/HIGH)` + delays | RESET pulse | **HIGH** - hardware reset sequence |
| `pinPowerDown()` | `digitalWrite(pins_.pwdn, LOW/HIGH)` | PWDN control (optional) | **MEDIUM** - optional control |
| `dataReady()` | `digitalRead(pins_.drdy)` | Poll DRDY | **HIGH** - data acquisition sync |
| `begin()` | `pinMode()` + `digitalWrite()` + `delayMicroseconds()` + `delay()` | Startup sequence | **CRITICAL** - initialization |
| `configureDefaults()` | No direct Arduino calls (all SPI via `spi_.*`) | Register config | **LOW** - delegated to SPI |
| All register access | `spi_.select()`, `spi_.xfer()`, `spi_.deselect()`, `ads_wait_decode()` | SPI commands | **CRITICAL** - all data I/O |

**Total Direct Arduino API Calls:** 8 functions
**Total Dependent on SPI Transport:** Register access layer (~15 methods)

---

#### ADS1299_SafeSPI.h
```cpp
#include <Arduino.h>
#include <SPI.h>
```

**Direct Arduino Dependencies:**
- `SPIClass& spi`: Template parameter (Arduino SPI class reference)
- `uint8_t csPin`: GPIO pin number
- `uint32_t spiHz`: SPI clock frequency

---

#### ADS1299_SafeSPI.cpp
```cpp
#include "ADS1299_SafeSPI.h"
```

**Direct Arduino API Calls:**

| Function | Arduino API | Purpose | Criticality |
|----------|------------|---------|------------|
| `begin()` | `pinMode(csPin_, OUTPUT)` | CS pin config | **HIGH** - chip select |
| `begin()` | `digitalWrite(csPin_, HIGH)` | CS idle state | **HIGH** - chip select |
| `begin()` | `spi_.begin()` | SPI init | **CRITICAL** - SPI hardware |
| `begin()` | `spi_.beginTransaction(SPISettings(...))` | SPI settings | **CRITICAL** - SPI protocol |
| `end()` | `spi_.endTransaction()` | SPI cleanup | **CRITICAL** - SPI protocol |
| `end()` | `spi_.end()` | SPI shutdown | **CRITICAL** - SPI hardware |
| `select()` | `digitalWrite(csPin_, LOW)` | CS assert | **CRITICAL** - chip select |
| `deselect()` | `digitalWrite(csPin_, HIGH)` | CS deassert | **CRITICAL** - chip select |
| `xfer()` | `spi_.transfer(data)` | SPI byte transfer | **CRITICAL** - all data I/O |
| `waitDecode()` | `delayMicroseconds(3)` | Protocol timing | **CRITICAL** - tSDECODE |

**Total Direct Arduino API Calls:** 10 functions

---

#### ADS1299_Registers.h
```cpp
#pragma once
#include <stdint.h>
```

**Arduino Dependencies:** NONE

**Content:** Pure register map (enum constants, no code)

---

#### examples/BasicRead/BasicRead.ino
```cpp
#include <Arduino.h>
#include <ADS1299Plus.h>
#include <ADS1299_SafeSPI.h>
```

**Direct Arduino Calls:**
- `Serial.begin()`, `Serial.println()`, `Serial.print()` - debug output
- `delay()` - timing
- `digitalWrite()` - potential if used for debugging
- SPI (implicitly via ADS1299Plus/SafeSPI)

**Scope:** Application layer; not part of driver refactoring target

---

### 1.2 Dependency Classification

#### A) Pure Portable Logic (No Arduino Required)

| Component | Location | Role |
|-----------|----------|------|
| Register map | ADS1299_Registers.h | All 27 register addresses and constants |
| Register helpers | ADS1299_Registers.h | ADS_CFG1_*, ADS_CH_*, etc. bit field macros |
| Channel count detection | `ADS1299Plus::channelsFromDeviceID()` | Decode ID register variant |
| Frame unpacking | `ADS1299Plus::unpack24()` | Convert 3-byte MSB to signed 32-bit |
| Status decoding | `ADS1299Plus::statusHasSync()`, etc. | Extract status bits |
| Register access protocol | `writeOne_()`, `readOne_()`, `writeBurst_()`, `readBurst_()` | SPI protocol sequencing (frame assembly) |
| Configuration logic | `setDataRate()`, `setChannel()`, `setChannelGain()`, etc. | Register manipulation (portable) |

**Lines of Code:** ~400 LOC (can move to core without Arduino)

---

#### B) Arduino-Specific Logic (Direct GPIO/SPI/Timing)

| Component | Location | Arduino APIs | Portability |
|-----------|----------|--------------|------------|
| CS control | ADS1299_SafeSPI | `digitalWrite()` | Should move to HAL |
| SPI transfer | ADS1299_SafeSPI | `spi_.transfer()` | Should move to HAL |
| SPI init/cleanup | ADS1299_SafeSPI | `spi_.begin/end/beginTransaction/endTransaction` | Should move to HAL |
| START control | ADS1299Plus | `digitalWrite()` | Should move to HAL |
| RESET control | ADS1299Plus | `digitalWrite()` + timing | Should move to HAL |
| PWDN control | ADS1299Plus | `digitalWrite()` | Should move to HAL |
| DRDY polling | ADS1299Plus | `digitalRead()` | Should move to HAL |
| PIN config | ADS1299Plus, ADS1299_SafeSPI | `pinMode()` | Should move to HAL init |
| Delays | ADS1299Plus, ADS1299_SafeSPI | `delay()`, `delayMicroseconds()` | Should move to HAL |

**Lines of Code:** ~50 LOC (can move to HAL implementations)

---

#### C) Mixed Logic (Sequence Orchestration)

| Component | Location | Portable | Arduino-Dependent |
|-----------|----------|----------|------------------|
| `begin()` setup sequence | ADS1299Plus | Channel count detection, state init | `pinMode()`, `digitalWrite()`, `delay()` |
| `configureDefaults()` | ADS1299Plus | Register config logic | None directly (delegated via HAL) |
| `readFrameRDATAC()` | ADS1299Plus | Frame unpacking, status check | `spi_.select/xfer/deselect` |
| `readDataOnDemand()` | ADS1299Plus | Frame unpacking, status check | `spi_.select/xfer/deselect` |
| SPI transaction control | ADS1299_SafeSPI | State tracking | `spi_.beginTransaction/endTransaction` |

**Lines of Code:** ~200 LOC (requires careful decomposition)

---

### 1.3 Critical Components (DO NOT MODIFY)

The following are validated on hardware and must remain functionally identical:

#### readFrameRDATAC()
```cpp
bool ADS1299Plus::readFrameRDATAC(uint32_t &status24, int32_t *chOut, size_t capacity)
```

**Why:** Core data acquisition loop; timing and sequence are hardware-validated.

**Must Preserve:**
- Frame byte count calculation
- Status byte extraction: `status24 = ((uint32_t)rxBuf[0] << 16) | ...`
- Channel unpacking: `unpack24(&rxBuf[STATUS_BYTES + BYTES_PER_CHANNEL * i])`
- Sync check: `statusHasSync(status24)`

**Can Refactor:** Internal SPI mechanics (select/xfer/deselect) via HAL

---

#### unpack24()
```cpp
static inline int32_t unpack24(const uint8_t b[3])
```

**Why:** Hardware-specific 24-bit signed conversion; used by all frame readers.

**Must Preserve:**
- MSB-first byte ordering
- Sign extension from bit 23 to bit 31

**Can Refactor:** None (pure bitwise logic)

---

#### SPI Configuration
```cpp
spi_.beginTransaction(SPISettings(spiHz_, MSBFIRST, SPI_MODE1));
```

**Why:** ADS1299 timing margins are tight; mode and clock validated experimentally.

**Must Preserve:**
- SPI_MODE1 (CPOL=0, CPHA=1)
- Clock frequency (2 MHz nominal, can go to ~8 MHz max)
- MSB-first byte order

**Can Refactor:** Encapsulate in HAL, but preserve settings

---

#### Startup Sequence (begin())
```cpp
// 1) GPIO config
pinMode(pins_.cs, OUTPUT);
// 2) Wait for power stabilization
ads_wait_ms(5);
// 3) SPI init
spi_.begin();
// 4) Digital reset
cmdReset();
// 5) Exit continuous read mode
cmdSDATAC(); cmdStop();
// 6) Verify ID
readReg(ADS_REG_ID, id);
```

**Why:** Power sequencing must follow datasheet timing requirements (section 11.1).

**Must Preserve:**
- 5 ms wait after GPIO config
- Reset pulse timing (10 µs low, 20 µs after high)
- SDATAC/STOP before register access
- ID verification before use

**Can Refactor:** GPIO/delay calls via HAL, but sequence immutable

---

#### Register Values (Defaults)
```cpp
static constexpr uint8_t kCFG1_Default = ADS_CFG1_250SPS;      // 0x96
static constexpr uint8_t kCFG2_Default = ADS_CFG2_TEST_OFF;    // 0x10
static constexpr uint8_t kCFG3_Default = ADS_CFG3_INTREF_NO_BIAS; // 0x60
static constexpr uint8_t kLOFF_Default = ADS_LOFF_AC_24NA_31HZ_87_5PCT_LEGACY;
```

**Why:** Conservative defaults validated in biomedical applications.

**Must Preserve:**
- Bit values unchanged
- Comments explaining hardware constraints

**Can Refactor:** None (constants only)

---

### 1.4 HAL Coverage Analysis

#### Current HAL Interface (Phase B1.1)

```cpp
class ADS1299_HAL {
    virtual void begin() = 0;
    virtual void end() = 0;
    virtual void csLow() = 0;
    virtual void csHigh() = 0;
    virtual uint8_t spiTransfer(uint8_t data) = 0;
    virtual void delayMicroseconds(uint32_t us) = 0;
    virtual void delayMilliseconds(uint32_t ms) = 0;
    virtual void setStart(bool high) = 0;
    virtual void setReset(bool high) = 0;
    virtual void setPwdn(bool high) = 0;
    virtual bool readDrdy() = 0;
};
```

**Coverage:**
- ✅ CS control (csLow, csHigh)
- ✅ SPI transfer (spiTransfer)
- ✅ Timing (delayMicroseconds, delayMilliseconds)
- ✅ START control (setStart)
- ✅ RESET control (setReset)
- ✅ PWDN control (setPwdn)
- ✅ DRDY polling (readDrdy)
- ✅ Initialization (begin/end)
- ✅ Optional PWDN support (PIN_UNUSED)

---

#### Gaps Identified

| Gap | Impact | Severity | Required For |
|-----|--------|----------|--------------|
| **No SPISettings support** | Can't configure SPI clock/mode/order programmatically | MEDIUM | Multi-platform SPI config |
| **No SPI transaction control** | beginTransaction/endTransaction not abstracted | MEDIUM | Thread-safe SPI in RTOS |
| **No pin configuration** | pinMode() calls still direct | LOW | Full GPIO abstraction |
| **No error reporting** | begin() returns void, no failure indication | LOW | Diagnostics |
| **No SPIClass selection** | Hardcoded to SPI object; multiple SPI not supported | LOW | Boards with SPI1, SPI2, etc. |
| **No common PIN_UNUSED constant** | Driver has ADS_PIN_UNUSED; HAL doesn't export it | LOW | Code consistency |
| **No HAL factory/selection** | No mechanism to choose HAL implementation at runtime | LOW | Firmware flexibility |
| **No platform-specific features** | No extension point for board-specific code | LOW | Future needs (DMA, interrupts) |

---

## Part 2: Integration Strategy

### 2.1 Phase B2.1: Refine HAL for SPI Control

**Objective:** Add neutral SPI configuration and transaction control to HAL.

**Files to Modify:**
- `src/hal/ADS1299_HAL_Types.h` (add neutral types)
- `src/hal/ADS1299_HAL.h` (add methods)
- `src/arduino/ADS1299_ArduinoHAL.h` (add methods)
- `src/arduino/ADS1299_ArduinoHAL.cpp` (implement)

**Changes:**

```cpp
// In ADS1299_HAL.h, add after spiTransfer():

/**
 * Configure and begin an SPI transaction using platform-neutral settings.
 * 
 * @param config SPI clock frequency, bit order, and mode
 */
virtual void beginTransaction(const ADS1299_SpiConfig& config) = 0;

/**
 * End an SPI transaction.
 * Encapsulates endTransaction().
 */
virtual void endTransaction() = 0;
```

**Arduino Implementation:**
```cpp
void ADS1299_ArduinoHAL::beginTransaction(const ADS1299_SpiConfig& config)
{
    SPI.beginTransaction(SPISettings(
        config.clockHz,
        config.bitOrder == ADS1299_SpiBitOrder::MSB_FIRST ? MSBFIRST : LSBFIRST,
        static_cast<uint8_t>(config.mode)
    ));
}

void ADS1299_ArduinoHAL::endTransaction()
{
    SPI.endTransaction();
}
```

**Why:**
- Isolates SPI mode/clock from driver logic
- Allows different SPI configs per board
- Enables multi-platform support

**Risk:** None (additive; doesn't touch existing methods)

**Verification:**
- Compile ADS1299Plus with new HAL
- Verify examples still work (no integration yet)
- No behavior change (HAL still unused)

**Affects API:** NO (HAL is internal; examples unchanged)

---

### 2.2 Phase B2.2: Adapt ADS1299_SafeSPI for Optional HAL

**Objective:** Make ADS1299_SafeSPI able to use HAL **OR** Arduino directly (backward compat).

**Files to Modify:**
- `src/ADS1299_SafeSPI.h` (add optional HAL path)
- `src/ADS1299_SafeSPI.cpp` (add conditional logic)

**Changes:**

```cpp
// In ADS1299_SafeSPI.h

class ADS1299_SafeSPI {
public:
    // Existing Arduino constructor (unchanged)
    explicit ADS1299_SafeSPI(uint8_t csPin,
                           SPIClass& spi = SPI,
                           uint32_t spiHz = DEFAULT_SPI_HZ);

    // New HAL constructor (additive)
    explicit ADS1299_SafeSPI(ADS1299_HAL& hal, uint32_t spiHz = DEFAULT_SPI_HZ);

private:
    // Existing: Arduino path
    uint8_t csPin_;
    SPIClass* spi_;
    uint32_t spiHz_;
    bool active_ = false;

    // New: HAL path
    ADS1299_HAL* hal_;
    bool useHal_ = false;
};
```

**Implementation Logic (in .cpp):**

```cpp
// Arduino constructor (existing)
ADS1299_SafeSPI::ADS1299_SafeSPI(uint8_t csPin, SPIClass& spi, uint32_t spiHz)
    : csPin_(csPin), spi_(&spi), spiHz_(spiHz), hal_(nullptr), useHal_(false) {}

// HAL constructor (new)
ADS1299_SafeSPI::ADS1299_SafeSPI(ADS1299_HAL& hal, uint32_t spiHz)
    : csPin_(0xFF), spi_(nullptr), spiHz_(spiHz), hal_(&hal), useHal_(true) {}

// begin() with dual path
void ADS1299_SafeSPI::begin()
{
    if (useHal_) {
        ADS1299_SpiConfig config;
        config.clockHz = spiHz_;
        hal_->beginTransaction(config);
        hal_->begin();
    } else {
        // Existing Arduino path
        pinMode(csPin_, OUTPUT);
        digitalWrite(csPin_, HIGH);
        if (active_) return;
        spi_->begin();
        spi_->beginTransaction(SPISettings(spiHz_, MSBFIRST, SPI_MODE1));
        active_ = true;
    }
}

// xfer() with dual path
uint8_t ADS1299_SafeSPI::xfer(uint8_t data)
{
    return useHal_ ? hal_->spiTransfer(data) : spi_->transfer(data);
}

// etc. for other methods
```

**Why:**
- ADS1299_SafeSPI becomes a transport adapter
- Can use Arduino SPI **OR** HAL-based SPI
- Existing code (Arduino path) works unchanged
- New code can opt into HAL path

**Risk:** MEDIUM
- Dual code paths can diverge
- Needs testing on both paths
- Can only use ONE at runtime (constructor choice)

**Verification:**
- Existing examples must still compile and work (Arduino path)
- Create new test sketch using HAL constructor
- Compare SPI traces (if possible)

**Affects API:** NO (additive constructor; existing code unchanged)

---

### 2.3 Phase B2.3: Add HAL Constructor to ADS1299Plus

**Objective:** Allow ADS1299Plus to accept HAL directly (while keeping SafeSPI optional).

**Files to Modify:**
- `src/ADS1299Plus.h` (add constructor overload)
- `src/ADS1299Plus.cpp` (implement dual init)

**Changes:**

```cpp
// In ADS1299Plus.h, add after existing constructor:

/**
 * Constructor using Hardware Abstraction Layer.
 * Phase B2 alternative to Arduino SafeSPI.
 * 
 * @param hal HAL instance
 * @param pins GPIO pin assignments (START, RESET, PWDN, DRDY)
 */
ADS1299Plus(ADS1299_HAL& hal, const Pins& pins);
```

**Implementation Strategy:**

Store both paths internally:

```cpp
// In ADS1299Plus private:

ADS1299_SafeSPI* spi_;        // Arduino path
ADS1299_HAL* hal_;            // HAL path
bool useHal_;                 // Which path active
Pins pins_;
```

Then in `begin()`, detect which was initialized:

```cpp
bool ADS1299Plus::begin()
{
    if (useHal_) {
        // HAL init path
        hal_->begin();
        hal_->setPwdn(true);     // PWDN to power-on
        hal_->delayMilliseconds(5);
        hal_->csHigh();
    } else {
        // Existing Arduino path
        pinMode(pins_.cs, OUTPUT);
        // ... existing code
    }
    // ... rest of begin() (same for both paths)
}
```

**Why:**
- ADS1299Plus can optionally use HAL
- Examples using SafeSPI continue working
- New embedded systems can use HAL directly
- No changes to examples or default behavior

**Risk:** LOW
- Dual code paths still, but well-isolated
- Existing API path unchanged
- New path is opt-in

**Verification:**
- Existing examples must still work (SafeSPI path)
- Create test using HAL constructor
- Compare initialization sequences

**Affects API:** YES (additive)
- New constructor (backward compatible)
- Existing code unaffected

---

### 2.4 Phase B2.4: Verify Examples Compile & Run

**Objective:** Ensure no regressions in existing sketches.

**Scope:** BasicRead, RegisterDump

**Compilation Requirements:**
- Both examples compile without modification
- Warnings must be zero (if possible)
- Binary size must not grow significantly

**Functional Requirements:**
- BasicRead reads device ID correctly
- BasicRead reads a few frames at 250 SPS
- RegisterDump dumps all registers
- Both work on target hardware (Arduino Uno or equivalent)

**Validation Checklist:**
```
☐ BasicRead.ino compiles
☐ BasicRead.ino links
☐ BasicRead.ino flashes to target
☐ BasicRead.ino runs (device detected)
☐ BasicRead.ino reads frames correctly
☐ RegisterDump.ino compiles
☐ RegisterDump.ino links
☐ RegisterDump.ino flashes to target
☐ RegisterDump.ino runs (registers readable)
☐ No new warnings introduced
☐ Binary size change < 5%
```

---

### 2.5 Phase B2.5 (Future): Optional HAL Example

**Status:** NOT YET (deferred to Phase B3)

**Objective:** Create example showing HAL usage (for documentation).

**Proposed Files:**
- `examples/HalBasedRead/HalBasedRead.ino` (optional, Phase B3+)

**Content:**
```cpp
#include <Arduino.h>
#include <ADS1299Plus.h>
#include <ADS1299_ArduinoHAL.h>

// HAL-based instead of SafeSPI
ADS1299_ArduinoHAL hal(PIN_CS, PIN_START, PIN_RESET, PIN_PWDN, PIN_DRDY);
ADS1299Plus::Pins pins = { PIN_CS, SCK, MOSI, MISO, PIN_DRDY, PIN_START, PIN_RESET, PIN_PWDN };
ADS1299Plus ads(hal, pins);  // New HAL constructor

void setup() {
    if (!ads.begin()) {
        // error
    }
    // ... same as BasicRead
}
```

**Why Deferred:**
- Requires B2.1–B2.4 complete and tested first
- Documentation example, not critical for functionality
- Added in Phase B3 after validation

---

## Part 3: Implementation Roadmap

### Timeline

| Phase | Files | Risk | Duration | Validation |
|-------|-------|------|----------|-----------|
| **B2.1** | HAL.h, ArduinoHAL.h/.cpp | LOW | 1 session | Compile only |
| **B2.2** | SafeSPI.h/.cpp | MEDIUM | 2 sessions | Compile + dual path testing |
| **B2.3** | ADS1299Plus.h/.cpp | MEDIUM | 2 sessions | Compile + HAL constructor test |
| **B2.4** | Examples (no mods) | LOW | 1 session | Hardware validation |
| **B2.5** | Optional example | LOW | 1 session | Documentation (Phase B3) |

---

### Per-Phase Checklist

#### B2.1 Checklist
```
Code:
☐ Add ADS1299_SpiConfig neutral type
☐ Add beginTransaction() to ADS1299_HAL
☐ Add endTransaction() to ADS1299_HAL
☐ Implement both in ADS1299_ArduinoHAL
☐ Document with Doxygen comments

Testing:
☐ Compile src/ (no errors)
☐ Compile examples/ (no errors, no new warnings)
☐ Verify no behavior change (examples still work)

Documentation:
☐ Update docs/history/phase-b2-integration-plan.md
```

#### B2.2 Checklist
```
Code:
☐ Add HAL constructor to ADS1299_SafeSPI
☐ Implement dual-path begin()
☐ Implement dual-path select/deselect/xfer
☐ Update comments explaining both paths

Testing:
☐ Compile with Arduino path (verify existing behavior)
☐ Compile with HAL path (verify new path compiles)
☐ Create test sketch using both paths
☐ Verify no regressions in examples

Documentation:
☐ Update docs/history/phase-b2-integration-plan.md
☐ Add usage examples in header comments
```

#### B2.3 Checklist
```
Code:
☐ Add HAL constructor to ADS1299Plus
☐ Add useHal_ flag and hal_ pointer
☐ Implement dual-path begin()
☐ Implement dual-path for GPIO operations (START, RESET, PWDN, DRDY)
☐ Ensure register access logic is shared (single path)

Testing:
☐ Compile with Arduino path (verify existing behavior)
☐ Compile with HAL path (verify new path compiles)
☐ Create test using ADS1299Plus(hal, pins) constructor
☐ Verify frame reading works via both paths
☐ Compare initialization traces

Documentation:
☐ Update docs/history/phase-b2-integration-plan.md
☐ Update docs/architecture/portability-roadmap.md
```

#### B2.4 Checklist
```
Validation:
☐ BasicRead compiles (Arduino Uno or equivalent)
☐ BasicRead flashes and runs
☐ RegisterDump compiles
☐ RegisterDump flashes and runs
☐ No binary size regression > 5%
☐ SPI traces match pre-refactor

Documentation:
☐ Confirm no changes to examples/
☐ Document validation results
```

---

## Part 4: Risk Mitigation

### Dual-Path Divergence

**Risk:** Arduino and HAL paths drift over time.

**Mitigation:**
- Keep parallel code paths as simple as possible
- Extract common logic into static utility functions
- Add side-by-side tests (Arduino vs HAL) in test suite
- Document exactly what differs between paths

---

### ADS1299_SafeSPI Constructor Ambiguity

**Risk:** Overloaded constructors could confuse users.

**Mitigation:**
```cpp
// Mark which is which clearly:

// ARDUINO PATH (existing, unchanged)
ADS1299_SafeSPI spi_arduino(PIN_CS);  // Obvious: pin number

// HAL PATH (new, explicit)
ADS1299_SafeSPI spi_hal(hal);  // Clear: HAL reference

// Compiler will select correct overload
```

---

### Testing Coverage

**Risk:** Not testing both paths thoroughly.

**Mitigation:**
- Mandatory: Verify both Arduino and HAL paths compile
- Mandatory: Run examples with Arduino path
- Optional: Create parallel test sketches (one per path)
- Future: Add integration test suite

---

## Part 5: Success Criteria

### Phase B2 Complete When:

1. ✅ HAL has SPI transaction methods (B2.1)
2. ✅ ADS1299_SafeSPI has dual-path support (B2.2)
3. ✅ ADS1299Plus has HAL constructor (B2.3)
4. ✅ Examples compile and run unchanged (B2.4)
5. ✅ No breaking changes to public API
6. ✅ Documentation updated in phase-b2-integration-plan.md
7. ✅ All code compiles with zero warnings
8. ✅ No new dependencies introduced

### NOT Required:

- ❌ Full refactoring of ADS1299Plus to use HAL exclusively
- ❌ Removal of Arduino path
- ❌ Changes to examples
- ❌ HAL example (deferred to B3)
- ❌ Other platform backends (ESP-IDF, STM32, etc.)

---

## Part 6: Notes for Next Phase

### What's Left (Phase B3+):

1. **Gradual Substitution:** Replace direct Arduino calls in ADS1299Plus begin() with HAL calls
2. **Refactor readFrameRDATAC():** Investigate moving select/xfer/deselect to HAL (if beneficial)
3. **Additional Backends:** Implement STM32 HAL, ESP-IDF, Zephyr backends
4. **Tests:** Create test suite for HAL and backends
5. **Documentation:** Write porting guide for new platforms

### Decision Points for Phase B3:

- Should readFrameRDATAC() have a HAL method for SPI transaction, or keep it as select/xfer/deselect?
- Should we support multiple HAL instances at runtime (factory pattern)?
- Should we extract delay logic to a separate timing module?
- Do we want optional interrupts/async support?

---

## Appendix: File Structure After B2.4

```
src/
  ADS1299Plus.h           (updated: HAL constructor)
  ADS1299Plus.cpp         (updated: dual-path begin/GPIO)
  ADS1299_SafeSPI.h       (updated: HAL constructor)
  ADS1299_SafeSPI.cpp     (updated: dual-path methods)
  ADS1299_Registers.h     (unchanged)
  
  hal/
    ADS1299_HAL.h         (updated: SPI transaction methods)
    ADS1299_HAL_Types.h   (new: neutral SPI/GPIO types)
  
  arduino/
    ADS1299_ArduinoHAL.h   (updated: SPI transaction methods)
    ADS1299_ArduinoHAL.cpp (updated: SPI transaction impl)

examples/
  BasicRead/BasicRead.ino         (unchanged)
  RegisterDump/RegisterDump.ino   (unchanged)

docs/
  portability-roadmap.md          (updated: B2 status)
  phase-b2-integration-plan.md    (THIS FILE)
  uno-q-eeg-midi.md               (unchanged)

library.properties  (unchanged)
README.md           (unchanged)
.gitignore          (unchanged)
```

---

**End of Phase B2 Integration Plan**
