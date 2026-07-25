# ADS1299Plus

Librería Arduino para controlar por SPI los ADC de biopotenciales de 24 bits
Texas Instruments **ADS1299-4**, **ADS1299-6** y **ADS1299** (8 canales).

La librería detecta la variante conectada, permite leer y escribir registros,
configurar canales y adquirir frames en modo continuo (`RDATAC`) o bajo demanda
(`RDATA`).

> **Seguridad:** el ADS1299 se usa habitualmente en equipos conectados a
> electrodos. No conectes una persona a un montaje alimentado o enlazado a
> equipos sin el aislamiento y las protecciones exigidos para aplicaciones
> biomédicas. Esta librería no convierte un prototipo en un dispositivo médico.

## Instalación

### Arduino IDE

1. Descarga el ZIP de este repositorio desde GitHub.
2. En Arduino IDE abre **Programa > Incluir librería > Añadir biblioteca
   .ZIP...**.
3. Selecciona el ZIP y reinicia el IDE si no aparecen los ejemplos.
4. Abre **Archivo > Ejemplos > ADS1299Plus > RegisterDump**.

Si instalas manualmente, la carpeta que contiene `library.properties` debe
quedar dentro de `Documentos\Arduino\libraries\ADS1299Plus`.

### Arduino CLI

Desde PowerShell, con el ZIP descargado:

```powershell
arduino-cli lib install --zip-path .\ADS1299-Library-main.zip
arduino-cli core update-index
arduino-cli core install arduino:avr
arduino-cli compile --fqbn arduino:avr:uno .\examples\RegisterDump
arduino-cli compile --fqbn arduino:avr:uno .\examples\BasicRead
```

Los dos últimos comandos se ejecutan desde una copia del repositorio. Ajusta el
FQBN y los pines para tu placa.

## Cableado

| Señal ADS1299 | Dirección | Conexión |
|---|---|---|
| `CS` | MCU → ADS1299 | GPIO seleccionado como chip select |
| `SCLK` | MCU → ADS1299 | Pin SCK del SPI hardware |
| `DIN` / `MOSI` | MCU → ADS1299 | Pin MOSI del SPI hardware |
| `DOUT` / `MISO` | ADS1299 → MCU | Pin MISO del SPI hardware |
| `DRDY` | ADS1299 → MCU | GPIO de entrada; activo en bajo |
| `START` | MCU → ADS1299 | GPIO de salida |
| `RESET` | MCU → ADS1299 | GPIO de salida; activo en bajo |
| `PWDN` | MCU → ADS1299 | GPIO opcional; activo en bajo |

Une también las masas digitales y verifica que los niveles lógicos, fuentes,
reloj y desacoplos cumplen el datasheet y el diseño de tu placa ADS1299.
`SCK`, `MOSI` y `MISO` son los pines SPI hardware de la placa Arduino.

Si `PWDN` está conectado directamente a `VDD`, no lo configures como GPIO:

```cpp
static constexpr uint8_t PIN_PWDN = ADS1299Plus::ADS_PIN_UNUSED;
```

## Primer arranque recomendado

1. Instala la librería.
2. Compila `RegisterDump`.
3. Conecta el hardware con la alimentación desconectada.
4. Ejecuta `RegisterDump` y abre el monitor serie a 115200 baudios.
5. Confirma que se lee un ID ADS1299 y que aparecen 4, 6 u 8 canales.
6. Ajusta los mismos pines en `BasicRead`, compílalo y ejecútalo.
7. Confirma que `DRDY` genera lecturas y que aparecen frames `RDATAC` estables.

`RegisterDump` inicializa el chip, aplica la configuración por defecto y muestra
los registros principales sin iniciar adquisición continua. `BasicRead`
configura el chip, inicia `RDATAC` e imprime `STATUS` y los canales como
valores `int32_t`.

## Uso mínimo

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
    // Procesar solo channels[0] ... channels[ads.channelCount() - 1].
  }
}
```

## Archivos principales

- `src/ADS1299Plus.h` y `.cpp`: API del dispositivo, secuencia de inicio,
  registros, configuración de canales y adquisición.
- `src/ADS1299_SafeSPI.h` y `.cpp`: transporte Arduino SPI, control de `CS`,
  modo SPI 1 y temporización de comandos.
- `src/ADS1299_Registers.h`: direcciones, comandos, máscaras y constantes del
  mapa de registros.
- `examples/RegisterDump`: diagnóstico inicial e inspección de registros.
- `examples/BasicRead`: adquisición continua mínima con `RDATAC`.

## Documentación

- [Índice](docs/README.md)
- [Guía de usuario](docs/user-guide.md)
- [Referencia de API](docs/api-reference.md)
- [Pruebas sin hardware](docs/testing-without-hardware.md)
- [Solución de problemas](docs/troubleshooting.md)
