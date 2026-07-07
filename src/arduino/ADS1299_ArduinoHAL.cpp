/*
 * ADS1299_ArduinoHAL.cpp - Arduino Implementation of HAL
 *
 * Implementation of the ADS1299_HAL interface using Arduino APIs.
 *
 * This is a conservative Phase B1 skeleton. The ADS1299Plus driver is not yet
 * refactored to use this HAL; it still uses Arduino APIs directly.
 *
 * Phase B1: HAL implementation only. Integration comes in Phase B2+.
 */

#include "ADS1299_ArduinoHAL.h"
#include <Arduino.h>
#include <SPI.h>

static decltype(MSBFIRST) toArduinoBitOrder(ADS1299_SpiBitOrder bitOrder)
{
    switch (bitOrder) {
    case ADS1299_SpiBitOrder::LSB_FIRST:
        return LSBFIRST;
    case ADS1299_SpiBitOrder::MSB_FIRST:
    default:
        return MSBFIRST;
    }
}

static decltype(SPI_MODE0) toArduinoSpiMode(ADS1299_SpiMode mode)
{
    switch (mode) {
    case ADS1299_SpiMode::MODE1:
        return SPI_MODE1;
    case ADS1299_SpiMode::MODE2:
        return SPI_MODE2;
    case ADS1299_SpiMode::MODE3:
        return SPI_MODE3;
    case ADS1299_SpiMode::MODE0:
    default:
        return SPI_MODE0;
    }
}

/**
 * Constructor - store pin assignments
 */
ADS1299_ArduinoHAL::ADS1299_ArduinoHAL(
    uint8_t csPin,
    uint8_t startPin,
    uint8_t resetPin,
    uint8_t pwdnPin,
    uint8_t drdyPin
)
    : m_csPin(csPin),
      m_startPin(startPin),
      m_resetPin(resetPin),
      m_pwdnPin(pwdnPin),
      m_drdyPin(drdyPin)
{
}

/**
 * Destructor
 */
ADS1299_ArduinoHAL::~ADS1299_ArduinoHAL()
{
}

/**
 * Initialize SPI and GPIO pins for Arduino
 */
void ADS1299_ArduinoHAL::begin()
{
    // Initialize SPI library with default settings
    // The application may have already called SPI.begin(),
    // so we just configure the pins we need
    
    // Configure GPIO pins as outputs
    pinMode(m_csPin, OUTPUT);
    pinMode(m_startPin, OUTPUT);
    pinMode(m_resetPin, OUTPUT);
    
    // Configure PWDN pin only if it's assigned (not PIN_UNUSED)
    if (m_pwdnPin != PIN_UNUSED) {
        pinMode(m_pwdnPin, OUTPUT);
    }
    
    // Configure DRDY pin as input with pull-up
    pinMode(m_drdyPin, INPUT_PULLUP);
    
    // Start with CS HIGH (inactive)
    csHigh();
    
    // Start with START LOW (no conversion)
    setStart(false);
    
    // Start with RESET HIGH (inactive; RESET is active-low)
    setReset(true);
    
    // Start with PWDN HIGH (normal operation) if the pin is assigned
    if (m_pwdnPin != PIN_UNUSED) {
        setPwdn(true);
    }
}

/**
 * Deinitialize the HAL layer
 * Currently a no-op for Arduino
 */
void ADS1299_ArduinoHAL::end()
{
    // Reserved for future use (e.g., SPI.end() if needed)
}

/**
 * Begin SPI transaction using Arduino SPISettings
 */
void ADS1299_ArduinoHAL::beginTransaction(const ADS1299_SpiConfig& config)
{
    SPI.beginTransaction(SPISettings(
        config.clockHz,
        toArduinoBitOrder(config.bitOrder),
        toArduinoSpiMode(config.mode)
    ));
}

/**
 * End SPI transaction
 */
void ADS1299_ArduinoHAL::endTransaction()
{
    SPI.endTransaction();
}

/**
 * Assert chip select (pull CS LOW)
 */
void ADS1299_ArduinoHAL::csLow()
{
    digitalWrite(m_csPin, LOW);
}

/**
 * Deassert chip select (pull CS HIGH)
 */
void ADS1299_ArduinoHAL::csHigh()
{
    digitalWrite(m_csPin, HIGH);
}

/**
 * SPI transfer using Arduino SPI.transfer()
 */
uint8_t ADS1299_ArduinoHAL::spiTransfer(uint8_t data)
{
    return SPI.transfer(data);
}

/**
 * Delay in microseconds
 */
void ADS1299_ArduinoHAL::delayMicroseconds(uint32_t us)
{
    ::delayMicroseconds(us);
}

/**
 * Delay in milliseconds
 */
void ADS1299_ArduinoHAL::delayMilliseconds(uint32_t ms)
{
    delay(ms);
}

/**
 * Control START pin
 */
void ADS1299_ArduinoHAL::setStart(bool high)
{
    digitalWrite(m_startPin, high ? HIGH : LOW);
}

/**
 * Control RESET pin
 */
void ADS1299_ArduinoHAL::setReset(bool high)
{
    digitalWrite(m_resetPin, high ? HIGH : LOW);
}

/**
 * Control PWDN pin
 * Does nothing if PIN_UNUSED was specified in constructor.
 */
void ADS1299_ArduinoHAL::setPwdn(bool high)
{
    if (m_pwdnPin == PIN_UNUSED) {
        return;
    }
    digitalWrite(m_pwdnPin, high ? HIGH : LOW);
}

/**
 * Read DRDY pin
 */
bool ADS1299_ArduinoHAL::readDrdy()
{
    return digitalRead(m_drdyPin) == HIGH;
}
