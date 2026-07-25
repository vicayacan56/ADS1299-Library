# Configuration guide

This guide shows where and how to change the ADS1299 configuration without
editing the library source code.

## Correct configuration order

For initial setup:

```cpp
if (!ads.begin()) {
  Serial.println("ERROR: begin() failed");
  while (true) delay(1000);
}

if (!ads.configureDefaults()) {
  Serial.println("ERROR: configureDefaults() failed");
  while (true) delay(1000);
}

// Apply custom settings here.

ads.pinStartHigh();
delay(10);
ads.cmdRDATAC();
```

Always follow these rules:

1. Apply custom settings after `configureDefaults()`.
2. Apply them before `pinStartHigh()`, `cmdStart()`, or `cmdRDATAC()`.
3. Check every returned `bool`.
4. Never read or write registers while `RDATAC` is active.
5. Use `channelCount()` instead of assuming that eight channels exist.

Calling `configureDefaults()` after custom settings will overwrite those
settings and restore the default 250 SPS, gain 24 configuration.

## Changing the sample rate

Call `setDataRate()` with one of the named data-rate constants:

| Desired sample rate | Constant |
|---:|---|
| 16,000 SPS | `ADS_DR_16k` |
| 8,000 SPS | `ADS_DR_8k` |
| 4,000 SPS | `ADS_DR_4k` |
| 2,000 SPS | `ADS_DR_2k` |
| 1,000 SPS | `ADS_DR_1k` |
| 500 SPS | `ADS_DR_500` |
| 250 SPS | `ADS_DR_250` |

Example for 500 SPS:

```cpp
if (!ads.setDataRate(ADS_DR_500)) {
  Serial.println("ERROR: could not set 500 SPS");
  while (true) delay(1000);
}
```

Use `ADS_DR_500`, not `500`. The function expects the ADS1299 register field,
and the named constant supplies the correct value.

These rates assume the nominal 2.048 MHz ADS1299 master clock. They scale if a
different master clock is used. At high rates, printing every sample through
`Serial` can be slower than acquisition. Buffer, decimate, or use a faster data
transport instead of treating missing Serial lines as an ADC failure.

## Changing gain

Available programmable gain constants are:

| Gain | Constant |
|---:|---|
| 1 | `ADS_GAIN_1` |
| 2 | `ADS_GAIN_2` |
| 4 | `ADS_GAIN_4` |
| 6 | `ADS_GAIN_6` |
| 8 | `ADS_GAIN_8` |
| 12 | `ADS_GAIN_12` |
| 24 | `ADS_GAIN_24` |

A higher gain makes smaller differential inputs use more of the ADC range, but
large inputs saturate sooner. Select the gain for the expected signal amplitude
and analog front-end.

Change one channel:

```cpp
if (!ads.setChannelGain(1, ADS_GAIN_12)) {
  Serial.println("ERROR: could not configure CH1 gain");
}
```

Change every channel detected by `begin()`:

```cpp
for (uint8_t ch = 1; ch <= ads.channelCount(); ++ch) {
  if (!ads.setChannelGain(ch, ADS_GAIN_12)) {
    Serial.print("ERROR: gain configuration failed on CH");
    Serial.println(ch);
    while (true) delay(1000);
  }
}
```

Channel numbering starts at 1. Do not call `setChannelGain(0, ...)`.

## Complete sample-rate and gain example

In `examples/BasicRead/BasicRead.ino`, place this block immediately after the
successful `configureDefaults()` call:

```cpp
static constexpr uint8_t DATA_RATE = ADS_DR_1k;
static constexpr uint8_t CHANNEL_GAIN = ADS_GAIN_6;

if (!ads.setDataRate(DATA_RATE)) {
  Serial.println("ERROR: setDataRate() failed");
  while (true) delay(1000);
}

for (uint8_t ch = 1; ch <= ads.channelCount(); ++ch) {
  if (!ads.setChannelGain(ch, CHANNEL_GAIN)) {
    Serial.println("ERROR: setChannelGain() failed");
    while (true) delay(1000);
  }
}
```

To try another setting, change only the two constants:

```cpp
static constexpr uint8_t DATA_RATE = ADS_DR_500;
static constexpr uint8_t CHANNEL_GAIN = ADS_GAIN_12;
```

Do not change `ADS1299Plus.cpp`, `ADS1299Plus.h`, or the register constants.

## Channel input and power settings

Set the input multiplexer for one channel:

```cpp
ads.setChannelMux(1, ADS_MUX_NORMAL);  // Normal differential input
ads.setChannelMux(1, ADS_MUX_SHORT);   // Inputs internally shorted for testing
ads.setChannelMux(1, ADS_MUX_TESTSIG); // Internal test signal
ads.setChannelMux(1, ADS_MUX_TEMP);    // Internal temperature sensor
```

