# Testing without hardware

Without a connected ADS1299, you can validate installation and compilation
compatibility, but not communication or acquisition.

## Available checks

From the repository root:

```powershell
arduino-cli version
arduino-cli core list
arduino-cli compile --fqbn arduino:avr:uno .\examples\RegisterDump
arduino-cli compile --fqbn arduino:avr:uno .\examples\BasicRead
```

A successful compilation confirms that:

- Arduino finds `ADS1299Plus.h`, `ADS1299_SafeSPI.h`, and their sources;
- the library structure and metadata are valid;
- the examples use an available API;
- the selected core provides Arduino and SPI.

You can also check that Arduino CLI sees only one installation:

```powershell
arduino-cli lib list | Select-String ADS1299Plus
```

## Checks that require real hardware

You need a powered and wired ADS1299 to validate:

- SPI communication and a correct `ID` register response;
- detection of 4, 6, or 8 channels;
- actual register reads and writes;
- `DRDY` transitions;
- `RDATAC` or `RDATA` frame integrity and synchronization;
- noise, stability, sample rate, and channel values;
- RESET, START, and PWDN behavior;
- reference, BIAS, lead-off, and analog inputs.

Compilation does not prove that pins, voltages, clock, isolation, or analog
signals are correct. Use `RegisterDump` for the first hardware test, followed
by `BasicRead`.
