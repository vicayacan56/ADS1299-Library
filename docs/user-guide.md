# Guía de usuario

## Dispositivos compatibles

ADS1299Plus controla las tres variantes de la familia:

| Dispositivo | Canales detectados | Bytes por frame |
|---|---:|---:|
| ADS1299-4 | 4 | 15 |
| ADS1299-6 | 6 | 21 |
| ADS1299 | 8 | 27 |

`begin()` lee el registro `ID` y selecciona automáticamente el número de
canales. Reserva siempre un array de `ADS1299Plus::MAX_CHANNELS` y procesa solo
los elementos indicados por `channelCount()`.

## Instalación

En Arduino IDE, descarga el ZIP del repositorio y usa **Programa > Incluir
librería > Añadir biblioteca .ZIP...**. Después abre
**Archivo > Ejemplos > ADS1299Plus > RegisterDump**.

Con Arduino CLI puedes instalar el mismo ZIP y compilar los ejemplos:

```powershell
arduino-cli lib install --zip-path .\ADS1299-Library-main.zip
arduino-cli compile --fqbn arduino:avr:uno .\examples\RegisterDump
arduino-cli compile --fqbn arduino:avr:uno .\examples\BasicRead
```

Si copias la librería manualmente, `library.properties`, `src` y `examples`
deben estar juntos en una sola carpeta dentro del directorio `libraries` de tu
sketchbook.

## Pines y cableado digital

Define `CS`, `DRDY`, `START` y `RESET` como GPIO adecuados para tu placa. Usa
los pines `SCK`, `MOSI` y `MISO` del periférico SPI hardware. `PWDN` puede ir a
un GPIO o directamente a `VDD`.

```cpp
static constexpr uint8_t PIN_CS    = 10;
static constexpr uint8_t PIN_DRDY  = 7;
static constexpr uint8_t PIN_START = 9;
static constexpr uint8_t PIN_RESET = 8;
static constexpr uint8_t PIN_PWDN  = ADS1299Plus::ADS_PIN_UNUSED; // PWDN a VDD
```

El orden de `ADS1299Plus::Pins` es:

```cpp
ADS1299Plus::Pins pins = {
  PIN_CS, SCK, MOSI, MISO, PIN_DRDY, PIN_START, PIN_RESET, PIN_PWDN
};
```

No asumas que una placa ADS1299 tolera los niveles de tensión de cualquier
Arduino. Comprueba alimentación, masa, reloj, niveles lógicos y desacoplos
contra el datasheet y el esquema de tu módulo.

## Puesta en marcha

1. Compila `RegisterDump` antes de conectar el ADC.
2. Desconecta la alimentación y realiza el cableado.
3. Enciende el sistema y abre el monitor serie a 115200 baudios.
4. Confirma que `begin()` no falla, que el ID pertenece a un ADS1299 y que
   `channelCount()` devuelve 4, 6 u 8.
5. Revisa el volcado producido después de `configureDefaults()`.
6. Compila y ejecuta `BasicRead`.
7. Confirma que `DRDY` pasa a nivel bajo y se imprimen frames sin errores de
   sincronización.

## Qué configura `configureDefaults()`

La función detiene `RDATAC` y las conversiones antes de escribir registros.
Después aplica una base conservadora:

- 250 muestras por segundo.
- referencia interna habilitada y señal de prueba interna deshabilitada;
- canales activos, entrada diferencial normal, ganancia 24 y `SRB2` apagado;
- BIAS drive deshabilitado;
- parámetros lead-off cargados, con comparadores lead-off deshabilitados;
- GPIO del ADS1299 como entradas;
- conversión continua.

La función no inicia la adquisición. Si necesitas otra configuración, llama a
los helpers correspondientes fuera de `RDATAC`.

## Inicio y parada de adquisición

`BasicRead` mantiene `START` alto y activa la salida continua:

```cpp
ads.pinStartHigh();
delay(10);
ads.cmdRDATAC();
```

Como alternativa, con el pin `START` bajo puedes usar `cmdStart()` para enviar
el comando START. `cmdRDATAC()` activa el envío continuo de frames. Antes de
leer o escribir registros, llama a `cmdSDATAC()`; para detener conversiones usa
también `cmdStop()` o baja el pin `START`, según el método de inicio elegido.

Para una muestra bajo demanda, mantén `RDATAC` desactivado y usa:

```cpp
uint32_t status;
int32_t channels[ADS1299Plus::MAX_CHANNELS];
bool ok = ads.readDataOnDemand(
  status, channels, ADS1299Plus::MAX_CHANNELS
);
```

## Interpretación de resultados

- **ID:** `begin()` valida los bits que identifican la familia ADS1299 y usa los
  bits de variante para detectar 4, 6 u 8 canales. El byte completo puede
  incluir bits de revisión; no compruebes solo un valor hexadecimal fijo.
- **`channelCount()`:** número real de canales detectados.
- **`bytesPerFrame()`:** 3 bytes de `STATUS` más 3 bytes por canal.
- **`STATUS`:** palabra de 24 bits. Su nibble superior debe ser `0xC`; el resto
  contiene estados lead-off y GPIO. Un patrón de sincronización incorrecto hace
  que la función de lectura devuelva `false`.
- **Canales:** cada muestra de 24 bits se extiende con signo y se entrega como
  `int32_t`. Es un código ADC sin convertir a voltios. Recorre únicamente
  `channelCount()` posiciones.

Consulta la [referencia de API](api-reference.md) para configurar registros,
canales, referencia, BIAS y lead-off.

