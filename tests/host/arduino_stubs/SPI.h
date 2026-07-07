#ifndef SPI_H
#define SPI_H

#include <stdint.h>

static const uint8_t MSBFIRST = 1;
static const uint8_t LSBFIRST = 0;

static const uint8_t SPI_MODE0 = 0;
static const uint8_t SPI_MODE1 = 1;
static const uint8_t SPI_MODE2 = 2;
static const uint8_t SPI_MODE3 = 3;

class SPISettings {
public:
    SPISettings(uint32_t clockHz, uint8_t bitOrder, uint8_t mode)
        : clockHz_(clockHz), bitOrder_(bitOrder), mode_(mode) {}

    uint32_t clockHz_;
    uint8_t bitOrder_;
    uint8_t mode_;
};

class SPIClass {
public:
    void begin() {}
    void end() {}
    void beginTransaction(const SPISettings&) {}
    void endTransaction() {}
    uint8_t transfer(uint8_t) { return 0; }
};

extern SPIClass SPI;

#endif // SPI_H
