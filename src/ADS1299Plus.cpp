// ADS1299Plus.cpp

#include "ADS1299Plus.h"
#include "ADS1299_SafeSPI.h"
#include "ADS1299_Registers.h"

// Utilidades internas de tiempo.
static inline void ads_wait_us(uint32_t us) { delayMicroseconds(us); }
static inline void ads_wait_ms(uint32_t ms) { delay(ms); }

ADS1299Plus::ADS1299Plus(ADS1299_SafeSPI &spi, const Pins &pins)
    : ownedSpi_(pins.cs),
      spi_(&spi),
      hal_(nullptr),
      pins_(pins),
      useHal_(false) {}

ADS1299Plus::ADS1299Plus(ADS1299_HAL &hal, const Pins &pins, uint32_t spiHz)
    : ownedSpi_(hal, spiHz),
      spi_(&ownedSpi_),
      hal_(&hal),
      pins_(pins),
      useHal_(true) {}

void ADS1299Plus::waitUs_(uint32_t us) const
{
  if (useHal_) {
    hal_->delayMicroseconds(us);
  } else {
    ads_wait_us(us);
  }
}

void ADS1299Plus::waitMs_(uint32_t ms) const
{
  if (useHal_) {
    hal_->delayMilliseconds(ms);
  } else {
    ads_wait_ms(ms);
  }
}

void ADS1299Plus::waitDecode_() const
{
  waitUs_(3);
}

uint8_t ADS1299Plus::channelsFromDeviceID(uint8_t id)
{
  return ADS1299Core::channelsFromDeviceID(id);
}

// ---- Control de pines auxiliares ----
void ADS1299Plus::pinStartHigh()
{
  if (useHal_) {
    hal_->setStart(true);
  } else {
    digitalWrite(pins_.start, HIGH);
  }
}

void ADS1299Plus::pinStartLow()
{
  if (useHal_) {
    hal_->setStart(false);
  } else {
    digitalWrite(pins_.start, LOW);
  }
}

void ADS1299Plus::pinResetPulse()
{
  if (useHal_) {
    hal_->setReset(false);
    waitUs_(10);
    hal_->setReset(true);
  } else {
    digitalWrite(pins_.reset, LOW);
    waitUs_(10);
    digitalWrite(pins_.reset, HIGH);
  }
  waitUs_(20);
}

void ADS1299Plus::pinPowerDown(bool activeLow)
{
  if (useHal_) {
    hal_->setPwdn(!activeLow);
    return;
  }

  if (pins_.pwdn == ADS_PIN_UNUSED)
    return;
  digitalWrite(pins_.pwdn, activeLow ? LOW : HIGH);
}

bool ADS1299Plus::dataReady() const
{
  if (useHal_) {
    return !hal_->readDrdy();
  }

  return digitalRead(pins_.drdy) == LOW;
}

// ---- Secuencia de arranque (11.1) ----
bool ADS1299Plus::begin()
{
  if (useHal_) {
    // The HAL-backed SafeSPI path initializes GPIO, SPI, CS idle state,
    // START/RESET/PWDN defaults, and SPI transaction settings.
    spi_->begin();
    waitMs_(5);
  } else {
    // 1) Configurar pines.
    pinMode(pins_.cs, OUTPUT);
    digitalWrite(pins_.cs, HIGH);
    // SCLK/MOSI/MISO belong to the selected Arduino SPI peripheral and are
    // configured by SPI.begin() in ADS1299_SafeSPI. They are kept in Pins as
    // documentation of the wiring, but the driver does not force pinMode() on
    // them to remain portable across Arduino-compatible cores.
    pinMode(pins_.drdy, INPUT_PULLUP);
    pinMode(pins_.start, OUTPUT);
    digitalWrite(pins_.start, LOW);
    pinMode(pins_.reset, OUTPUT);
    digitalWrite(pins_.reset, HIGH);

    // PWDN es activo en bajo. En muchos diseños se conecta directamente a VDD;
    // en ese caso no debe configurarse como GPIO y se indica con ADS_PIN_UNUSED.
    if (pins_.pwdn != ADS_PIN_UNUSED) {
      pinMode(pins_.pwdn, OUTPUT);
      digitalWrite(pins_.pwdn, HIGH);
    }

    // 2) Esperar a que fuentes y líneas digitales se estabilicen.
    waitMs_(5);

    // 3) Inicializar SPI seguro. ADS1299_SafeSPI::begin() es idempotente.
    spi_->begin();
  }

  // 4) Reset digital.
  cmdReset();

  // 5) Tras reset/power-up, RDATAC puede estar activo por defecto. Salir de
  // lectura continua antes de acceder a registros.
  cmdSDATAC();
  cmdStop();

  // 6) Verificar ID.
  uint8_t id = 0;
  if (!readReg(ADS_REG_ID, id))
    return false;
  if (!ADS_ID_DEV_IS_1299(id))
    return false;

  // 7) Detectar automáticamente la variante: ADS1299-4/6/8.
  const uint8_t detected_channels = channelsFromDeviceID(id);
  if (detected_channels == 0)
    return false;

  num_channels_ = detected_channels;
  return true;
}

