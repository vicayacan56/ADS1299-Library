/*
 * ADS1299_ArduinoHAL.h - Arduino Implementation of HAL
 *
 * Provides an Arduino-specific implementation of the ADS1299_HAL interface.
 * Uses Arduino GPIO, Arduino SPI, and Arduino delay functions.
 *
 * This is a conservative Phase B1 skeleton. The ADS1299Plus driver is not yet
 * refactored to use this HAL; it still uses Arduino APIs directly.
 *
 * Phase B1: HAL implementation only. Integration comes in Phase B2+.
 */

#ifndef ADS1299_ARDUINO_HAL_H
#define ADS1299_ARDUINO_HAL_H

#include "ADS1299_HAL.h"
#include <stdint.h>

/**
 * ADS1299_ArduinoHAL - Arduino implementation of the HAL interface
 *
 * This implementation uses:
 * - Arduino SPI library (SPIClass)
 * - Arduino GPIO functions (pinMode, digitalWrite, digitalRead)
 * - Arduino delay functions (delay, delayMicroseconds)
 *
 * Example usage (Phase B2+):
 *     ADS1299_ArduinoHAL hal(10, 9, 8, 7);  // CS, START, RESET, PWDN
 *     hal.begin();
 *     // later, pass HAL to ADS1299Plus core
 *     hal.end();
 */
class ADS1299_ArduinoHAL : public ADS1299_HAL {
public:
    /**
     * Constructor.
     *
     * @param csPin     Arduino pin number for chip select (CS)
     * @param startPin  Arduino pin number for START control
     * @param resetPin  Arduino pin number for RESET control
     * @param pwdnPin   Arduino pin number for PWDN (power down) control
     * @param drdyPin   Arduino pin number for DRDY (data ready) input
     */
    ADS1299_ArduinoHAL(
        uint8_t csPin,
        uint8_t startPin,
        uint8_t resetPin,
        uint8_t pwdnPin,
        uint8_t drdyPin
    );

    /**
     * Destructor.
     */
    virtual ~ADS1299_ArduinoHAL();

    /**
     * Initialize the HAL layer.
     * Sets up SPI and GPIO pins for Arduino platform.
     */
    virtual void begin();

    /**
     * Deinitialize the HAL layer.
     * Currently a no-op for Arduino, but reserved for future use.
     */
    virtual void end();

    /**
     * Assert chip select (pull CS LOW).
     */
    virtual void csLow();

    /**
     * Deassert chip select (pull CS HIGH).
     */
    virtual void csHigh();

    /**
     * SPI transfer using Arduino SPI library.
     *
     * @param data byte to send
     * @return byte received from ADS1299
     */
    virtual uint8_t spiTransfer(uint8_t data);

    /**
     * Delay in microseconds using Arduino delayMicroseconds().
     *
     * @param us microseconds to delay
     */
    virtual void delayMicroseconds(uint32_t us);

    /**
     * Delay in milliseconds using Arduino delay().
     *
     * @param ms milliseconds to delay
     */
    virtual void delayMilliseconds(uint32_t ms);

    /**
     * Control the START pin.
     *
     * @param high true = HIGH, false = LOW
     */
    virtual void setStart(bool high);

    /**
     * Control the RESET pin.
     *
     * @param high true = HIGH, false = LOW
     */
    virtual void setReset(bool high);

    /**
     * Control the PWDN (power down) pin.
     *
     * @param high true = HIGH (normal operation), false = LOW (power down)
     */
    virtual void setPwdn(bool high);

    /**
     * Read the DRDY (data ready) pin.
     *
     * @return true if DRDY is HIGH, false if DRDY is LOW
     */
    virtual bool readDrdy();

private:
    uint8_t m_csPin;
    uint8_t m_startPin;
    uint8_t m_resetPin;
    uint8_t m_pwdnPin;
    uint8_t m_drdyPin;
};

#endif // ADS1299_ARDUINO_HAL_H
