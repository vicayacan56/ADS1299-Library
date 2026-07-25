# Referencia de API

Incluye la librería con:

```cpp
#include <ADS1299Plus.h>
#include <ADS1299_SafeSPI.h>
```

## Clases y archivos

`ADS1299Plus` representa el ADC y contiene la API de inicio, configuración y
adquisición. `ADS1299_SafeSPI` adapta esa API al SPI de Arduino, controla `CS` y
usa 2 MHz, MSB primero y modo SPI 1 de forma predeterminada.

`ADS1299_Registers.h` declara opcodes, direcciones, máscaras, ganancias, modos
de entrada y constantes de configuración. Se incluye desde `ADS1299Plus.h`;
puedes incluirlo directamente si necesitas sus nombres al construir valores de
registro.

## Inicio e información

- `begin()`: configura GPIO y SPI, reinicia el ADC, sale de `RDATAC`, detiene
  conversiones, valida `ID` y detecta la variante. No aplica los defaults ni
  inicia adquisición.
- `configureDefaults()`: detiene adquisición y escribe la configuración base
  descrita en la [guía de usuario](user-guide.md).
- `end()`: detiene conversiones, sale de `RDATAC` y finaliza el transporte SPI.
- `readDeviceID(id)`: lee el registro `ID`.
- `channelCount()`: devuelve 4, 6 u 8 después de un `begin()` correcto.
- `bytesPerFrame()`: devuelve `3 + 3 * channelCount()`.
- `dataReady()`: devuelve `true` cuando `DRDY` está bajo.

## Comandos del ADS1299

- `cmdWakeup()` / `cmdStandby()`: sale de o entra en standby.
- `cmdReset()`: envía RESET por SPI.
- `cmdStart()` / `cmdStop()`: inicia o detiene conversiones mediante comando.
- `cmdRDATAC()`: activa la lectura continua y el estado interno correspondiente.
- `cmdSDATAC()`: detiene la lectura continua; úsalo antes de acceder a registros.
- `cmdRDATA()`: solicita un frame. Normalmente se usa indirectamente mediante
  `readDataOnDemand()`.

También están disponibles `pinStartHigh()`, `pinStartLow()`,
`pinResetPulse()` y `pinPowerDown()`. Esta última no hace nada cuando `PWDN`
se declaró como `ADS_PIN_UNUSED`.

## Registros

Estas funciones devuelven `false` si el rango no es válido o si `RDATAC` está
activo:

```cpp
bool writeReg(uint8_t addr, uint8_t value);
bool readReg(uint8_t addr, uint8_t& value);
bool writeRegs(uint8_t startAddr, const uint8_t* data, size_t n);
bool readRegs(uint8_t startAddr, uint8_t* data, size_t n);
```

Ejemplo:

```cpp
ads.cmdSDATAC();
uint8_t config1;
if (ads.readReg(ADS_REG_CONFIG1, config1)) {
  // Usar config1.
}
```

## Configuración

- Reloj y tasa: `setDataRate()`, `setClockOut()` y
  `setMultipleReadbackMode()`.
- Canal: `setChannel()`, `powerDownChannel()`, `setChannelGain()`,
  `setChannelMux()` y `setSRB2()`.
- Referencia común: `enableSRB1()`.
- Referencia y BIAS: `useInternalRef()`, `useBiasInternalRef()`,
  `enableBiasBuffer()`, `routeBiasSense()` y `enableBiasMeasure()`.
- Derivación BIAS: `setBiasDeriveP()` y `setBiasDeriveN()`.
- Lead-off: `configureLeadOff()`, `enableLeadOffSenseP()`,
  `enableLeadOffSenseN()`, `setLeadOffFlip()` y
  `enableLoffComparators()`.
- Conversión: `setSingleShot()`.

Los números de canal válidos van de 1 a `channelCount()`. Las funciones que
reciben máscaras ignoran los bits de canales que no existen en la variante
detectada. Configura registros solo con `RDATAC` detenido.

## Adquisición

```cpp
bool readFrameRDATAC(
  uint32_t& status, int32_t* channels, size_t capacity
);
bool readDataOnDemand(
  uint32_t& status, int32_t* channels, size_t capacity
);
```

`readFrameRDATAC()` exige `RDATAC` activo. `readDataOnDemand()` exige `RDATAC`
inactivo y envía `RDATA`. Ambas requieren una capacidad igual o superior a
`channelCount()`, decodifican los canales a `int32_t` y devuelven `false` si el
patrón de sincronización de `STATUS` no es válido.

Con un array de ocho posiciones también puedes usar las sobrecargas sin
`capacity`:

```cpp
int32_t channels[ADS1299Plus::MAX_CHANNELS];
uint32_t status;
bool ok = ads.readFrameRDATAC(status, channels);
```

Helpers de `STATUS`: `statusHasSync()`, `statusLoffP()`, `statusLoffN()` y
`statusGPIO()`. `unpack24()` convierte tres bytes MSB-first con signo a
`int32_t`.