Other available MUX constants are `ADS_MUX_BIAS_MEAS`, `ADS_MUX_MVDD`,
`ADS_MUX_BIASP`, and `ADS_MUX_BIASN`. Consult the ADS1299 datasheet before
using specialized MUX modes.

Power down or restore a channel:

```cpp
ads.powerDownChannel(4, true);  // Power down CH4
ads.powerDownChannel(4, false); // Enable CH4
```

Write a complete `CHnSET` value only when you deliberately want to set power,
gain, MUX, and SRB2 together:

```cpp
uint8_t ch1 = ADS_CH_MAKE(true, ADS_GAIN_6, ADS_MUX_NORMAL, false);
ads.setChannel(1, ch1);
```

For ordinary changes, prefer `setChannelGain()`, `setChannelMux()`,
`powerDownChannel()`, and `setSRB2()` because they preserve the other channel
fields.

## SRB, reference, and BIAS

Common helpers include:

```cpp
ads.setSRB2(1, true);        // Connect CH1 positive input to SRB2
ads.enableSRB1(true);        // Enable the common SRB1 connection
ads.useInternalRef(true);    // Enable the internal reference buffer
ads.useBiasInternalRef(true);
ads.enableBiasBuffer(true);
ads.setBiasDeriveP(0x0F);    // Positive inputs CH1..CH4
ads.setBiasDeriveN(0x0F);    // Negative inputs CH1..CH4
```

BIAS and SRB routing affect the analog signal path. Do not copy a configuration
without confirming that it matches your electrodes, board schematic, and
safety design. Channel masks use bit 0 for CH1, bit 1 for CH2, and so on. The
library automatically removes mask bits for channels absent from an ADS1299-4
or ADS1299-6.

## Lead-off detection

The default configuration loads lead-off settings but leaves the lead-off
comparators disabled. A typical configuration sequence is:

```cpp
uint8_t activeChannels =
  (uint8_t)((1u << ads.channelCount()) - 1u);

if (!ads.configureLeadOff(ADS_LOFF_AC_24NA_31HZ_80PCT) ||
    !ads.enableLeadOffSenseP(activeChannels) ||
    !ads.enableLeadOffSenseN(activeChannels) ||
    !ads.enableLoffComparators(true)) {
  Serial.println("ERROR: lead-off configuration failed");
}
```

Lead-off current, frequency, thresholds, electrode routing, and patient safety
must be evaluated for the actual hardware and application.

## Clock output and conversion mode

```cpp
ads.setClockOut(true);                // Drive CLK_OUT
ads.setMultipleReadbackMode(true);    // Set CONFIG1.DAISY_EN
ads.setSingleShot(true);              // Select single-shot mode
```

Only enable these options when the board wiring and acquisition logic require
them. `setMultipleReadbackMode()` controls the ADS1299 `DAISY_EN` bit; consult
the datasheet when using multiple devices.

## Changing settings while running

Stop continuous output and conversions before changing registers. When using
the same pin-controlled start method as `BasicRead`:

```cpp
ads.cmdSDATAC();
ads.pinStartLow();

bool ok = ads.setDataRate(ADS_DR_500);
for (uint8_t ch = 1; ok && ch <= ads.channelCount(); ++ch) {
  ok = ads.setChannelGain(ch, ADS_GAIN_12);
}

if (!ok) {
  Serial.println("ERROR: runtime reconfiguration failed");
  while (true) delay(1000);
}

ads.pinStartHigh();
delay(10);
ads.cmdRDATAC();
```

When conversions were started with `cmdStart()`, use `cmdSDATAC()` followed by
`cmdStop()`, apply the settings, then call `cmdStart()` and `cmdRDATAC()`.

Discard the first frame after restarting if your application requires a clean
boundary between configurations.

## Verifying the applied settings

Every helper returns `true` when the register transaction could be performed.
To verify the value read back from the device:

```cpp
uint8_t config1 = 0;
uint8_t ch1set = 0;

if (ads.readReg(ADS_REG_CONFIG1, config1) &&
    ads.readReg(ADS_REG_CH1SET, ch1set)) {
  Serial.print("CONFIG1=0x");
  Serial.println(config1, HEX);
  Serial.print("CH1SET=0x");
  Serial.println(ch1set, HEX);
}
```

Perform readback before starting `RDATAC`, or stop `RDATAC` first. To inspect
custom settings with `RegisterDump`, add the same custom configuration block
after its `configureDefaults()` call; otherwise it will display only the
default configuration.

## Direct register access

Use `readReg()`, `writeReg()`, `readRegs()`, and `writeRegs()` for settings not
covered by a helper. This is an advanced path:

```cpp
ads.cmdSDATAC();
if (!ads.writeReg(ADS_REG_CONFIG2, customConfig2)) {
  Serial.println("ERROR: register write failed");
}
```

Preserve reserved bits exactly as specified in the ADS1299 datasheet. Prefer
the high-level helpers whenever one is available.
