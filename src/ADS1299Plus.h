// ADS1299Plus.h
// Arduino-compatible C++ driver for the TI ADS1299-x family.
//
// Soporte dinámico de variante:
// - ADS1299-4  -> 4 canales
// - ADS1299-6  -> 6 canales
// - ADS1299    -> 8 canales
//
// El número real de canales se detecta leyendo el registro ID durante begin().
// Para código de aplicación, reserva siempre arrays de ADS1299Plus::MAX_CHANNELS
// y recorre únicamente ads.channelCount().
//
// Perfil por defecto conservador:
// - entradas fully differential
// - ganancia = 24
// - referencia interna
// - data rate = 250 SPS
// - CLK_OUT deshabilitado
// - bias drive deshabilitado
// - configuración LOFF legacy probada en hardware
// - comparadores lead-off deshabilitados por defecto para preservar adquisición validada
// - GPIO como entrada por defecto

#pragma once
#include <Arduino.h>
#include "ADS1299_Registers.h"
#include "ADS1299_SafeSPI.h"
#include "core/ADS1299_Core.h"

class ADS1299Plus {
public:
  // ----- Constantes del dispositivo -----
  static constexpr uint8_t MIN_CHANNELS = ADS1299Core::MIN_CHANNELS;
  static constexpr uint8_t MAX_CHANNELS = ADS1299Core::MAX_CHANNELS;

  // Alias de capacidad máxima. Se mantiene el nombre NUM_CHANNELS para que los
  // sketches que reservan arrays con ADS1299Plus::NUM_CHANNELS sean seguros en
  // ADS1299-4/6/8. Para conocer canales reales, usar channelCount().
  static constexpr uint8_t NUM_CHANNELS = MAX_CHANNELS;

  static constexpr uint16_t STATUS_BYTES = ADS1299Core::STATUS_BYTES;
  static constexpr uint16_t BYTES_PER_CHANNEL = ADS1299Core::BYTES_PER_CHANNEL;
  static constexpr uint16_t BYTES_PER_FRAME_MAX = ADS1299Core::BYTES_PER_FRAME_MAX;

  // Valor reservado para indicar que un pin opcional no está conectado al MCU.
  // Uso típico: PWDN cableado directamente a VDD.
  static constexpr uint8_t ADS_PIN_UNUSED = 0xFF;

  // Alias legacy: tamaño del frame de ADS1299-4.
  static constexpr uint16_t BYTES_PER_FRAME_4CH = STATUS_BYTES + BYTES_PER_CHANNEL * 4;

  // ----- Estructura de pines -----
  struct Pins {
    uint8_t cs;     // CS  (activo a nivel bajo)
    uint8_t sclk;   // SCLK hardware SPI de la placa. Informativo; lo configura SPI.begin().
    uint8_t mosi;   // DIN  (MCU->ADS), hardware SPI. Informativo; lo configura SPI.begin().
    uint8_t miso;   // DOUT (ADS->MCU), hardware SPI. Informativo; lo configura SPI.begin().
    uint8_t drdy;   // DRDY (ADS->MCU), activo bajo
    uint8_t start;  // START (MCU->ADS)
    uint8_t reset;  // RESET (MCU->ADS), activo bajo
    uint8_t pwdn;   // PWDN opcional. Usar ADS_PIN_UNUSED si PWDN va directo a VDD.
  };

  // ----- Configuración por defecto -----
  // CONFIG1: 0x96 = 250 SPS, CLK_OUT off, DAISY_EN=0.
  // Nota: DAISY_EN=0 significa daisy-chain mode según datasheet; se conserva
  // este valor porque es el comportamiento histórico probado en hardware.
  static constexpr uint8_t kCFG1_Default = ADS_CFG1_250SPS;

  // CONFIG2: test interno apagado.
  static constexpr uint8_t kCFG2_Default = ADS_CFG2_TEST_OFF;

