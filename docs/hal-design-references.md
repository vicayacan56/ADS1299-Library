# HAL Design References — Phase B2.0 Technical Study

**Status:** Technical Reference & Design Study

**Date:** 2026-07-07

**Branch:** `portable-core-hal`

**Current-use note:** This document records the Phase B2.0 design study and reference reasoning. Some snippets are planning examples. For current user-facing usage and implementation status, see `README.md`, `docs/hal-usage-guide.md`, and `docs/portability-roadmap.md`.

**Document Purpose:** Define HAL design principles and neutral interfaces before Phase B2.1 implementation.

---

## 1. Purpose

Phase B2.0 is a **technical study phase** that establishes design principles and reference architectures for the Hardware Abstraction Layer (HAL) without modifying existing driver code.

**Goals:**
- Define neutral, platform-agnostic types and interfaces
- Document hardware constraints from ADS1299 datasheet
- Map current Arduino-specific code to abstract concepts
- Plan conservative, incremental migration (B2.1 → B3)
- Provide reference for future platform implementations

**Scope:**
- Analysis only (no code changes)
- Design documentation
- Reference architecture sketches
- Cross-platform comparison

**Out of Scope:**
- Implementing HAL changes (Phase B2.1)
- Refactoring ADS1299Plus/SafeSPI (Phase B2.2+)
- Other platform backends (Phase B3+)
- Dynamic HAL selection, DMA, interrupts, RTOS integration

---

## 2. Primary Hardware Constraints from ADS1299

The HAL must respect these hardware requirements from the TI ADS1299-x family (datasheet Rev. C).

### 2.1 SPI Protocol (Section 9.5, 9.6)

**SPI Frame Structure (Section 9.5.1):**
- **CPOL:** 0 (CLK idle low)
- **CPHA:** 1 (sample on leading edge, change on trailing edge) → **SPI_MODE1**
- **Byte Order:** MSB-first (bit 7 first, bit 0 last)
- **Clock Frequency:** Nominal 2.048 MHz, typical range 0.5–8 MHz
- **Frame Format:** [COMMAND/ADDR] [COUNT/ADDR] [DATA...] [wait tSDECODE]

**Chip Select Timing (Section 9.5.2):**
- CS is active-low
- CS must go low before first clock edge
- CS must stay low during entire transaction (all bytes)
- CS must go high after last data bit
- **tSDECODE (SPI decode time) ≥ 4 tCLK** (≈1.95 µs at 2.048 MHz)
  - No command valid until tSDECODE elapsed
  - Minimum delay after CS high and before next CS low: **4 tCLK**

**SPI Transaction Sequence (typical):**
```
BEGIN TRANSACTION
  CS low
  Send command byte (8 bits)
  [optional: address/count/data bytes]
  Receive [optional: data bytes]
  CS high
  Wait ≥ tSDECODE (4 tCLK)
END TRANSACTION
```

### 2.2 Control Signals

**DRDY (Data Ready) Pin (Section 9.4.2, 11.1):**
- Open-drain output, active-low
- Pulls low when ADC conversion data is ready for readout
- Must use external pull-up (typically 100 kΩ) or GPIO INPUT_PULLUP
- Read via GPIO input (digitalRead or equivalent)
- Used for interrupt-driven or polled data acquisition

**START Pin (Section 9.5.3.5, 11.1):**
- Digital input to ADS1299
- High: Start continuous conversions (after cmdSTART)
- Low: Stop conversions (after cmdSTOP)
- Typically driven by MCU GPIO output

**RESET Pin (Section 9.5.3.4, 11.1):**
- Active-low digital input
- Pulse low (~10 µs) → triggers digital reset
- Default: held high via external pull-up or power supply
- Reset sequence: LOW 10µs → HIGH 20µs

**PWDN (Power Down) Pin (Section 9.5.3.2, 11.1):**
- Active-low digital input
- Low: Power-down mode (minimal current)
- High: Normal operation
- Optional: often tied directly to power supply (no GPIO required)
- Value: ADS_PIN_UNUSED if not available

### 2.3 Command Sequence Constraints (Section 9.5.3)

**RDATAC (Continuous Read Mode):**
- Once entered, device continuously outputs ADC data on DOUT
- No register access allowed in RDATAC mode
- Must exit via SDATAC before reading/writing registers

**SDATAC (Stop Continuous Read):**
- Exits RDATAC mode
- Must be called before any register read/write

**Register Access (RREG / WREG):**
- Can only read/write registers when NOT in RDATAC
- RREG requires SDATAC first if device was in RDATAC

---

## 3. Arduino Library Layout Constraints

### 3.1 Compilation Unit Structure

