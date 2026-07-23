/*
 * ADS1299_HAL.h - Hardware Abstraction Layer Interface
 *
 * Defines the minimal interface that the ADS1299 driver needs from the platform.
 * Platform-specific implementations (Arduino, STM32 HAL, ESP-IDF, etc.) must
 * provide a class derived from this interface.
 *
 * ADS1299Plus keeps the classic Arduino/SafeSPI path as the default user path,
 * and also supports an optional HAL-backed constructor for portability work.
 */

#ifndef ADS1299_HAL_H
#define ADS1299_HAL_H

#include "ADS1299_HAL_Types.h"
#include <stdint.h>

/**
 * ADS1299_HAL - Abstract base class for hardware operations
 *
 * Any platform-specific implementation must derive from this class and
 * provide implementations for all virtual methods.
 */
class ADS1299_HAL {
public:
    virtual ~ADS1299_HAL() = default;

    /**
     * Initialize the HAL layer.
     * Called once at startup.
     * Sets up SPI, GPIO pins, and other platform-specific resources.
     */
    virtual void begin() = 0;

    /**
     * Deinitialize the HAL layer.
     * Called on shutdown.
     * Releases SPI, GPIO pins, and other platform-specific resources.
     */
    virtual void end() = 0;

    /**
     * Begin an SPI transaction using platform-neutral settings.
     *
     * @param config SPI clock, bit order, and mode
     */
    virtual void beginTransaction(const ADS1299_SpiConfig& config) = 0;

    /**
     * End the active SPI transaction.
     */
    virtual void endTransaction() = 0;

    /**
     * Assert chip select (pull CS LOW).
     * The ADS1299 is active when CS is low.
     */
    virtual void csLow() = 0;

    /**
     * Deassert chip select (pull CS HIGH).
     * The ADS1299 is inactive when CS is high.
     */
    virtual void csHigh() = 0;

    /**
     * SPI transfer: send one byte, receive one byte.
     * Blocking operation.
     *
     * @param data byte to send
     * @return byte received from ADS1299
     */
    virtual uint8_t spiTransfer(uint8_t data) = 0;

    /**
     * Delay in microseconds.
     * Non-blocking or blocking depending on platform.
     *
     * @param us microseconds to delay
     */
    virtual void delayMicroseconds(uint32_t us) = 0;

    /**
     * Delay in milliseconds.
     * Non-blocking or blocking depending on platform.
     *
     * @param ms milliseconds to delay
     */
    virtual void delayMilliseconds(uint32_t ms) = 0;

    /**
     * Control the START pin.
     * Used to start/stop data conversion.
     *
     * @param high true = HIGH, false = LOW
     */
    virtual void setStart(bool high) = 0;

    /**
     * Control the RESET pin.
     * Used to perform a hardware reset.
     *
     * @param high true = HIGH, false = LOW
     */
    virtual void setReset(bool high) = 0;

    /**
     * Control the PWDN (power down) pin.
     * Used to enable/disable the device.
     *
     * @param high true = HIGH (normal operation), false = LOW (power down)
     */
    virtual void setPwdn(bool high) = 0;

    /**
     * Read the DRDY (data ready) pin.
     * The ADS1299 pulls this pin low when data is ready.
     *
     * @return true if DRDY is HIGH (no data), false if DRDY is LOW (data ready)
     */
    virtual bool readDrdy() = 0;
};

#endif // ADS1299_HAL_H