  // CONFIG3: referencia interna ON, bias OFF, no medir bias.
  static constexpr uint8_t kCFG3_Default = ADS_CFG3_INTREF_NO_BIAS;

  // LOFF: constante legacy que reproduce el byte probado históricamente.
  // El umbral real es 87.5 %, no 80 %. La constante corregida 80 % queda
  // disponible como ADS_LOFF_AC_24NA_31HZ_80PCT.
  static constexpr uint8_t kLOFF_Default = ADS_LOFF_AC_24NA_31HZ_87_5PCT_LEGACY;

  // CHnSET por defecto: ON, GAIN=24, MUX=normal diff, SRB2=OFF.
  static inline uint8_t kCH_Default() { return ADS_CH_DEFAULT_GAIN24(); }

  // GPIO: todos como entrada.
  static constexpr uint8_t kGPIO_Default = ADS_GPIO_ALL_INPUTS;

  // CONFIG4: conversión continua y comparadores lead-off OFF por defecto.
  // Se preserva el byte 0x00 validado en adquisición. Para habilitar
  // comparadores: enableLoffComparators(true) o ADS_CFG4_CONT_LOFF_COMP_ON.
  static constexpr uint8_t kCFG4_Default = ADS_CFG4_CONT_LOFF_COMP_OFF;

public:
  // ----- Construcción -----
  ADS1299Plus(ADS1299_SafeSPI& spi, const Pins& pins);
  ADS1299Plus(ADS1299_HAL& hal,
              const Pins& pins,
              uint32_t spiHz = ADS1299_SafeSPI::DEFAULT_SPI_HZ);

  // ----- Ciclo de vida -----
  bool begin();
  bool configureDefaults();
  void end();

  // ----- Información de dispositivo detectado -----
  uint8_t channelCount() const { return num_channels_; }
  uint16_t bytesPerFrame() const { return ADS1299Core::bytesPerFrame(num_channels_); }
  static uint8_t channelsFromDeviceID(uint8_t id);

  // ----- Comandos SPI (9.5.3.x) -----
  void cmdWakeup();
  void cmdStandby();
  void cmdReset();
  void cmdStart();
  void cmdStop();
  void cmdRDATAC();
  void cmdSDATAC();
  void cmdRDATA();

  // ----- Acceso a registros (usar fuera de RDATAC) -----
  bool writeReg(uint8_t addr, uint8_t value);
  bool readReg (uint8_t addr, uint8_t& value);
  bool writeRegs(uint8_t startAddr, const uint8_t* data, size_t n);
  bool readRegs (uint8_t startAddr,       uint8_t* data, size_t n);

  // ----- Helpers de alto nivel -----
  bool setDataRate(uint8_t dr3b);
  bool setClockOut(bool enable);

  // CONFIG1.DAISY_EN según datasheet: 0=daisy-chain, 1=multiple readback.
  bool setMultipleReadbackMode(bool enable);

  // Alias legacy: conserva comportamiento anterior, pero el nombre puede inducir
  // a error. Internamente controla el bit DAISY_EN tal cual lo define TI.
  bool setDaisyEnable(bool enable);

  // Canales válidos: ch=[1..channelCount()].
  bool setChannel(uint8_t ch, uint8_t chsetByte);
  bool powerDownChannel(uint8_t ch, bool pd);
  bool setChannelGain(uint8_t ch, uint8_t gain3b);
  bool setChannelMux (uint8_t ch, uint8_t mux3b);
  bool setSRB2(uint8_t ch, bool en);

  bool enableSRB1(bool en);

  // BIAS/Referencia (CONFIG3).
  bool useInternalRef(bool enBuf);
  bool useBiasInternalRef(bool enInt);
  bool enableBiasBuffer(bool en);
  bool routeBiasSense(bool en);
  bool enableBiasMeasure(bool en);