The Arduino library specification ([Arduino CLI Library Specification](#10-references)) defines:

**Source Directory (`src/`):**
- All C++ files in `src/` and subdirectories are compiled
- Subdirectories (`src/hal/`, `src/arduino/`, etc.) are supported
- All headers in `src/` and subdirectories are included in the include path

**Include Path Behavior:**
- Arduino IDE adds `-I<library>/src/` to compilation flags
- Headers in `src/` can use `#include "Header.h"` (relative)
- Headers in subdirectories can use `#include "../hal/Header.h"` (relative path)
- External code includes: `#include <LibraryName/Header.h>` (angle brackets)

**Examples Directory (`examples/`):**
- Each subdirectory is a sketch
- Arduino IDE compiles each as a separate sketch
- Sketches can `#include <LibraryName/Header.h>`

**Property File (`library.properties`):**
- Metadata: name, version, author, depends
- Does not affect HAL design

### 3.2 Implications for HAL Structure

**Recommended Layout:**
```
src/
  ADS1299Plus.h         (public class, includes via <ADS1299Plus.h>)
  ADS1299Plus.cpp
  ADS1299_SafeSPI.h     (public transport, includes via <ADS1299_SafeSPI.h>)
  ADS1299_SafeSPI.cpp
  ADS1299_Registers.h   (public constants)

  hal/
    ADS1299_HAL.h       (abstract interface, internal use)
    ADS1299_HAL_Types.h (neutral types: SpiConfig, etc.)

  arduino/
    ADS1299_ArduinoHAL.h   (Arduino implementation, internal use)
    ADS1299_ArduinoHAL.cpp
```

**Benefits:**
- Subdirectories don't pollute public include space
- Internal HAL headers not in user's namespace
- Future: other backends in `src/esp32/`, `src/stm32/`, `src/zephyr/`
- Library users include only public classes: `<ADS1299Plus.h>`, `<ADS1299_SafeSPI.h>`

---

## 4. Cross-Platform SPI Design Considerations

### 4.1 Platform SPI APIs (Overview)

Different platforms provide different SPI abstractions. The HAL must abstract these differences into neutral concepts.

#### Arduino SPI Library (Arduino Core, reference implementation)

```cpp
#include <SPI.h>

SPI.begin();                    // Initialize
SPI.beginTransaction(SPISettings(clockHz, bitOrder, mode));
uint8_t result = SPI.transfer(data);
SPI.endTransaction();
SPI.end();
```

**Characteristics:**
- `SPISettings` encapsulates clock, bit order, SPI mode
- `transfer()` is blocking, single byte
- `beginTransaction()`/`endTransaction()` for thread-safety
- Multiple SPIClass instances possible (SPI, SPI1, etc.)

#### ESP-IDF (Espressif)

```cpp
#include "driver/spi_master.h"

spi_device_handle_t spi;
spi_device_interface_config_t config = {
    .clock_speed_hz = 2000000,
    .mode = 1,
    .spics_io_num = PIN_CS,
};
spi_bus_add_device(host, &config, &spi);
spi_transaction_t t = { .length = 8, .tx_buffer = &data };
spi_device_transmit(spi, &t);
```

**Characteristics:**
- Device handle per peripheral
- Configuration struct with platform types
- Transaction objects
- DMA support built-in
- Multi-device arbitration via transactions

#### Zephyr RTOS (Nordic, STMicroelectronics, others)

```cpp
#include <zephyr/drivers/spi.h>

const struct device *spi_dev = DEVICE_DT_GET(DT_NODELABEL(spi0));
const struct spi_config config = {
    .frequency = 2000000,
    .operation = SPI_WORD_SET(8) | SPI_MODE_GET(1) | SPI_MSB,
};
struct spi_buf buf = { .buf = data, .len = 1 };
spi_transceive(spi_dev, &config, &buf, &buf);
```

**Characteristics:**
- Device tree configuration
- Device pointers instead of IDs
- Configuration flags (bit-packed)
- Devicetree overlay for pin assignment
- Devicetree objects cannot be abstracted (runtime)

#### STM32 HAL (STMicroelectronics)

```cpp
#include "stm32f4xx_hal.h"

SPI_HandleTypeDef hspi = { ... };
HAL_SPI_Init(&hspi);
HAL_SPI_Transmit(&hspi, (uint8_t *)data, 1, timeout);
```

**Characteristics:**
- Peripheral handles (struct with configuration)
- Separate Tx/Rx functions
- DMA capable
- Blocking or polling
- Pins configured via `HAL_SPI_MspInit()` (platform-specific)

### 4.2 What Core Should NOT Know

The ADS1299 core driver logic must **never reference** platform-specific types:

```cpp
// ❌ DO NOT export to core:
SPISettings          // Arduino specific
SPIClass*            // Arduino specific
spi_device_handle_t  // ESP-IDF specific
spi_config           // ESP-IDF specific
spi_device_t*        // Zephyr specific
SPI_HandleTypeDef*   // STM32 specific
```

**Reason:** If core knows about these, porting to a new platform requires modifying core code, defeating the purpose of HAL.

### 4.3 Neutral Concepts

Instead, the HAL exports only **neutral abstractions**:

```cpp
// ✅ Neutral concepts (platform-agnostic):
uint32_t clockHz;              // Frequency in Hz
enum SpiBitOrder { MSB, LSB }; // Byte order
enum SpiMode { 0, 1, 2, 3 };  // CPOL/CPHA
uint8_t byte;                  // Transfer data
bool highOrLow;                // GPIO logic level
uint32_t microseconds;         // Time delay
```

---

## 5. Proposed Neutral HAL Types

To ensure portability, the HAL should define neutral types that every platform can implement.

### 5.1 SPI Configuration Types

```cpp
// File: src/hal/ADS1299_HAL_Types.h

#pragma once
#include <stdint.h>

/**
 * ADS1299_SpiBitOrder - SPI bit transmission order
 */
enum class ADS1299_SpiBitOrder : uint8_t {
  MSB_FIRST = 0,  // Most significant bit first (default for ADS1299)
  LSB_FIRST = 1   // Least significant bit first (not supported by ADS1299)
};

/**
 * ADS1299_SpiMode - SPI clock phase and polarity
 * CPOL = clock polarity, CPHA = clock phase
 * 
 * MODE0: CPOL=0, CPHA=0 (CLK idle low, sample on trailing edge)
 * MODE1: CPOL=0, CPHA=1 (CLK idle low, sample on leading edge) ← ADS1299 default
 * MODE2: CPOL=1, CPHA=0 (CLK idle high, sample on trailing edge)
 * MODE3: CPOL=1, CPHA=1 (CLK idle high, sample on leading edge)
 */
enum class ADS1299_SpiMode : uint8_t {
  MODE0 = 0,
  MODE1 = 1,  // ← ADS1299 requirement
  MODE2 = 2,
  MODE3 = 3
};

/**
 * ADS1299_SpiConfig - SPI configuration (platform-independent)
 * 
 * Encapsulates clock frequency, bit order, and SPI mode.
 * Every platform HAL implementation must translate this to its native config.
 */
struct ADS1299_SpiConfig {
  /**
   * Clock frequency in Hz.
   * ADS1299 supports 0.5–8 MHz; nominal 2.048 MHz.
   */
  uint32_t clockHz = 2048000UL;

  /**
   * Bit transmission order.
   * ADS1299 requires MSB_FIRST.
   */
  ADS1299_SpiBitOrder bitOrder = ADS1299_SpiBitOrder::MSB_FIRST;

  /**
   * SPI mode (CPOL, CPHA).
   * ADS1299 requires MODE1 (CPOL=0, CPHA=1).
   */
  ADS1299_SpiMode mode = ADS1299_SpiMode::MODE1;
};

/**
 * ADS1299_GpioLevel - Digital logic level
 * Neutral representation (platform-independent).
 */
enum class ADS1299_GpioLevel : uint8_t {
  LevelLow  = 0,
  LevelHigh = 1
};

```

**Rationale:**
- No platform-specific types (no `SPISettings`, `spi_config_t`, etc.)
- All values are portable primitives (`uint32_t`, enums)
- Comments explain ADS1299 requirements
- Every platform can map these to its native config

---

## 6. Proposed HAL Interface Direction

Based on the analysis above, the HAL interface should be extended (Phase B2.1) to include SPI configuration and transaction methods.

### 6.1 Extended HAL Interface (Target Design)

```cpp
// File: src/hal/ADS1299_HAL.h

#pragma once
#include <stdint.h>
#include "ADS1299_HAL_Types.h"

/**
 * ADS1299_HAL - Hardware Abstraction Layer (Phase B2+)
 * 
 * Abstract base class defining all hardware operations needed by ADS1299Plus driver.
 * Every platform (Arduino, ESP32, STM32, etc.) provides a concrete implementation.
 * 
 * INVARIANTS:
 * - All timings are in hardware units (microseconds, milliseconds).
 * - All GPIO levels are HIGH/LOW (neutral).
 * - All SPI operations respect ADS1299 protocol timing.
 */
class ADS1299_HAL {
public:
    virtual ~ADS1299_HAL() = default;

    // ========== INITIALIZATION ==========

    /**
     * Initialize HAL resources (GPIO, SPI, timers).
     * Called once during driver startup (ADS1299Plus::begin()).
     * 
     * PRECONDITION: All pins are allocated.
     * POSTCONDITION: SPI and GPIO ready; not yet in transaction.
     */
    virtual void begin() = 0;

    /**
     * Deinitialize HAL resources.
     * Called during driver shutdown (ADS1299Plus::end()).
     * 
     * POSTCONDITION: All resources released; safe to delete HAL instance.
     */
    virtual void end() = 0;

    // ========== SPI TRANSACTIONS ==========

    /**
     * Begin an SPI transaction with specified configuration.
     * 
     * Must set up SPI peripheral for:
     * - Clock frequency (config.clockHz)
     * - Bit order (config.bitOrder, typically MSB_FIRST)
     * - SPI mode (config.mode, for ADS1299: MODE1)
     * 
     * Calls to spiTransfer(), csLow(), csHigh() are valid only between
     * beginTransaction() and endTransaction().
     * 
     * PRECONDITION: end of previous transaction or begin() called.
     * POSTCONDITION: SPI configured and locked (if RTOS aware).
     * 
     * @param config SPI configuration (clock, bit order, mode)
     */
    virtual void beginTransaction(const ADS1299_SpiConfig& config) = 0;

    /**
     * End an SPI transaction.
     * 
     * Must release SPI peripheral (if RTOS aware) to allow other devices.
     * After endTransaction(), the next call must be beginTransaction() or end().
     * 
     * PRECONDITION: beginTransaction() called.
     * POSTCONDITION: SPI unlocked; spiTransfer/cs operations invalid until next beginTransaction().
     */
    virtual void endTransaction() = 0;

    // ========== SPI BYTE TRANSFER ==========

    /**
     * Transfer one SPI byte (send and receive).
     * Blocking operation; waits for SPI hardware completion.
     * 
     * ADS1299 protocol requires full-duplex transfer:
     * - MCU sends command/address/data
     * - MCU simultaneously receives response data
     * 
     * PRECONDITION: beginTransaction() called, CS is as needed.
     * 
     * @param data Byte to send to ADS1299
     * @return Byte received from ADS1299 DOUT pin
     */
    virtual uint8_t spiTransfer(uint8_t data) = 0;

    // ========== CHIP SELECT ==========

    /**
     * Assert chip select (CS low).
     * 
     * ADS1299 is active when CS is low.
     * Must be called before first transfer() in a command sequence.
     * 
     * PRECONDITION: beginTransaction() called.
     */
    virtual void csLow() = 0;

    /**
     * Deassert chip select (CS high).
     * 
     * Called after last transfer() in a command sequence.
     * After csHigh(), must wait ≥ tSDECODE (4 tCLK ≈ 2 µs) before next csLow().
     * 
     * PRECONDITION: beginTransaction() called.
     * POSTCONDITION: ADS1299 latches command; CS high; ready for next command after tSDECODE.
     */
    virtual void csHigh() = 0;

    // ========== TIMING ==========

    /**
     * Delay in microseconds.
     * 
     * Used for protocol timing:
     * - tSDECODE = 4 tCLK (≈1.95 µs at 2.048 MHz) → delayMicroseconds(2)
     * - RESET pulse width (10 µs low, 20 µs high)
     * - Lead-off current switching times
     * 
     * Implementation may be blocking (delay()) or non-blocking (timer-based)
     * depending on platform and RTOS availability.
     * 
     * @param us Microseconds to delay
     */
    virtual void delayMicroseconds(uint32_t us) = 0;

    /**
     * Delay in milliseconds.
     * 
     * Used for longer waits:
     * - Power-up stabilization (5 ms after GPIO config)
     * - Register write settling times
     * 
     * Implementation typically blocking (delay()) on embedded platforms.
     * 
     * @param ms Milliseconds to delay
     */
    virtual void delayMilliseconds(uint32_t ms) = 0;

    // ========== CONTROL SIGNALS ==========

    /**
     * Control START pin (MCU → ADS1299).
     * 
     * START is a digital input to ADS1299:
     * - HIGH: Start continuous conversions
     * - LOW: Stop conversions
     * 
     * PRECONDITION: begin() called, START pin configured as OUTPUT.
     * 
     * @param high true=HIGH, false=LOW
     */
    virtual void setStart(bool high) = 0;

    /**
     * Control RESET pin (MCU → ADS1299).
     * 
     * RESET is active-low input:
     * - Pulsing LOW (~10 µs) → digital reset
     * - Default: held HIGH via external pull-up
     * 
     * PRECONDITION: begin() called, RESET pin configured as OUTPUT.
     * 
     * @param high true=HIGH (normal), false=LOW (resetting)
     */
    virtual void setReset(bool high) = 0;

    /**
     * Control PWDN pin (MCU → ADS1299, optional).
     * 
     * PWDN is active-low input (power down):
     * - LOW: Power-down mode (minimal current draw)
     * - HIGH: Normal operation
     * 
     * NOTE: Many designs tie PWDN directly to power supply (never connected to GPIO).
     * Implementation must check if PWDN pin exists before asserting output.
     * 
     * If PWDN is not available (ADS_PIN_UNUSED), this method returns without action.
     * 
     * PRECONDITION: begin() called.
     * 
     * @param high true=HIGH (normal), false=LOW (power down)
     */
    virtual void setPwdn(bool high) = 0;

    // ========== DATA READY POLLING ==========

    /**
     * Read DRDY pin (ADS1299 → MCU).
     * 
     * DRDY is open-drain, active-low output from ADS1299:
     * - LOW (0): ADC data ready for readout
     * - HIGH (1): No data ready (pulled up via external resistor or INPUT_PULLUP)
     * 
     * Returns logic level as seen by MCU GPIO input.
     * 
     * PRECONDITION: begin() called, DRDY pin configured as INPUT_PULLUP.
     * 
     * @return true if DRDY is HIGH (idle), false if DRDY is LOW (data ready)
     */
    virtual bool readDrdy() = 0;
};

```

### 6.2 Why This Design

**Key Features:**
- **Neutral Types:** `ADS1299_SpiConfig`, `ADS1299_GpioLevel` (not `SPISettings` or platform-specific types)
- **Clear Invariants:** Preconditions/postconditions document safe usage
- **Transaction API:** `beginTransaction()/endTransaction()` encapsulates SPI mode setup
- **Explicit Timing:** All delay operations named explicitly (not hidden in `waitDecode()`)
- **Optional Features:** PWDN pin marked optional (safe to ignore)
- **Platform-Agnostic Naming:** No references to Arduino, ESP-IDF, STM32, Zephyr

**Avoiding Scope Creep:**
- ✅ Includes: synchronous SPI, GPIO, timing
- ❌ Excludes: DMA, interrupts, async, mutexes, RTOS objects

---

## 7. What Should NOT Be Abstracted Yet

To keep the HAL focused and portable, the following are **explicitly out of scope** for Phase B2–B3:

### 7.1 Features Deferred

| Feature | Why | When |
|---------|-----|------|
| **DMA (Direct Memory Access)** | Adds complexity; SPI is byte-driven now; can optimize later | Phase B4+ |
| **Interrupts** | Driver is polling-based; async requires design rethink | Phase B4+ |
| **Mutexes / RTOS Locks** | Orthogonal concern; thread-safety per platform | Phase B4+ |
| **Async Transfers** | Current API is blocking; async requires new return types | Phase B4+ |
| **RTOS Objects (tasks, queues)** | Platform-specific; belongs in application, not HAL | Application layer |
| **Pin Structs (ESP32 GPIO_NUM_*, etc.)** | Platform-specific; HAL takes pin IDs at construction | Platform impl |
| **Devicetree Objects (Zephyr)** | Runtime objects can't be abstracted; bind in platform code | Platform impl |
| **SPIClass / spi_device_t / SPI_HandleTypeDef** | Platform-specific internals; hidden in HAL impl | Platform impl |
| **Calibration / Self-Test** | Device-specific; not in HAL | Driver or application |
| **Power Modes (sleep, standby)** | Hardware-specific; handled by existing cmds | Future driver |

### 7.2 Why These Deferred

**DMA / Interrupts:**
- Current code is polling-based, single-threaded
- Significant API/design changes needed
- Can be added after HAL core is stable

**RTOS / Async:**
- Not all platforms have RTOS
- Blocking API simpler to port
- Async can be layered on top later

**Platform-Specific Objects:**
- Hide complexity in platform implementation
- Core doesn't need to know device handles or devicetree nodes
- Constructor/factory methods bridge the gap

---

## 8. Mapping to Current Code

This section shows how the proposed HAL interface maps to existing Arduino code in ADS1299Plus and ADS1299_SafeSPI.

### 8.1 ADS1299_SafeSPI → HAL

**Current Arduino Code:**
```cpp
// ADS1299_SafeSPI.h
class ADS1299_SafeSPI {
  void begin();
  void end();
  void select();      // → csLow()
  void deselect();    // → csHigh()
  uint8_t xfer(uint8_t data);  // → transfer()
  void waitDecode();  // → part of SPI transaction timing
};

// ADS1299_SafeSPI.cpp
void ADS1299_SafeSPI::begin() {
  pinMode(csPin_, OUTPUT);
  digitalWrite(csPin_, HIGH);
  spi_.begin();
  spi_.beginTransaction(SPISettings(spiHz_, MSBFIRST, SPI_MODE1));
  active_ = true;
}

void ADS1299_SafeSPI::select() {
  digitalWrite(csPin_, LOW);
}

void ADS1299_SafeSPI::deselect() {
  digitalWrite(csPin_, HIGH);
}

uint8_t ADS1299_SafeSPI::xfer(uint8_t data) {
  return spi_.transfer(data);
}
```

**Proposed HAL Mapping:**

| Current Code | Maps To | HAL Method |
|--------------|---------|-----------|
| `begin()` | GPIO + SPI init | `begin()` |
| `end()` | SPI cleanup | `end()` |
| `beginTransaction(SPISettings(...))` | SPI mode setup | `beginTransaction(ADS1299_SpiConfig)` |
| `endTransaction()` | Release SPI | `endTransaction()` |
| `select()` | CS low | `csLow()` |
| `deselect()` | CS high | `csHigh()` |
| `xfer()` | SPI byte transfer | `spiTransfer()` |
| `waitDecode()` | tSDECODE delay | `delayMicroseconds(3)` (caller's responsibility) |

**Phase B2.2 Refactoring (Outline):**
```cpp
// ADS1299_SafeSPI with optional HAL (pseudo-code)
class ADS1299_SafeSPI {
public:
  // Existing Arduino constructor (unchanged)
  explicit ADS1299_SafeSPI(uint8_t csPin, SPIClass& spi, uint32_t spiHz);

  // New HAL constructor (additive)
  explicit ADS1299_SafeSPI(ADS1299_HAL& hal, uint32_t spiHz);

  // Existing public interface (unchanged)
  void begin();
  void end();
  void select();
  void deselect();
  uint8_t xfer(uint8_t data);
  void waitDecode();

private:
  // Dual path: Arduino OR HAL
  bool useHal_;
  ADS1299_HAL* hal_;
  SPIClass* spi_;
  uint32_t spiHz_;
};

// In cpp:
void ADS1299_SafeSPI::begin() {
  if (useHal_) {
    ADS1299_SpiConfig config = {
      .clockHz = spiHz_,
      .bitOrder = ADS1299_SpiBitOrder::MSB_FIRST,
      .mode = ADS1299_SpiMode::MODE1
    };
    hal_->beginTransaction(config);
    hal_->begin();
  } else {
    // Existing Arduino path (unchanged)
    ...
  }
}
```

### 8.2 ADS1299Plus GPIO → HAL

**Current Arduino Code:**
```cpp
// ADS1299Plus.cpp
void ADS1299Plus::pinStartHigh() {
  digitalWrite(pins_.start, HIGH);
}

void ADS1299Plus::pinStartLow() {
  digitalWrite(pins_.start, LOW);
}

void ADS1299Plus::pinResetPulse() {
  digitalWrite(pins_.reset, LOW);
  ads_wait_us(10);
  digitalWrite(pins_.reset, HIGH);
  ads_wait_us(20);
}

void ADS1299Plus::pinPowerDown(bool activeLow) {
  if (pins_.pwdn == ADS_PIN_UNUSED)
    return;
  digitalWrite(pins_.pwdn, activeLow ? LOW : HIGH);
}

bool ADS1299Plus::dataReady() const {
  return digitalRead(pins_.drdy) == LOW;
}

bool ADS1299Plus::begin() {
  pinMode(pins_.cs, OUTPUT);
  digitalWrite(pins_.cs, HIGH);
  pinMode(pins_.drdy, INPUT_PULLUP);
  pinMode(pins_.start, OUTPUT);
  digitalWrite(pins_.start, LOW);
  pinMode(pins_.reset, OUTPUT);
  digitalWrite(pins_.reset, HIGH);
  if (pins_.pwdn != ADS_PIN_UNUSED) {
    pinMode(pins_.pwdn, OUTPUT);
    digitalWrite(pins_.pwdn, HIGH);
  }
  ads_wait_ms(5);
  spi_.begin();
  // ... rest of begin()
}
```

**Proposed HAL Mapping:**

| Current Code | Maps To | HAL Method |
|--------------|---------|-----------|
| `digitalWrite(pins_.start, HIGH)` | Set START HIGH | `setStart(true)` |
| `digitalWrite(pins_.start, LOW)` | Set START LOW | `setStart(false)` |
| `digitalWrite(pins_.reset, LOW/HIGH)` + delays | RESET pulse | `setReset(false); delayMicroseconds(10); setReset(true); delayMicroseconds(20)` |
| `digitalWrite(pins_.pwdn, ...)` (conditional) | Set PWDN | `setPwdn(high)` (HAL handles PIN_UNUSED check) |
| `digitalRead(pins_.drdy) == LOW` | Poll DRDY | `!readDrdy()` (note: readDrdy() returns HIGH/LOW directly) |
| `pinMode(..., OUTPUT/INPUT_PULLUP)` | GPIO config | `begin()` (all pin config in HAL init) |
| `ads_wait_ms(5)` | Millisecond delay | `delayMilliseconds(5)` |

**Phase B2.3 Refactoring (Outline):**
```cpp
// ADS1299Plus with optional HAL (pseudo-code)
class ADS1299Plus {
public:
  // Existing SafeSPI constructor
  ADS1299Plus(ADS1299_SafeSPI& spi, const Pins& pins);

  // New HAL constructor
  ADS1299Plus(ADS1299_HAL& hal, const Pins& pins);

private:
  bool useHal_;
  ADS1299_HAL* hal_;
  ADS1299_SafeSPI* spi_;
  Pins pins_;
};

// In begin() (dual path):
bool ADS1299Plus::begin() {
  if (useHal_) {
    hal_->begin();
    hal_->setPwdn(true);
    hal_->delayMilliseconds(5);
    hal_->csHigh();  // Handled in SafeSPI before, now explicit
    // ... rest same for both
  } else {
    // Existing Arduino path (unchanged)
    pinMode(pins_.cs, OUTPUT);
    digitalWrite(pins_.cs, HIGH);
    // ...
  }
  // ... shared logic (SPI commands, register access)
}
```

---

## 9. Conservative Migration Plan

The migration from Arduino-only to HAL-aware code is split into phases, each with clear validation gates.

### 9.1 Phase Timeline

| Phase | Focus | Code Changes | Risk | Duration | Validation |
|-------|-------|--------------|------|----------|-----------|
| **B2.0** | Design study | None | - | 1 session | Document (this file) |
| **B2.1** | HAL types & SPI methods | Add to HAL | LOW | 1 session | Compile check |
| **B2.2** | SafeSPI dual-path | Add HAL constructor | MEDIUM | 1 session | Both paths compile |
| **B2.3** | ADS1299Plus dual-path | Add HAL constructor | MEDIUM | 1 session | Both paths compile |
| **B2.4** | Example validation | None | LOW | 1 session | Examples work unchanged |
| **B3** | Optional HAL example | New sketch | LOW | 1 session | HAL path works |

### 9.2 Phase B2.1: Extend HAL with SPI Config & Transactions

**Deliverables:**
- `src/hal/ADS1299_HAL.h` — extended interface (sections 6.1 above)
- `src/hal/ADS1299_HAL_Types.h` — neutral types (section 5 above)
- `src/arduino/ADS1299_ArduinoHAL.h` — new methods
- `src/arduino/ADS1299_ArduinoHAL.cpp` — Arduino implementation

**Changes Summary:**
```cpp
// Add to ADS1299_HAL:
virtual void beginTransaction(const ADS1299_SpiConfig& config) = 0;
virtual void endTransaction() = 0;

// Implement in ADS1299_ArduinoHAL:
void beginTransaction(const ADS1299_SpiConfig& config) {
  SPI.beginTransaction(SPISettings(
    config.clockHz,
    (config.bitOrder == ADS1299_SpiBitOrder::MSB_FIRST) ? MSBFIRST : LSBFIRST,
    static_cast<uint8_t>(config.mode)
  ));
}
void endTransaction() {
  SPI.endTransaction();
}
```

**Verification Checklist:**
```
☐ src/ compiles without errors
☐ examples/ compiles without errors
☐ No new warnings
☐ API unchanged (HAL only extended, not modified)
```

---

### 9.3 Phase B2.2: Adapt SafeSPI for HAL

**Deliverables:**
- `src/ADS1299_SafeSPI.h` — dual constructor
- `src/ADS1299_SafeSPI.cpp` — dual-path implementation

**Changes Summary:**
```cpp
// Add to SafeSPI.h:
explicit ADS1299_SafeSPI(ADS1299_HAL& hal, uint32_t spiHz);

// In SafeSPI.cpp, dual-path all methods:
void begin() {
  if (useHal_) {
    // HAL path
    ADS1299_SpiConfig config = { spiHz_, ... };
    hal_->beginTransaction(config);
    hal_->begin();
  } else {
    // Arduino path (existing)
    ...
  }
}
```

**Verification Checklist:**
```
☐ Arduino path still compiles and works (backward compat)
☐ HAL path compiles (new code path)
☐ Both paths exercised in tests
☐ No API changes (only additive constructor)
☐ examples/ still work unchanged
```

---

### 9.4 Phase B2.3: Add HAL Constructor to ADS1299Plus

**Deliverables:**
- `src/ADS1299Plus.h` — HAL constructor
- `src/ADS1299Plus.cpp` — dual-path GPIO operations

**Changes Summary:**
```cpp
// Add to ADS1299Plus.h:
explicit ADS1299Plus(ADS1299_HAL& hal, const Pins& pins);

// In begin(), dual-path GPIO init:
if (useHal_) {
  hal_->begin();
  hal_->setPwdn(true);
  hal_->delayMilliseconds(5);
} else {
  // Arduino path (existing)
  pinMode(...);
  ...
}

// Replace pinXxx() methods with dual-path:
void pinStartHigh() {
  if (useHal_) hal_->setStart(true);
  else digitalWrite(pins_.start, HIGH);
}
```

**Verification Checklist:**
```
☐ Arduino path still works
☐ HAL path compiles
☐ Frame reading works via both paths
☐ examples/ still work unchanged
☐ No API changes (only additive constructor)
```

---

### 9.5 Phase B2.4: Validate Examples

**Scope:** BasicRead, RegisterDump

**Verification Checklist:**
```
☐ BasicRead compiles
☐ BasicRead runs on hardware (device detected, ID read)
☐ BasicRead reads frames correctly (250 SPS, 8 channels)
☐ RegisterDump compiles
☐ RegisterDump runs on hardware (registers readable)
☐ No binary size regression (< 5%)
☐ No new compiler warnings
```

---

### 9.6 Phase B3: Optional HAL Example

**Status:** Deferred (Phase B3+)

**Deliverable:**
- `examples/HalBasedRead/HalBasedRead.ino` — sketch using HAL constructor

**Content Outline:**
```cpp
#include <Arduino.h>
#include <ADS1299Plus.h>
#include <ADS1299_ArduinoHAL.h>

// Use HAL instead of SafeSPI
ADS1299_ArduinoHAL hal(PIN_CS, PIN_START, PIN_RESET, PIN_PWDN, PIN_DRDY);
ADS1299Plus::Pins pins = { ... };
ADS1299Plus ads(hal, pins);  // HAL constructor

void setup() {
  if (!ads.begin()) {
    // error
  }
  // ... same as BasicRead, but ads uses HAL internally
}
```

**Why Deferred:**
- Requires B2.1–B2.4 complete and tested
- Documentation-only (not critical path)
- Added after validation

---

## 10. References

### Hardware Datasheets

**[1] Texas Instruments, "ADS1299-x 8-Channel, 24-Bit, Simultaneously Sampling Data Acquisition System,"**  
      Datasheet Rev. C, November 2013.  
      Section 9.5: SPI protocol (command format, timing).  
      Section 11.1: Startup sequence, power-up timing.  
      Section 9.4.2: DRDY behavior and timing.  
      URL: https://www.ti.com/lit/ds/symlink/ads1299.pdf

### Arduino Ecosystem

**[2] Arduino Project, "Arduino Library Specification,"**  
      Defines library structure (src/, examples/, library.properties).  
      URL: https://arduino.github.io/arduino-cli/0.35/library-specification/

**[3] Arduino Project, "SPI Transaction API,"**  
      Reference for SPISettings and beginTransaction/endTransaction.  
      Describes CPOL, CPHA, clock frequency, bit order.  
      URL: https://www.arduino.cc/reference/en/language/functions/communication/spi/begintransaction/

### Platform-Specific Documentation

**[4] Espressif Systems, "ESP-IDF SPI Master Driver,"**  
      ESP-IDF documentation for SPI master on ESP32.  
      Covers spi_device_handle_t, spi_transaction_t, DMA support.  
      URL: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/spi_master.html

**[5] Zephyr Project, "Zephyr GPIO and SPI Drivers,"**  
      Zephyr RTOS API for GPIO and SPI.  
      Covers devicetree bindings, spi_transceive, configuration flags.  
      URL: https://docs.zephyrproject.org/latest/hardware/peripherals/index.html

**[6] STMicroelectronics, "STM32 HAL User Manual,"**  
      STM32 Hardware Abstraction Layer SPI and GPIO functions.  
      Covers SPI_HandleTypeDef, HAL_SPI_Transmit, HAL_GPIO_WritePin.  
      Reference for future STM32 backend (Phase B3+).  
      URL: https://www.st.com/content/dam/en/search-results-download-dirte/global/datasheets/microcontrollers-microprocessors/stm32f4xx-hal-user-manual.pdf (example; version-specific)

### Design References

**[7] C++ Standards Committee, "ISO/IEC 14882:2020 (C++20),"**  
      Reference for virtual base classes, pure virtual methods, move semantics.  
      Specifically: virtual destructors (C++11), override keyword (C++11).

**[8] MISRA C:2012 Guidelines (Motor Industry Software Reliability Association),**  
      Guidelines for embedded C/C++ safety-critical code.  
      Relevant for driver stability and predictability.

---

## Appendix: Design Decisions Summary

| Decision | Rationale | Alternative Considered |
|----------|-----------|----------------------|
| **Neutral SpiConfig struct** | Avoids platform-specific types in core | Export SPISettings (rejected: Arduino-only) |
| **beginTransaction/endTransaction methods** | Encapsulates SPI mode setup | Implicit in begin() (rejected: inflexible) |
| **Separate delay methods** | Explicit timing control | Implicit in transfer (rejected: opaque) |
| **Optional PWDN handling** | Many boards tie PWDN to power | Mandatory PWDN output (rejected: inflexible) |
| **readDrdy() returns bool** | Natural mapping to pin state | readDrdy() returns gpio_level_t (rejected: overkill) |
| **Defer DMA/interrupts** | Complexity vs. benefit | Implement now (rejected: scope creep) |
| **Dual-path SafeSPI/ADS1299Plus** | Gradual migration, backward compat | Full refactor (rejected: risky) |

---

**End of HAL Design References — Phase B2.0**
