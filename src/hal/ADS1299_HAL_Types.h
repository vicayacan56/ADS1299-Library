/*
 * ADS1299_HAL_Types.h - Portable HAL types
 *
 * Defines platform-neutral types used by the ADS1299 HAL interface.
 * Platform backends translate these values to their native SPI/GPIO APIs.
 */

#ifndef ADS1299_HAL_TYPES_H
#define ADS1299_HAL_TYPES_H

#include <stdint.h>

enum class ADS1299_SpiBitOrder : uint8_t {
    MSB_FIRST = 0,
    LSB_FIRST = 1
};

enum class ADS1299_SpiMode : uint8_t {
    MODE0 = 0,
    MODE1 = 1,
    MODE2 = 2,
    MODE3 = 3
};

enum class ADS1299_GpioLevel : uint8_t {
    LOW = 0,
    HIGH = 1
};

struct ADS1299_SpiConfig {
    uint32_t clockHz = 2048000;
    ADS1299_SpiBitOrder bitOrder = ADS1299_SpiBitOrder::MSB_FIRST;
    ADS1299_SpiMode mode = ADS1299_SpiMode::MODE1;
};

#endif // ADS1299_HAL_TYPES_H
