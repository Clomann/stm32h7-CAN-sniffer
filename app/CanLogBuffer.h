
#pragma once

#include <stdint.h>

#define CANLOG_E_OK             0U
#define CANLOG_E_NOT_OK         1U
#define CANLOG_E_EMPTY          2U
#define CANLOG_E_BOCK_FULL      3U
#define CANLOG_E_FILE_OPEN      4U
#define CANLOG_E_FILE_READ      5U
#define CANLOG_E_FILE_WRITE     6U

#define CANLOG_ENTRY_MAX_DATA_LENGTH 64U

#define CANLOG_VERSION      2U
#define BLOCK_SIZE          (64U * 1024U)
#define LOG_BUFFER_SIZE     (3U * BLOCK_SIZE)

#define CLB_ENTRY_TYPE_NONE   0
#define CLB_ENTRY_TYPE_FRAME  1
#define CLB_ENTRY_TYPE_SYNC   2
#define CLB_ENTRY_TYPE_MARKER 3

typedef uint8_t ClbEntryTypeType;

#define CAN_DLC_MASK         0x0F
#define CAN_DLC_MASK_Pos     0U
#define CAN_FLAG_IDE_Pos     4U
#define CAN_FLAG_IDE         (1U << CAN_FLAG_IDE_Pos)
#define CAN_FLAG_RTR_FDF_Pos 5U
#define CAN_FLAG_RTR_FDF     (1U << CAN_FLAG_RTR_FDF_Pos)  // RTR for classic, FDF for CAN FD
#define CAN_FLAG_BRS_Pos     6U
#define CAN_FLAG_BRS         (1U << CAN_FLAG_BRS_Pos)
#define CAN_FLAG_ESI_Pos     7U
#define CAN_FLAG_ESI         (1U << CAN_FLAG_ESI_Pos)

typedef uint8_t ClbDlcFlagsType;

#define GET_DLC(dlc_flags)         ((dlc_flags) & CAN_DLC_MASK)
#define GET_FLAGS(dlc_flags)       ((dlc_flags) & 0xF0)
#define MAKE_DLC_FLAGS(dlc, flags) (((dlc) & 0x0F) | ((flags) & 0xF0))

typedef struct {
    ClbEntryTypeType  type;       // e.g., 0 = None, 1 = CAN (FD) frame, 2 = SYNC, 3 = Marker
    uint8_t  header_len; // Length of header (excluding type and length)
    uint8_t total_len;   // Full length including header + payload
} __attribute__((packed)) CanLogEntryHeaderType;

typedef struct __attribute__((packed)) {
    CanLogEntryHeaderType header;
    uint32_t timestamp; /*!< timestamp in us */
    uint32_t can_id;
    uint8_t channel;
    ClbDlcFlagsType dlc_flags;
    uint8_t data_len;     
    uint8_t  data[];
} CanLogEntryType;

typedef struct {
    _Alignas(CanLogEntryType) uint8_t raw[sizeof(CanLogEntryType) + CANLOG_ENTRY_MAX_DATA_LENGTH];
} CanLogEntryStackBufferType;

typedef struct __attribute__((packed)) {
    CanLogEntryHeaderType header;
    uint32_t timestamp; /*!< timestamp in us */
    uint32_t abs_time_high;  /*!< most significant 32 bit of absolute timestamp in us */
} CanLogSyncType;

typedef struct {
    uint8_t version;
    uint8_t header_size;    // e.g., 64
    uint8_t epoch; /*!< Epoch counter */
    uint8_t cnt;   /*!< Block sequence counter */
    uint32_t block_size;     // indicates block size (e.g. 512)
    uint32_t block_fill;    // indicates the block fill level to determine padding byte count
    uint32_t ingress_frames; /*!< ingress frame count of staging buffer */
    uint32_t frame_count; /*!< number of frames in this block */
} __attribute__((packed)) CanLogBlockHeaderType;

uint8_t CanLogBuffer_Init(void);

void CanLogBuffer_SetEpochCount(uint8_t epoch);

uint8_t CanLogBuffer_AddEntry(const void* entry, uint32_t entryTotalSize);
uint8_t CanLogBuffer_FillBlockWithPadding(void);

uint8_t CanLogBuffer_IsBlockReady(uint8_t*rdy);

uint8_t CanLogBuffer_UsedSlots(uint8_t *slots);

uint8_t CanLogBuffer_GetCurrentBlockIndex(uint32_t *index);

uint8_t CanLogBuffer_ReadNextBlock(uint8_t **data, uint32_t *len, uint32_t *frame_count);
uint8_t CanLogBuffer_Consume(uint32_t len, uint32_t frame_count);
