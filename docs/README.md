# ADS1299Plus Documentation

This documentation is intentionally small.

Use this index to choose the shortest path:

- [User Guide](user/user-guide.md): install the library, wire the ADS1299, and use the examples.
- [HAL Usage Guide](user/hal-usage-guide.md): understand the `ADS1299_Device` plus `ADS1299_ArduinoHAL` path.
- [Testing Without Hardware](user/testing-without-hardware.md): compile examples and run host-side tests.
- [HAL Branch Status](architecture/hal-branch-status.md): current branch state and next work.
- [HAL Backend Porting Guide](architecture/hal-backend-porting-guide.md): how to add STM32, ESP-IDF, Zephyr, or bare-metal backends.

## Current Public Path

```text
ADS1299_Device
  -> ADS1299_Protocol
  -> ADS1299_HAL
  -> ADS1299_ArduinoHAL
```

Arduino is currently the validated backend. Other backends should be added one at a time.