  // Lead-Off (LOFF + LOFF_SENSP/N + LOFF_FLIP + CONFIG4).
  bool configureLeadOff(uint8_t loffByte);
  bool enableLeadOffSenseP(uint8_t chMask);
  bool enableLeadOffSenseN(uint8_t chMask);
  bool setLeadOffFlip(uint8_t chMask);
  bool setSingleShot(bool singleShot);

  // CONFIG4 bit 1: true -> comparadores lead-off habilitados; false -> deshabilitados.
  bool enableLoffComparators(bool en);

  // BIAS derivation.
  bool setBiasDeriveP(uint8_t chMask);
  bool setBiasDeriveN(uint8_t chMask);

  // ----- Adquisición -----
  // Variante segura: capacity debe ser >= channelCount().
  bool readFrameRDATAC(uint32_t& status24, int32_t* chOut, size_t capacity);
  bool readDataOnDemand(uint32_t& status24, int32_t* chOut, size_t capacity);

  // Variante cómoda: usar con arrays de tamaño ADS1299Plus::NUM_CHANNELS
  // o ADS1299Plus::MAX_CHANNELS.
  bool readFrameRDATAC(uint32_t& status24, int32_t chOut[NUM_CHANNELS]) {
    return readFrameRDATAC(status24, chOut, NUM_CHANNELS);
  }
  bool readDataOnDemand(uint32_t& status24, int32_t chOut[NUM_CHANNELS]) {
    return readDataOnDemand(status24, chOut, NUM_CHANNELS);
  }

  bool dataReady() const;
  bool isRDATACActive() const { return rdatacActive_; }

  // Decodificadores de STATUS (9.4.4.2).
  static inline bool statusHasSync(uint32_t s) {
    return ADS1299Core::statusHasSync(s);
  }
  static inline uint8_t statusLoffP(uint32_t s) { return ADS1299Core::statusLoffP(s); }
  static inline uint8_t statusLoffN(uint32_t s) { return ADS1299Core::statusLoffN(s); }
  static inline uint8_t statusGPIO (uint32_t s) { return ADS1299Core::statusGPIO(s); }

  // Convierte 3 bytes MSB-first en entero con signo (24 bits -> 32 bits).
  static inline int32_t unpack24(const uint8_t b[3]) {
    return ADS1299Core::unpack24(b);
  }

  bool readDeviceID(uint8_t& id);

  // Acceso a pines auxiliares. pinPowerDown() no hace nada si PWDN no está
  // conectado al MCU (pins_.pwdn == ADS_PIN_UNUSED).
  void pinStartHigh();
  void pinStartLow();
  void pinResetPulse();
  void pinPowerDown(bool activeLow);

private:
  bool writeOne_(uint8_t addr, uint8_t val);
  bool readOne_ (uint8_t addr, uint8_t& val);
  bool writeBurst_(uint8_t startAddr, const uint8_t* data, size_t n);
  bool readBurst_ (uint8_t startAddr,       uint8_t* data, size_t n);
  void waitUs_(uint32_t us) const;
  void waitMs_(uint32_t ms) const;
  void waitDecode_() const;

  bool validCh_(uint8_t ch) const { return ADS1299Core::isValidChannel(ch, num_channels_); }
  static inline uint8_t chRegAddr_(uint8_t ch) { return ADS1299Core::channelRegisterAddress(ch); }
  uint8_t clipMask_(uint8_t mask) const { return ADS1299Core::clipChannelMask(mask, num_channels_); }
  static inline bool validRegRange_(uint8_t startAddr, size_t n) {
    return ADS1299Core::validRegisterRange(startAddr, n);
  }

private:
  ADS1299_SafeSPI ownedSpi_;
  ADS1299_SafeSPI* spi_;
  ADS1299_HAL* hal_;
  Pins pins_;
  bool useHal_ = false;
  bool rdatacActive_ = false;
  uint8_t num_channels_ = MAX_CHANNELS; // se actualiza en begin() leyendo ID.
};
