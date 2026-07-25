# User guide

## Supported devices

ADS1299Plus supports all three ADS1299 family variants:

| Device | Detected channels | Bytes per frame |
|---|---:|---:|
| ADS1299-4 | 4 | 15 |
| ADS1299-6 | 6 | 21 |
| ADS1299 | 8 | 27 |

`begin()` reads the `ID` register and automatically selects the channel count.
Always allocate an `ADS1299Plus::MAX_CHANNELS` array and process only the
elements reported by `channelCount()`.

## Installation

In Arduino IDE, download the repository ZIP and select **Sketch > Include
Library > Add .ZIP Library...**. Then open
**File > Examples > ADS1299Plus > RegisterDump**.

With Arduino CLI, install the same ZIP and compile the examples:

```powershell
arduino-cli lib install --zip-path .\ADS1299-Library-main.zip
arduino-cli compile --fqbn arduino:avr:uno .\examples\RegisterDump
arduino-cli compile --fqbn arduino:avr:uno .\examples\BasicRead
```

For a manual installation, `library.properties`, `src`, and `examples` must be
together in one folder inside the sketchbook `libraries` directory.

## Digital pins and wiring

Assign suitable GPIOs for `CS`, `DRDY`, `START`, and `RESET`. Use the hardware
SPI peripheral pins for `SCK`, `MOSI`, and `MISO`. `PWDN` may be connected to a
GPIO or tied directly to `VDD`.

```cpp
static constexpr uint8_t PIN_CS    = 10;
static constexpr uint8_t PIN_DRDY  = 7;
static constexpr uint8_t PIN_START = 9;
static constexpr uint8_t PIN_RESET = 8;
static constexpr uint8_t PIN_PWDN  = ADS1299Plus::ADS_PIN_UNUSED; // PWDN to VDD
```

The `ADS1299Plus::Pins` field order is:

```cpp
ADS1299Plus::Pins pins = {
  PIN_CS, SCK, MOSI, MISO, PIN_DRDY, PIN_START, PIN_RESET, PIN_PWDN
};
```

Do not assume that an ADS1299 board accepts the logic levels of every Arduino
board. Check power, ground, clock, logic levels, and decoupling against the
ADS1299 datasheet and your module schematic.

## Bring-up procedure

1. Compile `RegisterDump` before connecting the ADC.
2. Disconnect power and wire the hardware.
3. Power the system and open the Serial Monitor at 115200 baud.
4. Confirm that `begin()` succeeds, the ID belongs to an ADS1299, and
   `channelCount()` returns 4, 6, or 8.
5. Review the register dump produced after `configureDefaults()`.
6. Compile and run `BasicRead`.
7. Confirm that `DRDY` goes low and frames are printed without synchronization
   errors.

## What `configureDefaults()` does

The function stops `RDATAC` and conversions before writing registers. It then
applies a conservative baseline:

- 250 samples per second;
- internal reference enabled and internal test signal disabled;
- active channels, normal differential input, gain 24, and `SRB2` disabled;
- BIAS drive disabled;
- lead-off settings loaded, with lead-off comparators disabled;
- ADS1299 GPIO pins configured as inputs;
- continuous conversion mode.

The function does not start acquisition. Apply any custom settings with the
appropriate helpers while `RDATAC` is stopped.

## Customizing the configuration

The simplest safe order is:

1. Call `begin()`.
2. Call `configureDefaults()` to establish a known baseline.
3. Apply custom settings such as sample rate and channel gain.
4. Check the return value of every configuration call.
5. Start conversions and then enable `RDATAC`.

Custom settings must come after `configureDefaults()`, because calling
`configureDefaults()` later restores the baseline and overwrites them. In
`BasicRead`, insert the custom configuration immediately after this block:

```cpp
if (!ads.configureDefaults()) {
  Serial.println("ERROR: configureDefaults() failed");
  while (true) delay(1000);
}

// Add custom settings here, before pinStartHigh() and cmdRDATAC().
```

For example:

```cpp
if (!ads.setDataRate(ADS_DR_1k)) {
  Serial.println("ERROR: setDataRate() failed");
  while (true) delay(1000);
}

for (uint8_t ch = 1; ch <= ads.channelCount(); ++ch) {
  if (!ads.setChannelGain(ch, ADS_GAIN_6)) {
    Serial.println("ERROR: setChannelGain() failed");
    while (true) delay(1000);
  }
}
```

This example changes all active channels to 1000 SPS and gain 6. Use named
constants rather than raw numbers. See the
[configuration guide](configuration-guide.md) for the complete list and
examples for changing one channel or other device settings.

## Starting and stopping acquisition

`BasicRead` holds the `START` pin high and enables continuous output:

```cpp
ads.pinStartHigh();
delay(10);
ads.cmdRDATAC();
```

Alternatively, while the `START` pin is low, use `cmdStart()` to send the START
command. `cmdRDATAC()` enables continuous frame output. Call `cmdSDATAC()`
before reading or writing registers. To stop conversions, also call `cmdStop()`
or drive the `START` pin low, depending on how conversion was started.

For an on-demand sample, keep `RDATAC` disabled and use:

```cpp
uint32_t status;
int32_t channels[ADS1299Plus::MAX_CHANNELS];
bool ok = ads.readDataOnDemand(
  status, channels, ADS1299Plus::MAX_CHANNELS
);
```

## Interpreting results

- **ID:** `begin()` validates the bits that identify the ADS1299 family and
  uses the variant bits to detect 4, 6, or 8 channels. The complete byte may
  contain revision bits, so do not compare it only with one fixed hexadecimal
  value.
- **`channelCount()`:** the detected physical channel count.
- **`bytesPerFrame()`:** 3 `STATUS` bytes plus 3 bytes per channel.
- **`STATUS`:** a 24-bit word. Its upper nibble must be `0xC`; the remaining
  bits contain lead-off and GPIO status. An invalid synchronization pattern
  makes the frame-reading function return `false`.
- **Channels:** every signed 24-bit sample is sign-extended and returned as an
  `int32_t`. This is a raw ADC code, not a voltage. Process only
  `channelCount()` entries.

See the [API reference](api-reference.md) for register, channel, reference,
BIAS, and lead-off configuration.
