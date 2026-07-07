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
    pinMode(m_pwdnPin, OUTPUT);
    
    // Configure DRDY pin as input
    pinMode(m_drdyPin, INPUT);
    
    // Start with CS HIGH (inactive)
    csHigh();
    
    // Start with START LOW (no conversion)
    setStart(false);
    
    // Start with RESET LOW (not resetting)
    setReset(false);
    
    // Start with PWDN HIGH (normal operation)
    setPwdn(true);
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
 */
void ADS1299_ArduinoHAL::setPwdn(bool high)
{
    digitalWrite(m_pwdnPin, high ? HIGH : LOW);
}

/**
 * Read DRDY pin
 */
bool ADS1299_ArduinoHAL::readDrdy()
{
    return digitalRead(m_drdyPin) == HIGH;
}
