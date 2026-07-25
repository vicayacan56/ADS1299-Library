# Solución de problemas

## `ADS1299Plus.h: No such file or directory`

- Instala el ZIP desde Arduino IDE o coloca la carpeta completa dentro de
  `libraries` en el sketchbook.
- Comprueba que `library.properties` y `src` estén en la raíz de esa carpeta,
  sin un nivel de carpetas duplicado.
- Reinicia Arduino IDE después de una instalación manual.
- No copies solo el archivo `.ino`.

## `begin()` devuelve `false`

`begin()` falla si no puede leer un ID de la familia ADS1299 o si la variante no
indica 4, 6 u 8 canales.

- Verifica alimentación, masa común, reloj del ADC y niveles lógicos.
- Comprueba `CS`, SCK, MOSI, MISO y RESET.
- Confirma que `PWDN` está alto. Si está unido a `VDD`, usa
  `ADS1299Plus::ADS_PIN_UNUSED`.
- Asegúrate de que ningún otro dispositivo mantiene ocupado el bus o `CS`.
- Ejecuta `RegisterDump` con los pines adaptados a tu placa.

## ID incorrecto o cambiante

Un valor `0x00`, `0xFF` o que cambia suele indicar ausencia de respuesta,
MISO flotante, `CS` incorrecto, alimentación/reloj inestables o incompatibilidad
de niveles. La revisión del chip puede cambiar bits del byte completo; valida
la familia y el número de canales, no solo una constante hexadecimal copiada.

## Frames inestables o error de sincronización

- Confirma que `STATUS` empieza por el patrón `0xC`.
- Lee exactamente `bytesPerFrame()` bytes por cada flanco de `DRDY`.
- No leas ni escribas registros durante `RDATAC`.
- Evita imprimir por Serial tan rápido que se pierdan muestras; para diagnosticar,
  reduce la cantidad de salida o almacena los datos en un buffer.
- Revisa masa, alimentación, desacoplo, longitud del cableado SPI y reloj.
- Comprueba que solo se inicia una vez el modo elegido.

## `DRDY` no cambia

- Confirma que el ADC no está en standby o power-down.
- Revisa `START`: `BasicRead` lo mantiene alto antes de `cmdRDATAC()`.
- Verifica RESET, reloj maestro, configuración y el pin real asignado a `DRDY`.
- Recuerda que `DRDY` es activo en bajo.

## Errores de compilación tras instalar

- Selecciona la placa y el core correctos en Arduino IDE o usa el FQBN correcto
  en Arduino CLI.
- Comprueba que el core proporciona `Arduino.h` y `SPI.h`.
- Elimina niveles de carpeta duplicados creados al descomprimir el ZIP.
- Activa la salida detallada de compilación para ver qué copia de la librería
  selecciona Arduino.

## Librería duplicada

Arduino IDE puede elegir otra copia si existe en varias carpetas. Revisa los
mensajes `Multiple libraries were found` del compilador y conserva una sola
instalación de ADS1299Plus. En Arduino CLI:

```powershell
arduino-cli lib list | Select-String ADS1299Plus
```

Después de retirar la copia antigua, reinicia el IDE y vuelve a compilar
`RegisterDump`.

