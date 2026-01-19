<div align="center">
  <img src="app/fs/pages/files/logo.png" alt="CAN Sniffer Logo" width="120">
  <h1>STM32H7 CAN sniffer</h1>
  <p><em>Dual-channel CAN logger with HTTP config UI and SD persistence</em></p>
</div>

<!-- [![Build](https://github.com/Clomann/stm32h745-spi-to-microSD/actions/workflows/build.yml/badge.svg?branch=main&event=push)](https://github.com/Clomann/stm32h745-spi-to-microSD/actions/workflows/build.yml) -->


This project implements the software for a device that logs CAN traffic on two CAN channels and stores the data on an SD card. The device can be configured and the logged data can be accessed via a simple built-in web GUI.

**Project recap**

- Role: solo firmware engineer (FreeRTOS + STM32H745)
- Built: dual-channel CAN logger with 3-stage buffering, on-board HTTP config UI, SD persistence
- Result: sustained 2×1 Mbit/s capture for 6 h with zero frame loss; SD wear limited via rotating logs
- Tooling: gcc/FreeRTOS, custom comm drivers, lightweight web stack

**Table of contents**

<!-- TOC -->

- [Features](#features)
    - [Measurements](#measurements)
        - [CAN ISR latency](#can-isr-latency)
        - [SPI/SD timings](#spisd-timings)
            - [SD multi-block write timing](#sd-multi-block-write-timing)
            - [Block flush window logic analyzer](#block-flush-window-logic-analyzer)
        - [Lossless logging validation](#lossless-logging-validation)
        - [Stress test](#stress-test)
- [Quick start](#quick-start)
    - [Run unit tests](#run-unit-tests)
    - [Verify logging](#verify-logging)
    - [System overview](#system-overview)
    - [Prerequisites](#prerequisites)
    - [Build the code](#build-the-code)
    - [Download the code](#download-the-code)
    - [Access the web GUI](#access-the-web-gui)
- [Architecture](#architecture)
    - [Software](#software)
    - [Hardware](#hardware)
    - [Tooling](#tooling)
        - [Documentation tooling](#documentation-tooling)
        - [Formatting tooling](#formatting-tooling)
- [Future work](#future-work)

<!-- /TOC -->

# Features

The CAN sniffer can log CAN frames on two channels with following specs:

- continuous logging at 2x 1 Mbit/s @ 100% busload using three-stage buffering
- supports classic and extended CAN IDs and message filtering
- rotating log file on an SD card in a way that keeps memory wear low
- JSON based config persisting
- HTTP API that supports connecting through built-in web GUI to:
    - control and config the CAN logger 
    - download of logged data
    - manage SD card logging (format on next restart, log file size/count, cluster size)
    - client-side log data parser

**Web GUI**

The *CAN setup* page allows you to configure the CAN sniffer and start/stop logging. It also includes SD card management to queue a format on the next restart and adjust log ring settings.

![CAN Trace screen](doc/images/webGUI/STM32H7%20CAN%20Sniffer%20-%20Setup.pdf.png)

Through the *CAN trace* page you can download logged data as a binary file and decode it through using the funciton in the *Parse and Display Log* function:

![CAN Setup screen](doc/images/webGUI/STM32H7%20CAN%20Sniffer%20-%20Trace.pdf.png)

## Measurements

The input data used for the measurements in this chapter were generated using CANoe and an Interactive Generator node for precise control and analysis of the test data.

To test the reliability of the CAN logger representative test cases are defined. In practice, bus loads are kept below 100 % to reduce contention and keep the traffic deterministic. Therefore, the CAN logger tests are run at a bus load of 85 %.
The actual frame rate is determined by the baud rate and the payload of the frames.

The following figure shows how the data rate depends on the payload length (DLC) assuming all frames have the same length. 
The maximum data-rate to the SD card is determined as described in section [SPI-SD-timings](#spi-sd-timings) with ~0.88 MiB/s (with 8 byte DLC decreasing with frame rate).
Each frame is stored with its meta data (e. g. timestamp, ID, etc.). The diagram shows that storage data rate increases as DLC decreases because the frame rate rises. Storing the frames to storage with dynamic length decreases the needed rate to storage considerably.


![Classic CAN byte and frame rates over the payload length](doc/images/graphs/datarates_anaylsis.py.svg)
*Figure: Classic CAN byte and frame rates over the payload length (DLC) (see [script](doc/images/graphs/datarates_anaylsis.py))*

In conclusion, at 1 Mbit/s and 85% bus load, lower-DLC classic CAN frames provide a realistic long-run test case (>1 h), complemented by short (<15 min) 100% bus load stress runs.

### CAN ISR latency

The FDCAN interrupt service routine empties the hardware buffer and processes the timestamps of the erceived frames. The frames are then put into a light weight first stage software buffer.
The measurements with an oscilloscope (1 GSs/s) shown in the following figure yield an eyballed average of 4 us with a jitter of +-400 ns.

![FDCAN interrupt service routine duration](doc/images/measurements/fdcan_isr_duration_8_bytes_dlc_1Mbits_at_2x85_percent_busload.png)
*Figure: FDCAN interrupt service routine duration with 1 Mbit/s on 2 channels at 85 % bus load.*

Notes:

- jitter and ISR duration can be optimize by placing ISR code and data in ITCM/DTCM and 
avoiding cache misses by keeping the buffer in tightly coupled RAM.

### [SPI/SD timings](#spi-sd-timings)

#### SD multi-block write timing

*See instrumentation in [FileHandler.c](app/FileHandler.c).*

Data is written to the SD card from a staging buffer. The staging buffer is a rotating buffer offering multiple slots. When a slot is full it is written to the SD card in one go. Following image shows the time it takes to write one slot to the SD card on the y axis (including FatFS and SD SPI overhead) over the absolute time passed since the device was powered up (global timestamp).

![SD multi-block write timing](doc/images/measurements/SD_card_write_duration/write_duration_block_1_MBps_85_percent_8_byte_dlc.csv.svg)

The diagram shows samples from 999 consecutive written slots. These slots were filled by test frames sent to CAN 1 and CAN 2 with both in listen-only mode. Thus, the bus load on each channel was a little above 85 % (to prevent error frames during logging tests).

There is a recurring peak to over ~38 000 µs every 224 writes.
Another pattern can be seen reoccuring after every 32 writes where write duration drops below ~34 500 us.
The median write duration is otherwise ~35 590 us.

The median throughput is accordingly: ~ 0.88 MiB/s. 
Logging 2 channels at 100 % bus load at 1 Mbit/s currently results in a datarate of 0.39 MiB/s to the SD card (28 byte per frame total; see [SD card bandwidth script](dev/scripts/sd_card_bandwidth.py)).

Notes:

- Recurring fast pattern every 32 writes (32 KiB chunks -> 1 MiB), likely erase/page alignment.
- Larger spike every 224 writes (7 MiB of data), probably controller cache/maintenance cycle.
- To confirm, query AU_SIZE/ERASE_SIZE via ACMD13; if erase group is 1 MiB the 32‑write cadence fits, if larger (e.g., several MiB) the 224 cadence may reflect the true erase/flush interval.

#### Block flush window (logic analyzer)

The following screenshot taken with a 50 Msps logic analyzer shows the instrumented GPIO toggle at channel 0 and the SPI communication at the 4 remaining channels. 
These measurements are marked in the logic analyzer screenshot:

- M0 spans the 32 KiB multi-block write (~35.6 ms)
- M1 spans flush + idle (~61.7 ms)
- M2 spans only flush (~2.4 ms)

Thus, it also depicts a 32 KiB block write duration of ~35 ms as measured in the previous section.

![Block flush window measurement with logic analyzer](doc/images/measurements/SD_card_write_duration/write_duration_32KiBblock_Logic%208.png)

Furthermore, it can be seen that a lot of time is spent waiting for the SD card to handle the incoming data between multi-block writes, as the following figure shows:

![Block flush window measurement detail with logic analyzer](doc/images/measurements/SD_card_write_duration/write_detail_multi-block_write_Logic%208.png)

### Lossless logging validation

Frame counters and embedded sequence IDs are captured on both CANoe and the firmware. Frame counters are generated in CANoe using a ramp signal generator creating a verifiable frame sequence. The logger records its own 64-bit start/stop timestamps (logger starts first, then CANoe; stopping happens in the reverse order) and writes only the summary (first/last sequence ID, drop count, timestamps). After the run a host script streams the SD log files, computes a CRC over the actual data, and compares the sequence range + runtime against the CANoe report to confirm no drops.

The test was run at 85 % bus load on both channels for 3 hours.

![Lossless logging measurement points](doc/images/measurement_lossless_proof.md.svg)

### Stress test

This section shows the results for a short stress test where both channels log fames at 1 Mbit/s@100 % bus load for 15 minutes to see the behavior under saturation.

> plceholder image TODO

# Quick start

The project consists of the source code written to run on a NUCLEO-144 STM32H745ZI discovery board. It uses GPIO to interface to:
- SD card
- CAN transceiver
- the on-board ETH interface

It is built using cmake and make based on the gcc toolchain.

This section briefly explains how to build and download the software for and to the device using the tool configuration included in this project.

## Run unit tests

_Placeholder: describe how to run `cmake -S tests -B build && cmake --build build && ctest`. Add pass/fail screenshot later._

## Verify logging

_Placeholder: describe end-to-end logging verification (e.g., feed CAN traffic, show resulting SD file)._

## System overview

The system uses following components:
- NUCLEO-H745ZI-Q STM32 board
- two TJA1050 based CAN transceiver breakout boards
- a 3 V micro SD card adapter breakout board

![System overview](doc/images/system_overview/system_overview.drawio.jpg)
*Figure: The image shows a simplified overview of the system components.*

More details of the systems technical context and a picture of the prototypical built can be found [here](doc/arc42-template-EN.md#technical-context)

> **NOTE**: Use a high quality SD card because cheap ones can have reliability issues when used with SPI (e.g., an SD card initializes correctly and some writes/reads work but then it hangs until power cycled for no apparent reason). The system was tested with a Kingston industrial-grade card.

## Prerequisites

Following tool versions are used to develop, debug and run the program on the target:

| Component | Version | Scope |
|-|-|-|
| cmake | 3.28.1 | build |
| make | 3.81 | build |
| GNU Tools for STM32 | 13.3.1 | build/ debug |
| OpenOCD | 0.12.0 | download/ debug |
| Cppcheck | 2.17.1 | develop |
| clang-format | 20.1.8 | develop |

## Build the code

To create the software, you can use following command from the repo's root directory:

```sh
cmake --preset "Debug" -B ./build/
``` 

Running this command might take a while because following dependencies are pulled while generating the configuration:

- FreeRTOS (see [FreeRTOS on github](https://github.com/FreeRTOS/FreeRTOS-LTS.git))
- lwrb (see [lwrb on github](https://github.com/MaJerle/lwrb.git))

To build the actual code, run the following from the root directory:

```sh
make -C build -j4
``` 

Alternatively, use the CMake workflow and just run 

```sh
cmake --workflow --preset release-cm4 &&
cmake --workflow --preset release-cm7
```

to generate the configuration and start the build in one go.

## Download the code

You can run the commands

> STM32_Programmer_CLI -c port=SWD -d _bin/Debug/SPI_FullDuplex_ComDMA_CM7.elf 0x08000000 -rst

and

> STM32_Programmer_CLI -c port=SWD -d _bin/Debug/SPI_FullDuplex_ComDMA_CM4.elf 0x08100000 -rst

from the root directory.


## Access the web GUI

The CAN sniffer uses Multicast DNS (mDNS) and Internet Group Management Protocol (IGMP) so that its hostname can be discoverd. If your network supports mDNS you can access the CAN sniffer via following hostname:

> http://can-sniffer.local

Otherwise, the IP address is assigned to the CAN sniffer by the DHCP server of your network.
You have to identify the IP address manually via the MAC address:

> 00:80:E1:00:00:00

# Architecture

This is a brief overview of the architecture. You can read the full architecture documentation [HERE](doc/arc42-template-EN.md).

## Software 

The driver design is inspired by Linux's opaque‑ops and linker‑list registration patterns (SPI, CAN, Ethernet) and they run bare-metal
on FreeRTOS. Task orchestration is realized by prioritized preemptive task scheduling. 

At a high level the firmware separates drivers, a multistage buffering pipeline, storage, and configuration into distinct layers; the full breakdown (logging pipeline, config workflow, hooks) lives in the [arc42 building-block view](doc/arc42-template-EN.md#building-block-view). Storage modules rely on overridable hooks for error handling—see the [architecture decisions](doc/arc42-template-EN.md#error-handling-hooks-for-storage-subsystems) before integrating.

The full arc42 template based document (doc/arc42-template-EN.md) covers the following in more detail:

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

## Tooling

### Documentation tooling

Mermaid diagrams live under doc/images/*.md.

To regenerate the .svg outputs:

1. Create/activate a virtual environment in dev/scripts:
    > python -m venv .venv && source .venv/bin/activate
2. Install the converter’s dependencies (inside dev/scripts): 
    > pip install -r requirements.txt
3. Start up docker from within dev/scripts
    > docker-compose -f docker-compose.yaml up -d
4. Run 
    > python convert_mermaid.py --root ../doc/images
    
    The script scans for .md sources and emits .md.svg files with the same name.

The docker-compose helper only provides a local Kroki + Mermaid renderer; the actual export still requires running `convert_mermaid.py` from inside `doc/scripts`.

> NOTE: Usage of the Memaid CLI can be found here: 
> https://doctoolchain.org/docToolchain/v2.0.x/020_tutorial/170_kroki-configuration.html

### Formatting tooling

```sh
clang-format -i --files=tools/clang/clang_files.txt
```

# Future work

Planned features are:
- support for CAN FD frames
- SDIO interface to allow for lower-quality SD cards
- Wi-Fi extension
- external Realtime-Clock (RTC)
- support to convert to various CAN log formats via GUI (e.g. Vector ASCII)
- for better performance and higher availability the serving of requests via ethernet shall be executed on another core to not interfere with the CAN trace logging when large files are loaded for user downloads.
- in-app-programming to update the firmware via web GUI
