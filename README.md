# ADS1299Plus

Arduino library for controlling the Texas Instruments **ADS1299-4**,
**ADS1299-6**, and **ADS1299** (8-channel) 24-bit biopotential ADCs over SPI.

The library detects the connected variant, provides register and channel
configuration, and acquires frames in continuous (`RDATAC`) or on-demand
(`RDATA`) mode.

> **Safety:** the ADS1299 is commonly used in equipment connected to
> electrodes. Never connect a person to a setup that is powered by or linked to
> non-isolated equipment without the isolation and protection required for
> biomedical applications. This library does not make a prototype a medical
> device.

## Installation

### Arduino IDE

1. Download this repository as a ZIP file from GitHub.
2. In Arduino IDE, select **Sketch > Include Library > Add .ZIP Library...**.
3. Select the ZIP file. Restart the IDE if the examples do not appear.
4. Open **File > Examples > ADS1299Plus > RegisterDump**.

For a manual installation, place the folder containing `library.properties`
inside `Documents\Arduino\libraries\ADS1299Plus`.

### Arduino CLI

From PowerShell, with the ZIP file already downloaded:

```powershell
arduino-cli lib install --zip-path .\ADS1299-Library-main.zip
arduino-cli core update-index
arduino-cli core install arduino:avr
arduino-cli compile --fqbn arduino:avr:uno .\examples\RegisterDump
arduino-cli compile --fqbn arduino:avr:uno .\examples\BasicRead
```

Run the last two commands from a repository checkout. Change the FQBN and pin
assignments for your board.

## Wiring

| ADS1299 signal | Direction | Connection |
|---|---|---|
| `CS` | MCU → ADS1299 | GPIO selected as chip select |
| `SCLK` | MCU → ADS1299 | Hardware SPI SCK pin |
| `DIN` / `MOSI` | MCU → ADS1299 | Hardware SPI MOSI pin |
| `DOUT` / `MISO` | ADS1299 → MCU | Hardware SPI MISO pin |
| `DRDY` | ADS1299 → MCU | Input GPIO; active low |
| `START` | MCU → ADS1299 | Output GPIO |
| `RESET` | MCU → ADS1299 | Output GPIO; active low |
| `PWDN` | MCU → ADS1299 | Optional output GPIO; active low |

Connect the digital grounds and verify that the logic levels, power supplies,
clock, and decoupling meet the ADS1299 datasheet and your board design. `SCK`,
`MOSI`, and `MISO` are the hardware SPI pins of the selected Arduino board.

If `PWDN` is tied directly to `VDD`, do not configure it as a GPIO:

```cpp
static constexpr uint8_t PIN_PWDN = ADS1299Plus::ADS_PIN_UNUSED;
```

## Recommended first run

1. Install the library.
2. Compile `RegisterDump`.
3. Wire the hardware while power is disconnected.
4. Run `RegisterDump` and open the Serial Monitor at 115200 baud.
5. Confirm that an ADS1299 ID is read and 4, 6, or 8 channels are detected.
6. Set the same pins in `BasicRead`, compile it, and run it.
7. Confirm that `DRDY` produces readings and stable `RDATAC` frames appear.

`RegisterDump` initializes the device, applies the default configuration, and
prints the main registers without starting continuous acquisition. `BasicRead`
configures the device, starts `RDATAC`, and prints `STATUS` and the channels as
`int32_t` values.

## Changing sample rate and gain

Apply custom settings **after** `configureDefaults()` and **before** starting
`RDATAC`. For example, this selects 500 SPS and gain 12 on every detected
channel:

```cpp
if (!ads.configureDefaults()) {
  while (true) {}
}

if (!ads.setDataRate(ADS_DR_500)) {
  while (true) {}
}

for (uint8_t ch = 1; ch <= ads.channelCount(); ++ch) {
  if (!ads.setChannelGain(ch, ADS_GAIN_12)) {
    while (true) {}
  }
}

ads.pinStartHigh();
delay(10);
ads.cmdRDATAC();
```

Do not pass a number such as `500` or `12` directly. Use the named constants
`ADS_DR_500` and `ADS_GAIN_12`. See the
[configuration guide](docs/configuration-guide.md) for every supported sample
rate and gain, per-channel settings, MUX, SRB, reference, BIAS, lead-off, and
safe runtime reconfiguration.

## Minimal example

```cpp
#include <ADS1299Plus.h>
#include <ADS1299_SafeSPI.h>

ADS1299_SafeSPI transport(10);
ADS1299Plus::Pins pins = {
  10, SCK, MOSI, MISO, 7, 9, 8, ADS1299Plus::ADS_PIN_UNUSED
};
ADS1299Plus ads(transport, pins);

void setup() {
  if (!ads.begin() || !ads.configureDefaults()) {
    while (true) {}
  }
  ads.pinStartHigh();
  ads.cmdRDATAC();
}

void loop() {
  if (!ads.dataReady()) return;

  uint32_t status;
  int32_t channels[ADS1299Plus::MAX_CHANNELS];
  if (ads.readFrameRDATAC(status, channels,
                          ADS1299Plus::MAX_CHANNELS)) {
    // Process channels[0] ... channels[ads.channelCount() - 1] only.
  }
}
```

## Main files

- `src/ADS1299Plus.h` and `.cpp`: device API, startup sequence, register access,
  channel configuration, and acquisition.
- `src/ADS1299_SafeSPI.h` and `.cpp`: Arduino SPI transport, `CS` control,
  SPI mode 1, and command timing.
- `src/ADS1299_Registers.h`: commands, addresses, masks, and register-map
  constants.
- `examples/RegisterDump`: initial diagnostics and register inspection.
- `examples/BasicRead`: minimal continuous acquisition using `RDATAC`.

## Documentation

- [Documentation index](docs/README.md)
- [User guide](docs/user-guide.md)
- [Configuration guide](docs/configuration-guide.md)
- [API reference](docs/api-reference.md)
- [Testing without hardware](docs/testing-without-hardware.md)
- [Troubleshooting](docs/troubleshooting.md)

## License

ADS1299Plus is released under the [MIT License](LICENSE).
