/*
 * ADS1299_ArduinoHAL.h - Arduino Implementation of HAL
 *
 * Provides an Arduino-specific implementation of the ADS1299_HAL interface.
 * Uses Arduino GPIO, Arduino SPI, and Arduino delay functions.
 *
 * This backend is the Arduino implementation used by the HAL-only
 * ADS1299_Device facade on the portable-core-hal branch.
 */

#ifndef ADS1299_ARDUINO_HAL_H
#define ADS1299_ARDUINO_HAL_H

#include "../hal/ADS1299_HAL.h"
#include <stdint.h>

/**
 * ADS1299_ArduinoHAL - Arduino implementation of the HAL interface
 *
 * This implementation uses:
 * - Arduino SPI library (SPIClass)
 * - Arduino GPIO functions (pinMode, digitalWrite, digitalRead)
 * - Arduino delay functions (delay, delayMicroseconds)
 *
 * Example usage:
 *     ADS1299_ArduinoHAL hal(10, 9, 8, 0xFF, 7);  // CS, START, RESET, PWDN, DRDY
 *     hal.begin();
 *     // Pass HAL to ADS1299_Device.
 *     hal.end();
 */
class ADS1299_ArduinoHAL : public ADS1299_HAL {
public:
    /**
     * Marker for unused pins (when PWDN is not available on the board).
     */
    static constexpr uint8_t PIN_UNUSED = 0xFF;

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
    void begin() override;

    /**
     * Deinitialize the HAL layer.
     * Releases Arduino SPI resources.
     */
    void end() override;

    /**
     * Begin an SPI transaction using Arduino SPISettings.
     *
     * @param config platform-neutral SPI configuration
     */
    void beginTransaction(const ADS1299_SpiConfig& config) override;

    /**
     * End the active SPI transaction.
     */
    void endTransaction() override;

    /**
     * Assert chip select (pull CS LOW).
     */
    void csLow() override;

    /**
     * Deassert chip select (pull CS HIGH).
     */
    void csHigh() override;

    /**
     * SPI transfer using Arduino SPI library.
     *
     * @param data byte to send
     * @return byte received from ADS1299
     */
    uint8_t spiTransfer(uint8_t data) override;

    /**
     * Delay in microseconds using Arduino delayMicroseconds().
     *
     * @param us microseconds to delay
     */
    void delayMicroseconds(uint32_t us) override;

    /**
     * Delay in milliseconds using Arduino delay().
     *
     * @param ms milliseconds to delay
     */
    void delayMilliseconds(uint32_t ms) override;

    /**
     * Control the START pin.
     *
     * @param high true = HIGH, false = LOW
     */
    void setStart(bool high) override;

    /**
     * Control the RESET pin.
     *
     * @param high true = HIGH, false = LOW
     */
    void setReset(bool high) override;

    /**
     * Control the PWDN (power down) pin.
     * Does nothing if PIN_UNUSED was specified in constructor.
     *
     * @param high true = HIGH (normal operation), false = LOW (power down)
     */
    void setPwdn(bool high) override;

    /**
     * Read the DRDY (data ready) pin.
     *
     * @return true if DRDY is HIGH, false if DRDY is LOW
     */
    bool readDrdy() override;

private:
    uint8_t m_csPin;
    uint8_t m_startPin;
    uint8_t m_resetPin;
    uint8_t m_pwdnPin;
    uint8_t m_drdyPin;
};

#endif // ADS1299_ARDUINO_HAL_H
