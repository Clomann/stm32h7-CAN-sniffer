## comm module
### Comm module driver factory pattern
Is used to realize dependency inversion and the open-closed principle so that extension with new drivers needs little changes to the existing code base.

Different approached for the driver registration are possible:
- linker section registry where drivers are added at linking time in the ".comm_factory" section
- static list in separate file
- array in RAM with dynamic registration at init runtime
- usage of weak-symbol switch (core needs to be patched for each new driver)

Decision:
- usage of linker section registry

Reason:
- The waek-symbo switch would be more safety friendly:
    - static analysing is possible (also no funciton pointers used to register a driver as with the static list approach)
    - almost not dependant on toolchain (like linker section approach)
    - explicit in which driver is supported and registered
    - deterministic (unlike runtime regisrtaion)

But the linker-section registry is a good opportunity to apply and learn about the factory and opaque design patterns and a discovery mechanism similar to those used by OSs.

## Drivers

### SPI Ring Buffer Data Management

| Field | Description |
|-------|-------------|
| **Title** | SPI Ring Buffer Data Management Pattern |
| **Status** | Decided |
| **Context** | SPI driver cannot guarantee `msg` pointer remains valid after `SPI_Send()` returns, but DMA executes asynchronously. Two patterns considered:<br/>1. direct pointer assignment (`slot->data = msg`)<br/>2.  copy-based (`memcpy(slot->data, msg, len)`) |
| **Decision** | Use existing `ring_buffer_put()` function. Data must always be copied into ring buffer slots before DMA operations. Reserve pattern rejected for `SPI_Send()` as redundant since it requires manual memcpy anyway.|
| **Consequences** | **Positive:** Memory safe for async DMA, thread safe atomic operations, API flexibility for callers<br/>**Negative:** Required memcpy overhead, pre-allocated slot memory needed |