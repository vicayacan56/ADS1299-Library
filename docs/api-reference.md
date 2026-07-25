# API reference

Include the library with:

```cpp
#include <ADS1299Plus.h>
#include <ADS1299_SafeSPI.h>
```

## Classes and files

`ADS1299Plus` represents the ADC and provides the startup, configuration, and
acquisition API. `ADS1299_SafeSPI` adapts that API to Arduino SPI, controls
`CS`, and uses 2 MHz, MSB first, and SPI mode 1 by default.

`ADS1299_Registers.h` declares opcodes, addresses, masks, gains, input modes,
and configuration constants. It is included by `ADS1299Plus.h`; include it
directly when you need its names to build register values.

## Startup and device information

- `begin()`: configures GPIO and SPI, resets the ADC, exits `RDATAC`, stops
  conversions, validates the `ID`, and detects the variant. It does not apply
  the default configuration or start acquisition.
- `configureDefaults()`: stops acquisition and writes the baseline described in
  the [user guide](user-guide.md).
- `end()`: stops conversions, exits `RDATAC`, and ends the SPI transport.
- `readDeviceID(id)`: reads the `ID` register.
- `channelCount()`: returns 4, 6, or 8 after a successful `begin()`.
- `bytesPerFrame()`: returns `3 + 3 * channelCount()`.
- `dataReady()`: returns `true` while `DRDY` is low.

## ADS1299 commands

- `cmdWakeup()` / `cmdStandby()`: leave or enter standby.
- `cmdReset()`: sends the SPI RESET command.
- `cmdStart()` / `cmdStop()`: start or stop conversions by command.
- `cmdRDATAC()`: enables continuous reading and updates the internal state.
- `cmdSDATAC()`: stops continuous reading; call it before register access.
- `cmdRDATA()`: requests one frame. It is normally used indirectly through
  `readDataOnDemand()`.

Pin helpers are also available: `pinStartHigh()`, `pinStartLow()`,
`pinResetPulse()`, and `pinPowerDown()`. The last function does nothing when
`PWDN` was declared as `ADS_PIN_UNUSED`.

## Register access

These functions return `false` if the address range is invalid or `RDATAC` is
active:

```cpp
bool writeReg(uint8_t addr, uint8_t value);
bool readReg(uint8_t addr, uint8_t& value);
bool writeRegs(uint8_t startAddr, const uint8_t* data, size_t n);
bool readRegs(uint8_t startAddr, uint8_t* data, size_t n);
```

Example:

```cpp
ads.cmdSDATAC();
uint8_t config1;
if (ads.readReg(ADS_REG_CONFIG1, config1)) {
  // Use config1.
}
```

## Configuration

The [configuration guide](configuration-guide.md) provides complete examples,
available constants, and the correct order for applying these functions.

- Clock and data rate: `setDataRate()`, `setClockOut()`, and
  `setMultipleReadbackMode()`.
- Channels: `setChannel()`, `powerDownChannel()`, `setChannelGain()`,
  `setChannelMux()`, and `setSRB2()`.
- Common reference: `enableSRB1()`.
- Reference and BIAS: `useInternalRef()`, `useBiasInternalRef()`,
  `enableBiasBuffer()`, `routeBiasSense()`, and `enableBiasMeasure()`.
- BIAS derivation: `setBiasDeriveP()` and `setBiasDeriveN()`.
- Lead-off: `configureLeadOff()`, `enableLeadOffSenseP()`,
  `enableLeadOffSenseN()`, `setLeadOffFlip()`, and
  `enableLoffComparators()`.
- Conversion mode: `setSingleShot()`.

Valid channel numbers range from 1 through `channelCount()`. Functions that
accept channel masks ignore bits for channels that do not exist on the
detected variant. Configure registers only while `RDATAC` is stopped.

## Acquisition

```cpp
bool readFrameRDATAC(
  uint32_t& status, int32_t* channels, size_t capacity
);
bool readDataOnDemand(
  uint32_t& status, int32_t* channels, size_t capacity
);
```

`readFrameRDATAC()` requires active `RDATAC`. `readDataOnDemand()` requires
inactive `RDATAC` and sends `RDATA`. Both require a capacity equal to or
greater than `channelCount()`, decode channels to `int32_t`, and return `false`
if the `STATUS` synchronization pattern is invalid.

With an eight-element array, you can also use the overloads without
`capacity`:

```cpp
int32_t channels[ADS1299Plus::MAX_CHANNELS];
uint32_t status;
bool ok = ads.readFrameRDATAC(status, channels);
```

`STATUS` helpers: `statusHasSync()`, `statusLoffP()`, `statusLoffN()`, and
`statusGPIO()`. `unpack24()` converts three signed MSB-first bytes to
`int32_t`.
