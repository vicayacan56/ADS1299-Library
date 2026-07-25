# Phase C7 - Hardware Smoke Test

This document records the first real-hardware smoke test of ADS1299Plus after the portable HAL work.

## Hardware

- Controller board: Arduino UNO Q
- Arduino core: `arduino:zephyr`
- FQBN: `arduino:zephyr:unoq`
- Port used during test: `COM7`
- Front-end device detected: ADS1299-4
- Device ID read: `0x3C`

## Wiring Used

The UNO Q wiring matched the example defaults:

| UNO Q pin | Front-end signal |
| --- | --- |
| D13 | FE_SCLK |
| D12 | FE_MISO |
| D11 | FE_MOSI |
| D10 | FE_CS |
| D9 | FE_START |
| D8 | FE_RESET |
| D7 | FE_DRDY |
| GND | GND |

PWDN was not controlled by a GPIO in the tested sketch configuration and was treated as unused by the library.

## Tooling

Arduino CLI was used from the Windows PC:

```powershell
arduino-cli core install arduino:zephyr
arduino-cli compile --fqbn arduino:zephyr:unoq examples\RegisterDump
arduino-cli compile --fqbn arduino:zephyr:unoq examples\BasicRead
arduino-cli compile --fqbn arduino:zephyr:unoq examples\HalBasedRead
```

Uploads used:

```powershell
arduino-cli upload -p COM7 --fqbn arduino:zephyr:unoq examples\RegisterDump
arduino-cli upload -p COM7 --fqbn arduino:zephyr:unoq examples\BasicRead
arduino-cli upload -p COM7 --fqbn arduino:zephyr:unoq examples\HalBasedRead
```

Serial monitor used:

```powershell
arduino-cli monitor -p COM7 -c baudrate=115200
```

## RegisterDump Result

`RegisterDump` compiled, uploaded, and ran.

Observed output included:

```text
ADS1299Plus RegisterDump
ADS1299 ID = 0x3C
Detected channels = 4
Bytes per frame = 15
```

Register dump after `configureDefaults()` included:

```text
CONFIG1 = 0x96
CONFIG2 = 0xC0
CONFIG3 = 0xE8
LOFF    = 0x66
CH1SET  = 0x60
CH2SET  = 0x60
CH3SET  = 0x60
CH4SET  = 0x60
GPIO    = 0x0F
CONFIG4 = 0x00
```

This confirms real SPI register communication and ADS1299-4 detection.

## BasicRead Result

`BasicRead` compiled, uploaded, and ran using the classic `ADS1299_SafeSPI` path.

Observed output included:

```text
ADS1299Plus BasicRead
ADS1299 ID = 0x3C
Detected channels = 4
RDATAC started
STATUS=0xC00000 CH1=... CH2=... CH3=... CH4=...
```

No persistent `invalid frame or sync mismatch` failure was observed in the provided output.

This confirms RDATAC acquisition through the classic Arduino/SafeSPI path on UNO Q.

## HalBasedRead Result

`HalBasedRead` compiled, uploaded, and ran using the optional HAL-backed path:

```text
ADS1299Plus -> ADS1299_ArduinoHAL -> ADS1299_Protocol -> ADS1299
```

Observed output included repeated valid frames:

```text
STATUS=0xC00000 CH1=... CH2=... CH3=... CH4=...
```

This confirms that the Arduino HAL backend works on real UNO Q hardware with a real ADS1299-4 front end.

## Notes

UNO Q uploads printed OpenOCD verify warnings such as:

```text
Error: verify failed in bank at 0x08000000 starting at 0x00100000
```

The uploaded sketches still executed and produced valid serial output. This appears to be an UNO Q/OpenOCD upload-tool warning in this setup rather than an ADS1299Plus runtime failure.

The channel values were not evaluated as EEG-quality measurements in this phase. Inputs may be floating or not connected to a controlled signal source. C7 validates communication, register access, frame transport, and HAL path execution.

## C7 Outcome

C7 passed for the tested hardware path:

- Arduino UNO Q
- `arduino:zephyr:unoq`
- ADS1299-4
- `RegisterDump`
- `BasicRead`
- `HalBasedRead`

The library has now been validated beyond compilation for:

- real ADS1299 SPI register access;
- default register configuration;
- RDATAC frame acquisition;
- classic Arduino/SafeSPI path;
- optional Arduino HAL-backed path.

## Remaining Validation

Still recommended before a polished release:

- longer acquisition run;
- controlled input or test signal capture;
- repeat on another Arduino-compatible board if available;
- document any required UNO Q upload workaround if OpenOCD warnings persist;
- decide release version and release notes.
