# Pruebas sin hardware

Sin un ADS1299 conectado puedes validar la instalación y la compatibilidad de
compilación, pero no la comunicación ni la adquisición.

## Comprobaciones posibles

Desde la raíz del repositorio:

```powershell
arduino-cli version
arduino-cli core list
arduino-cli compile --fqbn arduino:avr:uno .\examples\RegisterDump
arduino-cli compile --fqbn arduino:avr:uno .\examples\BasicRead
```

Una compilación correcta confirma que:

- Arduino encuentra `ADS1299Plus.h`, `ADS1299_SafeSPI.h` y sus fuentes;
- la estructura y los metadatos de la librería son válidos;
- los ejemplos usan una API disponible;
- el core seleccionado proporciona Arduino y SPI.

También puedes inspeccionar que Arduino CLI ve una única instalación:

```powershell
arduino-cli lib list | Select-String ADS1299Plus
```

## Lo que exige hardware real

Necesitas un ADS1299 alimentado y cableado para validar:

- respuesta SPI y lectura correcta del registro `ID`;
- detección de 4, 6 u 8 canales;
- lectura y escritura reales de registros;
- transiciones de `DRDY`;
- integridad y sincronización de frames `RDATAC` o `RDATA`;
- ruido, estabilidad, tasa de muestreo y valores de canal;
- RESET, START y PWDN;
- referencia, BIAS, lead-off y entradas analógicas.

Compilar no demuestra que los pines, tensiones, reloj, aislamiento o señales
analógicas sean correctos. El primer ensayo con placa debe ser
`RegisterDump`; después usa `BasicRead`.

