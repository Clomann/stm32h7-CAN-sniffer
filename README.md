<div align="center">
  <img src="app/services/http/fs/pages/files/logo.png" alt="CAN Sniffer Logo" width="120">
  <h1>STM32H7 CAN sniffer</h1>
  <p><em>Dual-channel CAN logger with HTTP config UI and SD persistence</em></p>
</div>

[![Build](https://github.com/Clomann/stm32h7-CAN-sniffer/actions/workflows/unity-tests.yml/badge.svg?branch=main&event=push)](https://github.com/Clomann/stm32h7-CAN-sniffer/actions/workflows/unity-tests.yml)

This project implements the software for a device that logs CAN traffic on two CAN channels and stores the data on an SD card. The device can be configured and the logged data can be accessed via a simple built-in web GUI.

**Project recap**

- Role: solo firmware engineer (FreeRTOS + STM32H745)
- Built: dual-channel CAN logger with 3-stage buffering, on-board HTTP config UI, SD persistence
- Result: sustained 2×1 Mbit/s capture for 6 h with zero frame loss ([losslessness tests](doc/measurements/losslessnes/)); SD wear limited via rotating logs
- Tooling: gcc/FreeRTOS, custom comm drivers, lightweight web stack

**Table of contents**



# Features

The CAN sniffer can log CAN frames on two channels with the following specs:

- continuous logging at 2x 1 Mbit/s @ 100% bus load (e.g., ~19920 frames/s) using three-stage buffering
- supports classic and extended CAN IDs and message filtering
- rotating log file on an SD card in a way that keeps memory wear low
- JSON-based configuration persistence
- HTTP API that powers the built-in web GUI to:
    - control and configure the CAN logger
    - download of logged data
    - manage SD card logging (format on next restart, log file size/count, cluster size)
    - client-side log data parser
    - in-app-programming to update the firmware via web GUI

**Web GUI**

The *CAN setup* page allows you to configure the CAN sniffer and start/stop logging. It also includes SD card management to queue a format on the next restart and adjust log ring settings.

![CAN Trace screen](doc/images/webGUI/STM32H7%20CAN%20Sniffer%20-%20Setup.pdf.png)

Through the *CAN trace* page you can download logged data as a binary file and decode it using the *Parse and Display Log* function:

![CAN Setup screen](doc/images/webGUI/STM32H7%20CAN%20Sniffer%20-%20Trace.pdf.png)

## Measurements

Detailed measurement setup, plots, and validation results (CAN ISR latency, SD write timings, lossless logging validation, stress test) live in the architecture doc: [Measurements](doc/arc42-template-EN.md#measurements).

# Quick start

The project targets a NUCLEO-144 STM32H745ZI discovery board. It uses GPIOs to interface with:
- SD card
- CAN transceiver
- the on-board ETH interface

It is built using CMake and Make with the GCC toolchain.

This section explains step by step how to build, test and download the firmware using the tool configuration included in this project.

## System overview

The system uses the following components:
- NUCLEO-H745ZI-Q STM32 board
- two TJA1050 based CAN transceiver breakout boards
- a 3 V micro SD card adapter breakout board

![System overview](doc/images/system_overview/system_overview.drawio.jpg)
*Figure: The image shows a simplified overview of the system components.*

More details of the system's technical context and a picture of the prototype build can be found [here](doc/arc42-template-EN.md#technical-context)

> **NOTE**: Use a high quality SD card because cheap ones can have reliability issues when used with SPI (e.g., an SD card initializes correctly and some writes/reads work but then it hangs until power cycled for no apparent reason). The system was tested with a Kingston industrial-grade card.

## Prerequisites

The following tool versions are used to develop, debug and run the program on the target:

| Component | Version | Scope |
|-|-|-|
| cmake | 3.28.1 | build |
| make | 3.81 | build |
| GNU Tools for STM32 | 13.3.1 | build/ debug |
| Python | 3.x | build (image signing) |
| host C compiler (gcc or clang) | | unit tests |
| OpenOCD | 0.12.0 | download/ debug |
| STM32CubeProgrammer CLI | | download |
| Cppcheck | 2.17.1 | develop |
| clang-format | 18.1.8 | develop |

`arm-none-eabi-gcc`, `cmake`, `make` and `python3` must be on your `PATH`.

## Build step by step

Run all commands from the repo's root directory.

### 1. Clone the repository and its submodules

The STM32 HAL and several libraries are git submodules. CMake does not initialize them yet:

```sh
git clone https://github.com/Clomann/stm32h7-CAN-sniffer.git
cd stm32h7-CAN-sniffer

git submodule sync --recursive
git submodule update --init --depth 1 libs/STM32_HAL/STM32H7
git submodule update --init --recursive libs/coreJSON
git submodule update --init libs/lwip-tcpip
git submodule update --init --recursive libs/mbedtls
git submodule update --init --depth 1 libs/mcuboot
```

> **NOTE**: Do not use `git clone --recursive` or `git submodule update --init --recursive` for all submodules: this also pulls nested submodules (e.g. of MCUboot) whose sources break the build.

FreeRTOS and lwrb are fetched by CMake during the first configure.

### 2. Install the Python dependencies

The build signs and encrypts the application image with MCUboot's `imgtool`, which needs a few Python packages. Install them into a virtual environment and keep it activated for the next steps:

```sh
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install -r requirements.txt
```

### 3. Generate the MCUboot keys

The bootloader only boots application images signed with its key. Keys are kept in `.keys/` and are never committed, so create your own once:

```sh
mkdir -p .keys
python3 ./libs/mcuboot/scripts/imgtool.py keygen -k .keys/sign-ecdsa-p256.priv.pem -t ecdsa-p256
python3 ./libs/mcuboot/scripts/imgtool.py keygen -k .keys/enc-ecies-p256.priv.pem -t ecdsa-p256
python3 ./libs/mcuboot/scripts/imgtool.py getpub -k .keys/enc-ecies-p256.priv.pem -e pem -o .keys/enc-ecies-p256.pub.pem
```

Without these keys, all CM7 presets fail during configure with `Signing key PEM not found`. Bootloader and application must be built with the same keys. See [CONTRIBUTING.md](CONTRIBUTING.md#sign-and-encrypt-image-using-mcuboot) for details.

### 4. Build the firmware

Build the CM7 bootloader, the CM7 application and the CM4 application:

```sh
cmake --fresh --preset Release_CM7_Boot -B ./build/bootloader && make -C ./build/bootloader -B -j
cmake --fresh --preset Release_CM7 -B ./build/application && make -C ./build/application -B -j
cmake --fresh --preset Release_CM4 -B ./build/application && make -C ./build/application -B -j
```

The first configure takes a while because FreeRTOS and lwrb are downloaded. The CM7 and CM4 applications share the build directory `build/application`, so `--fresh` is required when switching between them. All binaries are written to `_bin/Release/`.

| Configure preset | Builds |
|-|-|
| `Release_CM7_Boot` | CM7 bootloader (MCUboot) |
| `Release_CM7` | CM7 application, signed and encrypted |
| `Release_CM4` | CM4 application |
| `Pre_Release_CM7` | CM7 application for the soak test before a release (see [CONTRIBUTING.md](CONTRIBUTING.md#soak-test)) |

`Pre_Release_CM7` also writes to `_bin/Release/` and replaces the application image of `Release_CM7`.

Alternatively, the workflow presets `release-cm7-boot`, `release-cm7`, `release-cm4` and `pre-release-test` configure and build in one step, each in its own build directory `build/<preset>`, e.g. `cmake --workflow --preset release-cm7`. See [CONTRIBUTING.md](CONTRIBUTING.md#build-presets) for all presets and their options.

### 5. Run the unit tests

The unit tests run on the host and need the CM7 application image from step 4:

```sh
cmake -S tests --preset tests-default
cmake --build tests/build/tests-default -j
ctest --test-dir tests/build/tests-default --output-on-failure
```

Use the preset `tests-encryption` to also run the bootloader tests with image encryption enabled.

Alternatively, the workflow presets configure and build the tests in one step. They do not run the tests, so call `ctest` afterwards:

```sh
(cd tests && cmake --workflow --preset tests-default-workflow)
ctest --test-dir tests/build/tests-default --output-on-failure
```

See [CONTRIBUTING.md](CONTRIBUTING.md#testing) for details.

### 6. Flash the firmware

Flash the bootloader, the signed CM7 application and the CM4 application:

```sh
STM32_Programmer_CLI -c port=SWD -d _bin/Release/SPI_FullDuplex_ComDMA_CM7_boot.elf 0x08000000 -rst
STM32_Programmer_CLI -c port=SWD -d _bin/Release/SPI_FullDuplex_ComDMA_CM7_app-signed.bin 0x08020000 -rst
STM32_Programmer_CLI -c port=SWD -d _bin/Release/SPI_FullDuplex_ComDMA_CM4_app.elf 0x08100000 -rst
```

In VS Code, the task `Flash CM7 & CM4 (Release)` does the same (see [CONTRIBUTING.md](CONTRIBUTING.md#flashing-and-debugging-in-vs-code)).

## Verify logging

See [Lossless logging validation](doc/arc42-template-EN.md#lossless-logging-validation) for the end-to-end verification workflow.

## Access the web GUI

The CAN sniffer uses Multicast DNS (mDNS) and Internet Group Management Protocol (IGMP) so that its hostname can be discovered. If your network supports mDNS you can access the CAN sniffer via the following hostname:

> http://can-sniffer.local

Otherwise, the IP address is assigned to the CAN sniffer by the DHCP server of your network.
You have to identify the IP address manually via the MAC address:

> 00:80:E1:00:00:00

# Architecture

This is a brief overview of the architecture. You can read the full architecture documentation [HERE](doc/arc42-template-EN.md).

## Software 

The driver design is inspired by Linux's opaque‑ops and linker‑list registration patterns (SPI, CAN, Ethernet), and the firmware runs bare-metal
on FreeRTOS. Task orchestration is realized by prioritized preemptive task scheduling. 

At a high level the firmware separates drivers, a multistage buffering pipeline, storage, and configuration into distinct layers; the full breakdown (logging pipeline, config workflow, hooks) lives in the [arc42 building-block view](doc/arc42-template-EN.md#building-block-view). Storage modules rely on overridable hooks for error handling—see the [architecture decisions](doc/arc42-template-EN.md#error-handling-hooks-for-storage-subsystems) before integrating.

The full arc42-template-based document (doc/arc42-template-EN.md) covers the following in more detail:

- building blocks (modules, interfaces)
- runtime view (ISRs, queues)
- quality scenarios (SD write latency, CAN Rx latency)

Read it [here](doc/arc42-template-EN.md).

## Hardware

To get the system running, you need to connect the necessary adapters to the NUCLEO-144 board. 

Following diagram shows which pins are connected to which external components:



| Peripheral | Pins |
| - | - |
| CAN 1 | Rx: PB12, Tx: PB6 |
| CAN 2 | Rx: PA11, Tx: PA12 |
| SPI 1 | SS: PA4, CLK: PA5, MISO: PA6, MOSI: PD11 |
| ETH | MDC: PC1, REF_CLK: PA1, MDIO: PA2, CRS_DV: PA7, RXD0: PC4, RXD1: PC5, TXD1: PB13, TX_EN: PG11, TXD0: PG13 |
| SDMMC1 (for future extension) | D6: PC6, D7: PC7, D0: PC8, D1: PC9, D2: PC10, D3: PC11, CK: PC12, CMD: PD2, CLKIN: PB8, CDIR: PB9 |
|  |  |

```mermaid
flowchart TD
    subgraph STM32H7 [STM32H7]
        subgraph ETH [ETH MAC]
            MAC_MDC["MDC<br>(PC1)"]
            MAC_REF_CLK["REF_CLK<br>(PA1)"]
            MAC_MDIO["MDIO<br>(PA2)"]
            MAC_CRS_DV["CRS_DV<br>(PA7)"]
            MAC_RXD0["RXD0<br>(PC4)"]
            MAC_RXD1["RXD1<br>(PC5)"]
            MAC_TXD1["TXD1<br>(PB13)"]
            MAC_TX_EN["TX_EN<br>(PG11)"]
            MAC_TXD0["TXD0<br>(PG13)"]
        end
        
        subgraph SPI1 [SPI1 - Master]
                SPI1_SS["SS (PA4)<br>CN7.D13"]
                SPI1_CLK["CLK (PA5)<br>CN7.D13"]
                SPI1_MISO["MISO (PA6)<br>CN7.D12"]
                SPI1_MOSI["MOSI (PB5)<br>CN7.D11"]
        end

        subgraph CAN1 [CAN 1]
            CAN1_Tx["Tx<br>(B9)"]
            CAN1_Rx["Rx<br>(B8)"]
        end

        subgraph CAN2 [CAN 2]
            CAN2_Tx["Tx<br>(B6)"]
            CAN2_Rx["Rx<br>(B12)"]
        end
    end

    subgraph PHY["ETH - PHY (on board)"]
        PHY_MDC
        PHY_REF_CLK
        PHY_MDIO
        PHY_CRS_DV
        PHY_RXD0
        PHY_RXD1
        PHY_TXD1
        PHY_TX_EN
        PHY_TXD0
    end

    subgraph SDCARD [SD card]
        SPI4_SS["SS (PE11)"]
        SPI4_CLK["CLK (PE12)<br>CN10.D39"]
        SPI4_MISO["MISO (PE13)<br>CN10.D3"]
        SPI4_MOSI["MOSI (PE14)<br>CN10.D4"]
    end

    subgraph CAN1TRCV [CAN 1 Tranceiver]
        CAN1TRANC_Tx["CAN Tx"]
        CAN1TRANC_Rx["CAN Rx"]
    end

    subgraph CAN2TRCV [CAN 2 Tranceiver]
        CAN2TRANC_Tx["CAN Tx"]
        CAN2TRANC_Rx["CAN Rx"]
    end

    %% Connections with directions
    MAC_MDC     --- PHY_MDC
    MAC_REF_CLK --- PHY_REF_CLK
    MAC_MDIO    --- PHY_MDIO
    MAC_CRS_DV  --- PHY_CRS_DV
    MAC_RXD0    --- PHY_RXD0
    MAC_RXD1    --- PHY_RXD1
    MAC_TXD1    --- PHY_TXD1
    MAC_TX_EN   --- PHY_TX_EN
    MAC_TXD0    --- PHY_TXD0

    SPI1_SS --- |SS| SPI4_SS
    SPI1_CLK --- |CLK| SPI4_CLK
    SPI1_MOSI --- |MOSI| SPI4_MOSI

    CAN1_Tx --- |Tx| CAN1TRANC_Tx
    CAN1_Rx --- |Rx| CAN1TRANC_Rx

    CAN2_Tx --- |Tx| CAN2TRANC_Tx
    CAN2_Rx --- |Rx| CAN2TRANC_Rx

    SPI1_MISO --- |MISO| SPI4_MISO

```
> _Performance chart placeholder – add logic-analyzer capture showing CAN ISR latency and SD write timing (e.g., 1 Mbit/s @ 100 % bus load, block flush duration). See [arc42 measurement instrumentation](doc/arc42-template-EN.md#measurement-instrumentation) for the exact probe points._

# Contributing

Contributor workflows and tooling live in `CONTRIBUTING.md`.

# Future work

Planned features are:
- Wi-Fi extension
- SDIO interface to allow for lower-quality SD cards
- external Realtime-Clock (RTC)
- support to convert to various CAN log formats via GUI (e.g. Vector ASCII)
- for better performance and higher availability the serving of requests via ethernet shall be executed on another core to not interfere with the CAN trace logging when large files are loaded for user downloads.
