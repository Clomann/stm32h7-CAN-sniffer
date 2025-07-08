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

## Frame logging
### FreeRTOS tasks
