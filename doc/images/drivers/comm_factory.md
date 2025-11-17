``` mermaid
---
title: Comm module factory
---
classDiagram
    class CommManager {
        +CommManager_Init(CommDriver* drv, CommDriverConfigType* cfg, size_t cfg_size, uint8_t* tx, uint8_t* rx)
        -- looks up constructor --
    }
    class CommFactory {
        <<singleton>>
        +CommFactory_Find(CommProtocolType proto)
        +__start_comm_factory
        +__stop_comm_factory
    }
    class CommFactoryEntry {
        +protocol : CommProtocolType
        +create() : Comm_DriverCreate
    }
    CommManager --> CommFactory : ask for constructor
    CommFactory "1" --> "*" CommFactoryEntry : registry @ .comm_factory
    class FDCAN_Driver {
        «macro» COMM_REGISTER_DRIVER
        +FDCAN_CreateDriver(CommDriver* drv, CommDriverConfigType* cfg, size_t cfg_size, uint8_t* tx, uint8_t* rx)
        -FDCAN_Init(CommDriver* drv)
        -FDCAN_Send(CommDriver* drv, void* msg)
        -FDCAN_Read(CommDriver* drv, void* msg, uint8_t len, uint32_t timeout)
        -FDCAN_Deinit(CommDriver* drv)
        ---
        creates FdcanDeviceType instance
    }
    class SPI_Driver {
        «macro» COMM_REGISTER_DRIVER
        +SPI_CreateDriver(CommDriver* drv, CommDriverConfigType* cfg, size_t cfg_size, uint8_t* tx, uint8_t* rx)
        -SPI_Init(CommDriver* drv)
        -SPI_Send(CommDriver* drv, void* msg)
        -SPI_Read(CommDriver* drv, void* msg, uint8_t len, uint32_t timeout)
        -SPI_Deinit(CommDriver* drv)
        ---
        creates SpiInstanceType instance
    }
    FDCAN_Driver --o CommFactoryEntry : static obj<br>in .comm_factory
    SPI_Driver --o CommFactoryEntry : static obj<br>in .comm_factory
    note for CommFactoryEntry "All entries placed by<br>'COMM_REGISTER_DRIVER'<br>into a dedicated linker<br>section; the linker emits<br>__start/__stop symbols."
    note for CommManager "Init code never changes—<br>core depends only on the<br>abstract factory.<br>Populates CommDriver struct<br>and calls protocol-specific<br>CreateDriver function."
    note for FDCAN_Driver "Create function allocates<br>FdcanDeviceType and assigns<br>to CommDriver->instance.<br>Also sets CommDriver->interface<br>to CommHandle vtable."
    note for SPI_Driver "Create function allocates<br>SpiInstanceType and assigns<br>to CommDriver->instance.<br>Also sets CommDriver->interface<br>to CommHandle vtable."
``` 