// ---- Configuración por defecto ----
bool ADS1299Plus::configureDefaults()
{
  // Acceso a registros siempre fuera de RDATAC.
  cmdSDATAC();
  cmdStop();

  if (!writeReg(ADS_REG_CONFIG1, kCFG1_Default))
    return false;
  if (!writeReg(ADS_REG_CONFIG2, kCFG2_Default))
    return false;
  if (!writeReg(ADS_REG_CONFIG3, kCFG3_Default))
    return false;
  if (!writeReg(ADS_REG_LOFF, kLOFF_Default))
    return false;

  for (uint8_t ch = 1; ch <= num_channels_; ++ch)
  {
    if (!setChannel(ch, kCH_Default()))
      return false;
  }

  if (!writeReg(ADS_REG_BIAS_SENSP, 0x00))
    return false;
  if (!writeReg(ADS_REG_BIAS_SENSN, 0x00))
    return false;

  const uint8_t activeMask = ADS_ClipMaskToChannels(0xFF, num_channels_);
  if (!enableLeadOffSenseP(activeMask))
    return false;
  if (!enableLeadOffSenseN(activeMask))
    return false;

  if (!writeReg(ADS_REG_LOFF_FLIP, 0x00))
    return false;
  if (!writeReg(ADS_REG_GPIO, kGPIO_Default))
    return false;
  if (!writeReg(ADS_REG_MISC1, 0x00))
    return false;
  if (!writeReg(ADS_REG_CONFIG4, kCFG4_Default))
    return false;

  return true;
}

void ADS1299Plus::end()
{
  cmdStop();
  cmdSDATAC();
  spi_->end();
}

// ---- Comandos SPI ----
void ADS1299Plus::cmdWakeup()
{
  spi_->select();
  spi_->xfer(ADS_CMD_WAKEUP);
  spi_->deselect();
  waitDecode_();
}

void ADS1299Plus::cmdStandby()
{
  spi_->select();
  spi_->xfer(ADS_CMD_STANDBY);
  spi_->deselect();
  waitDecode_();
}

void ADS1299Plus::cmdReset()
{
  spi_->select();
  spi_->xfer(ADS_CMD_RESET);
  spi_->deselect();
  rdatacActive_ = false; // El estado real se normaliza con cmdSDATAC() tras reset.
  waitUs_(20);
}

void ADS1299Plus::cmdStart()
{
  spi_->select();
  spi_->xfer(ADS_CMD_START);
  spi_->deselect();
  waitDecode_();
}

void ADS1299Plus::cmdStop()
{
  spi_->select();
  spi_->xfer(ADS_CMD_STOP);
  spi_->deselect();
  waitDecode_();
}

void ADS1299Plus::cmdRDATAC()
{
  spi_->select();
  spi_->xfer(ADS_CMD_RDATAC);
  spi_->deselect();
  rdatacActive_ = true;
  waitDecode_();
}

void ADS1299Plus::cmdSDATAC()
{
  spi_->select();
  spi_->xfer(ADS_CMD_SDATAC);
  spi_->deselect();
  rdatacActive_ = false;
  waitDecode_();
}

