#ifndef ARDUINO_H
#define ARDUINO_H

#include <stddef.h>
#include <stdint.h>

#define HIGH 0x1
#define LOW  0x0

#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2

#define HEX 16

static const uint8_t SCK = 13;
static const uint8_t MOSI = 11;
static const uint8_t MISO = 12;

inline void pinMode(uint8_t, uint8_t) {}
inline void digitalWrite(uint8_t, uint8_t) {}
inline int digitalRead(uint8_t) { return HIGH; }
inline void delay(unsigned long) {}
inline void delayMicroseconds(unsigned int) {}

#endif // ARDUINO_H
