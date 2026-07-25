# Troubleshooting

## `ADS1299Plus.h: No such file or directory`

- Install the ZIP from Arduino IDE or place the complete folder inside the
  sketchbook `libraries` directory.
- Confirm that `library.properties` and `src` are at the root of that folder,
  without an extra nested directory.
- Restart Arduino IDE after a manual installation.
- Do not copy only the `.ino` file.

## `begin()` returns `false`

`begin()` fails when it cannot read an ADS1299-family ID or when the variant
does not report 4, 6, or 8 channels.

- Verify power, common ground, ADC clock, and logic levels.
- Check `CS`, SCK, MOSI, MISO, and RESET.
- Confirm that `PWDN` is high. If it is tied to `VDD`, use
  `ADS1299Plus::ADS_PIN_UNUSED`.
- Ensure that no other device is holding the bus or `CS`.
- Run `RegisterDump` after adapting the pins to your board.

## Incorrect or changing ID

A value of `0x00`, `0xFF`, or one that changes between reads usually indicates
no response, a floating MISO line, incorrect `CS`, unstable power or clock, or
incompatible logic levels. The chip revision may change bits in the complete
byte; validate the device family and channel count rather than only a copied
hexadecimal constant.

## Unstable frames or synchronization errors

- Confirm that `STATUS` begins with the `0xC` pattern.
- Read exactly `bytesPerFrame()` bytes for each `DRDY` event.
- Do not read or write registers during `RDATAC`.
- Avoid printing over Serial so quickly that samples are missed. For
  diagnostics, reduce output or buffer the data.
- Check ground, power, decoupling, SPI wiring length, and clock.
- Make sure the selected acquisition mode is started only once.

## `DRDY` does not change

- Confirm that the ADC is not in standby or power-down.
- Check `START`: `BasicRead` holds it high before `cmdRDATAC()`.
- Verify RESET, the master clock, configuration, and the actual GPIO assigned
  to `DRDY`.
- Remember that `DRDY` is active low.

## Compilation errors after installation

- Select the correct board and core in Arduino IDE, or use the correct FQBN
  with Arduino CLI.
- Confirm that the core provides `Arduino.h` and `SPI.h`.
- Remove extra directory levels created while extracting the ZIP.
- Enable verbose compilation output to see which library copy Arduino selects.

## Duplicate library installations

Arduino IDE may select the wrong copy if the library exists in multiple
folders. Review any `Multiple libraries were found` compiler message and keep
only one ADS1299Plus installation. With Arduino CLI:

```powershell
arduino-cli lib list | Select-String ADS1299Plus
```

After removing the old copy, restart the IDE and compile `RegisterDump` again.
