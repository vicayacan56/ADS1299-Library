/*
 * ADS1299_HAL_Types.h - Portable HAL types
 *
 * Defines platform-neutral types used by the ADS1299 HAL interface.
 * Platform backends translate these values to their native SPI/GPIO APIs.
 */

#ifndef ADS1299_HAL_TYPES_H
#define ADS1299_HAL_TYPES_H

#include <stdint.h>

enum class ADS1299_SpiBitOrder {
    MsbFirst,
    LsbFirst
};

enum class ADS1299_SpiMode {
    Mode0,
    Mode1,
    Mode2,
    Mode3
};

enum class ADS1299_GpioLevel {
    Low,
    High
};

struct ADS1299_SpiConfig {
    uint32_t clockHz;
    ADS1299_SpiBitOrder bitOrder;
    ADS1299_SpiMode mode;
};

#endif // ADS1299_HAL_TYPES_H
