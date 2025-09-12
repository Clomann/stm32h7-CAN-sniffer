# Funtional requirements

This section states the most important functional requirements for the CAN sniffer.

## CAN
The CAN sniffer shall have following CAN specific functions:
- The system shall support standard (11-bit) and extended (29-bit) CAN identifiers.
- The system shall support baudrates of up to 1 Mbit/s
- The system shall be able to filter messages based on CAN ID ranges.
- The system shall detect and log error frames.
- The system shall log bus load statistics.
- The system shall support ISO-TP (ISO 15765-2) reassembly for multi-frame messages.

## Data Logging Requirements
- The system shall log messages to an SD card formatted with FAT32.
- The system shall store CAN messages in a timestamped log format.
- The system shall allow log retrieval via a network or direct SD card access.
- The system shall support automatic log file rotation to prevent SD card overflow.
- The system shall include metadata (e. g. timestamp, frame type) for each message.

## Server Requirements
- The system shall host a web interface accessible over Wi-Fi or LAN.
- The web interface shall allow live message monitoring.
- The web interface shall provide options to configure the message filters based on CAN IDs.
- The system shall support starting and stopping logging sessions via the web interface.
- The system shall support remote log file download.
- optional: The system shall support firmware updates via the web interface.

## Storage and File Management Requirements
- The system shall implement buffered writes to the SD card to minimize wear.
- The system shall create a new log file at the start of each session.
- The system shall allow oldest logs to be deleted automatically if storage is full.

# Hardware Constraints
- The system shall operate with an STM32H7 microcontroller.
- The system shall support an SPI-connected SD card.
- The system shall support a low-power mode when logging is not active.
- The system shall indicate operational status via LED indicators.

# Wishlist/ Optional
- the web interface shall support client side CAN log parsing to offload the 
formatting from the server
- support for fdcan frames

# Docuemnt history

| Date | Revision | Author | Status | Comment |
|-|-|-|-|-|
| 2025-07-17 | 0.1 | Clemens Bromann | Draft | initial collection of functional requirements |
|  |  |  |  |  |