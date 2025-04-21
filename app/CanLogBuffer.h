
#pragma once

#include <stdint.h>

#define BLOCK_SIZE  512U
#define ENTRY_COUNT 96U
#define ENTRY_SIZE  16U

typedef struct {
    struct {
        uint16_t lsb;
        uint8_t msb;
    } __attribute__((packed)) timestamp_us;
    uint8_t dlc;
    uint8_t data[8];
    uint32_t can_id;
} __attribute__((packed)) CanLogEntryType;

typedef struct {
    union {
        /* 75 entries should suffice for 10 ms listening to
        classic CAN at 1 Mbit/s with extendede CAN IDs,
        additional entries are added to have a multiple
        of the SD cards block size (512 bytes) */
        CanLogEntryType entries[ENTRY_COUNT]; 
        uint8_t raw[3 * BLOCK_SIZE];
    };
} __attribute__((packed)) CanLogType;

uint8_t CanLogBuffer_Init(void);

uint8_t CanLogBuffer_AddEntry(const CanLogEntryType* entry);

uint8_t CanLogBuffer_IsBlockReady(uint8_t*rdy);

uint8_t CanLogBuffer_GetCurrentBlockIndex(uint32_t *index);

uint8_t CanLogBuffer_ReadNextBlock(uint8_t *data, uint32_t *len);

uint32_t get_timestamp_us(const CanLogEntryType* entry);