void ADS1299Plus::cmdRDATA()
{
  spi_->select();
  spi_->xfer(ADS_CMD_RDATA);
  spi_->deselect();
}

// ---- Acceso a registros ----
bool ADS1299Plus::writeOne_(uint8_t addr, uint8_t val)
{
  if (rdatacActive_ || addr > ADS_REG_CONFIG4)
    return false;

  spi_->select();
  spi_->xfer(ADS1299Core::writeRegisterCommand(addr));
  spi_->xfer(0x00); // escribir 1 registro: n-1 = 0.
  spi_->xfer(val);
  spi_->deselect();
  waitDecode_();
  return true;
}

bool ADS1299Plus::readOne_(uint8_t addr, uint8_t &val)
{
  if (rdatacActive_ || addr > ADS_REG_CONFIG4)
    return false;

  spi_->select();
  spi_->xfer(ADS1299Core::readRegisterCommand(addr));
  spi_->xfer(0x00); // leer 1 registro: n-1 = 0.
  val = spi_->xfer(ADS_CMD_NOP);
  spi_->deselect();
  waitDecode_();
  return true;
}

bool ADS1299Plus::writeBurst_(uint8_t startAddr, const uint8_t *data, size_t n)
{
  if (rdatacActive_ || data == nullptr || !validRegRange_(startAddr, n))
    return false;

  spi_->select();
  spi_->xfer(ADS1299Core::writeRegisterCommand(startAddr));
  spi_->xfer((uint8_t)(n - 1));
  for (size_t i = 0; i < n; ++i)
  {
    spi_->xfer(data[i]);
  }
  spi_->deselect();
  waitDecode_();
  return true;
}

bool ADS1299Plus::readBurst_(uint8_t startAddr, uint8_t *data, size_t n)
{
  if (rdatacActive_ || data == nullptr || !validRegRange_(startAddr, n))
    return false;

  spi_->select();
  spi_->xfer(ADS1299Core::readRegisterCommand(startAddr));
  spi_->xfer((uint8_t)(n - 1));
  for (size_t i = 0; i < n; ++i)
  {
    data[i] = spi_->xfer(ADS_CMD_NOP);
  }
  spi_->deselect();
  waitDecode_();
  return true;
}

bool ADS1299Plus::writeReg(uint8_t addr, uint8_t value) { return writeOne_(addr, value); }
bool ADS1299Plus::readReg(uint8_t addr, uint8_t &value) { return readOne_(addr, value); }
bool ADS1299Plus::writeRegs(uint8_t startAddr, const uint8_t *data, size_t n) { return writeBurst_(startAddr, data, n); }
bool ADS1299Plus::readRegs(uint8_t startAddr, uint8_t *data, size_t n) { return readBurst_(startAddr, data, n); }

// ---- Helpers alto nivel ----
bool ADS1299Plus::setDataRate(uint8_t dr3b)
{
  uint8_t cfg1;
  if (!readReg(ADS_REG_CONFIG1, cfg1))
    return false;
  cfg1 = (cfg1 & 0xF8) | (dr3b & 0x07);
  return writeReg(ADS_REG_CONFIG1, cfg1);
}

bool ADS1299Plus::setClockOut(bool enable)
{
  uint8_t cfg1;
  if (!readReg(ADS_REG_CONFIG1, cfg1))
    return false;
  if (enable)
    cfg1 |= ADS_CFG1_CLK_EN;
  else
    cfg1 &= ~ADS_CFG1_CLK_EN;
  return writeReg(ADS_REG_CONFIG1, cfg1);
}

bool ADS1299Plus::setMultipleReadbackMode(bool enable)
{
  uint8_t cfg1;
  if (!readReg(ADS_REG_CONFIG1, cfg1))
    return false;
  if (enable)
    cfg1 |= ADS_CFG1_MULTIPLE_READBACK;
  else
    cfg1 &= ~ADS_CFG1_MULTIPLE_READBACK;
  return writeReg(ADS_REG_CONFIG1, cfg1);
}

