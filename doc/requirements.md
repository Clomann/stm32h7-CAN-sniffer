# Functional requirements checklist

Legend: 
- [x] implemented
- [ ] not implemented or partial (notes describe gaps).
- [-] deprecated

## CAN features

- [x] Support standard (11-bit) and extended (29-bit) identifiers
- [x] Operate up to 1 Mbit/s
- [ ] Filter CAN IDs by range
- [ ] Detect/log error frames
- [ ] Log bus load statistics
- [-] ISO-TP reassembly

## Data logging

- [x] Log to FAT32 SD card
- [x] Include timestamps per message
- [x] Allow log retrieval via network or SD
- [x] Rotate log files automatically
- [x] Include metadata per message

## Server / web UI

- [x] Host web interface over LAN
- [ ] Wi-Fi access
- [x] Live message monitoring
- [x] Configure CAN filters via UI
- [x] Start/stop logging via UI
- [x] Remote log download
- [ ] Firmware updates over web

## Storage & file management

- [x] Buffered SD writes
- [x] New log file per session
- [x] Format SD card via web GUI
- [-] Auto-delete oldest logs

## Hardware constraints

- [x] Operate on STM32H7
- [x] SPI-connected SD card
- [ ] Low-power mode when idle
- [ ] LED indicators

## Wishlist / optional

- [x] Client-side CAN log parsing
- [ ] FDCAN (CAN-FD) support

# Document history

| Date | Revision | Author | Status | Comment |
| - | - | - | - | - |
| 2025-07-17 | 0.1 | Clemens Bromann | Draft | Initial checklist conversion |
| 2025-11-17 | 0.2 | Clemens Bromann | Draft | turned into checklist; added new reqs; deprecated: ISO-TP, auto-delete  |