bool ADS1299Plus::setDaisyEnable(bool enable)
{
  // Alias legacy: controla el bit DAISY_EN sin cambiar comportamiento previo.
  return setMultipleReadbackMode(enable);
}

bool ADS1299Plus::setChannel(uint8_t ch, uint8_t chsetByte)
{
  if (!validCh_(ch))
    return false;
  return writeReg(chRegAddr_(ch), chsetByte);
}

bool ADS1299Plus::powerDownChannel(uint8_t ch, bool pd)
{
  if (!validCh_(ch))
    return false;
  uint8_t ch_val;
  if (!readReg(chRegAddr_(ch), ch_val))
    return false;
  if (pd)
    ch_val |= ADS_CH_PD;
  else
    ch_val &= ~ADS_CH_PD;
  return writeReg(chRegAddr_(ch), ch_val);
}

bool ADS1299Plus::setChannelGain(uint8_t ch, uint8_t gain3b)
{
  if (!validCh_(ch))
    return false;
  uint8_t ch_val;
  if (!readReg(chRegAddr_(ch), ch_val))
    return false;
  ch_val = (ch_val & 0x8F) | ((gain3b & 0x07) << 4);
  return writeReg(chRegAddr_(ch), ch_val);
}

bool ADS1299Plus::setChannelMux(uint8_t ch, uint8_t mux3b)
{
  if (!validCh_(ch))
    return false;
  uint8_t ch_val;
  if (!readReg(chRegAddr_(ch), ch_val))
    return false;
  ch_val = (ch_val & 0xF8) | (mux3b & 0x07);
  return writeReg(chRegAddr_(ch), ch_val);
}

bool ADS1299Plus::setSRB2(uint8_t ch, bool en)
{
  if (!validCh_(ch))
    return false;
  uint8_t ch_val;
  if (!readReg(chRegAddr_(ch), ch_val))
    return false;
  if (en)
    ch_val |= ADS_CH_SRB2;
  else
    ch_val &= ~ADS_CH_SRB2;
  return writeReg(chRegAddr_(ch), ch_val);
}

bool ADS1299Plus::enableSRB1(bool en)
{
  uint8_t misc1;
  if (!readReg(ADS_REG_MISC1, misc1))
    return false;
  if (en)
    misc1 |= ADS_MISC1_SRB1;
  else
    misc1 &= ~ADS_MISC1_SRB1;
  return writeReg(ADS_REG_MISC1, misc1);
}

bool ADS1299Plus::useInternalRef(bool enBuf)
{
  uint8_t cfg3;
  if (!readReg(ADS_REG_CONFIG3, cfg3))
    return false;
  if (enBuf)
    cfg3 |= ADS_CFG3_PD_REFBUF;
  else
    cfg3 &= ~ADS_CFG3_PD_REFBUF;
  return writeReg(ADS_REG_CONFIG3, cfg3);
}

bool ADS1299Plus::useBiasInternalRef(bool enInt)
{
  uint8_t cfg3;
  if (!readReg(ADS_REG_CONFIG3, cfg3))
    return false;
  if (enInt)
    cfg3 |= ADS_CFG3_BIASREF_INT;
  else
    cfg3 &= ~ADS_CFG3_BIASREF_INT;
  return writeReg(ADS_REG_CONFIG3, cfg3);
}

bool ADS1299Plus::enableBiasBuffer(bool en)
{
  uint8_t cfg3;
  if (!readReg(ADS_REG_CONFIG3, cfg3))
    return false;
  if (en)
    cfg3 |= ADS_CFG3_PD_BIAS;
  else
    cfg3 &= ~ADS_CFG3_PD_BIAS;
  return writeReg(ADS_REG_CONFIG3, cfg3);
}

bool ADS1299Plus::routeBiasSense(bool en)
{
  uint8_t cfg3;
  if (!readReg(ADS_REG_CONFIG3, cfg3))
    return false;
  if (en)
    cfg3 |= ADS_CFG3_BIAS_LOFF_SENS;
  else
    cfg3 &= ~ADS_CFG3_BIAS_LOFF_SENS;
  return writeReg(ADS_REG_CONFIG3, cfg3);
}

bool ADS1299Plus::enableBiasMeasure(bool en)
{
  uint8_t cfg3;
  if (!readReg(ADS_REG_CONFIG3, cfg3))
    return false;
  if (en)
    cfg3 |= ADS_CFG3_BIAS_MEAS;
  else
    cfg3 &= ~ADS_CFG3_BIAS_MEAS;
  return writeReg(ADS_REG_CONFIG3, cfg3);
}

bool ADS1299Plus::configureLeadOff(uint8_t loffByte)
{
  return writeReg(ADS_REG_LOFF, loffByte);
}

bool ADS1299Plus::enableLeadOffSenseP(uint8_t chMask)
{
  return writeReg(ADS_REG_LOFF_SENSP, clipMask_(chMask));
}

bool ADS1299Plus::enableLeadOffSenseN(uint8_t chMask)
{
  return writeReg(ADS_REG_LOFF_SENSN, clipMask_(chMask));
}

bool ADS1299Plus::setLeadOffFlip(uint8_t chMask)
{
  return writeReg(ADS_REG_LOFF_FLIP, clipMask_(chMask));
}

bool ADS1299Plus::setSingleShot(bool singleShot)
{
  uint8_t cfg4;
  if (!readReg(ADS_REG_CONFIG4, cfg4))
    return false;
  if (singleShot)
    cfg4 |= ADS_CFG4_SINGLE_SHOT;
  else
    cfg4 &= ~ADS_CFG4_SINGLE_SHOT;
  return writeReg(ADS_REG_CONFIG4, cfg4);
}

bool ADS1299Plus::enableLoffComparators(bool en)
{
  uint8_t cfg4;
  if (!readReg(ADS_REG_CONFIG4, cfg4))
    return false;
  if (en)
    cfg4 |= ADS_CFG4_LOFF_COMP_EN;
  else
    cfg4 &= ~ADS_CFG4_LOFF_COMP_EN;
  return writeReg(ADS_REG_CONFIG4, cfg4);
}

bool ADS1299Plus::setBiasDeriveP(uint8_t chMask)
{
  return writeReg(ADS_REG_BIAS_SENSP, clipMask_(chMask));
}

bool ADS1299Plus::setBiasDeriveN(uint8_t chMask)
{
  return writeReg(ADS_REG_BIAS_SENSN, clipMask_(chMask));
}

// ---- Lectura de frames ----
bool ADS1299Plus::readFrameRDATAC(uint32_t &status24, int32_t *chOut, size_t capacity)
{
  if (!rdatacActive_ || chOut == nullptr || capacity < num_channels_)
    return false;

  const uint16_t nbytes = bytesPerFrame();
  if (nbytes > BYTES_PER_FRAME_MAX)
    return false;

  uint8_t rxBuf[BYTES_PER_FRAME_MAX] = {0};
  spi_->select();
  for (uint16_t i = 0; i < nbytes; ++i)
  {
    rxBuf[i] = spi_->xfer(ADS_CMD_NOP);
  }
  spi_->deselect();

  return ADS1299Core::decodeFrame(rxBuf, num_channels_, status24, chOut, capacity);
}

bool ADS1299Plus::readDataOnDemand(uint32_t &status24, int32_t *chOut, size_t capacity)
{
  if (rdatacActive_ || chOut == nullptr || capacity < num_channels_)
    return false;

  const uint16_t nbytes = bytesPerFrame();
  if (nbytes > BYTES_PER_FRAME_MAX)
    return false;

  cmdRDATA();

  uint8_t rxBuf[BYTES_PER_FRAME_MAX] = {0};
  spi_->select();
  for (uint16_t i = 0; i < nbytes; ++i)
  {
    rxBuf[i] = spi_->xfer(ADS_CMD_NOP);
  }
  spi_->deselect();

  return ADS1299Core::decodeFrame(rxBuf, num_channels_, status24, chOut, capacity);
}

bool ADS1299Plus::readDeviceID(uint8_t &id)
{
  return readReg(ADS_REG_ID, id);
}
