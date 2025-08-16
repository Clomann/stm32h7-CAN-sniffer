
#pragma once

#include <stdint.h>

#include "stm32h7xx_nucleo.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_spi.h"

#include "CommFactory.h"
#include "spi_cfg.h"

#define SPI_DRIVER_NUMBER_1 0U
#define SPI_DRIVER_NUMBER_2 1U
#define SPI_DRIVER_NUMBERn  2U

typedef uint8_t SpiDriverNumberType;

#define SPI_ASSIGN_SLOT(_array_)                                               \
    {.used_len = 0U,                                                           \
     .max_len  = sizeof(_array_) / sizeof(_array_[0]),                         \
     .data     = _array_}

#define SPI_SLOT_META_BYTES ((uint32_t)offsetof(SpiSlotType, data))

#define SPI_ASSERT_SIZE(_array_)                                               \
    _Static_assert(                                                            \
        (sizeof(_array_) % CACHE_LINE_SIZE) == 0,                              \
        "sizeof(" #_array_ ")slot size must be a multiple of CACHE_LINE_SIZE"  \
    )

#define SPI_ASSERT_ALIGNMENT(_sym_)                                            \
    _Static_assert(                                                            \
        __alignof__(_sym_) >= CACHE_LINE_SIZE,                                 \
        #_sym_ " is not cache-line aligned"                                    \
    )

#define SPI_E_OK           0U
#define SPI_E_NOT_OK       1U
#define SPI_E_NULL_POINTER 1U

typedef uint8_t SpiErrorType;

#define SPI_DIR_TX_RX   0U
#define SPI_DIR_TX_ONLY 1U
#define SPI_DIR_RX_ONLY 2U
// #define SPI_DIR_RX_WITH_CMD      3U
// #define SPI_DIR_DUPLEX_STREAMING 4U
// #define SPI_DIR_BURST_READ       5U

typedef uint8_t SpiDirectionType;

typedef struct
{
    SPI_TypeDef *spi;
    SPI_HandleTypeDef hspi;
    CommDriver *drv;
} SpiInstanceType;

typedef struct
{
    uint8_t *rx_slots;
    uint8_t *tx_slots;
    uint8_t tx_bin_cnt;
    uint8_t rx_bin_cnt;

    /*! byte distance between slot i and i+1        */
    uint16_t rx_slot_stride;
    uint16_t tx_slot_stride;

    uint8_t rx_slots_cnt;
    uint8_t tx_slots_cnt;
} SpiConfigType;

typedef void (*SpiCompleteionCallbackType)(
    void *context,
    uint32_t status,
    const uint8_t *rx_data,
    uint32_t len
);

typedef struct
{
    uint32_t id;
    uint32_t timeout;
    uint16_t length;
    SpiDirectionType direction;
    SpiCompleteionCallbackType callback;
    void *context;
} SpiTransactionType;

typedef struct
{
    uint16_t used_len;
    const uint16_t max_len;
    SpiTransactionType transaction;
    uint8_t data[];
} SpiSlotType;

typedef struct
{
    const uint32_t slot_cnt;
    const RingBuffer *const slots;
} SpiBinType;

comm_status_t SPI_DestroyDriver(CommDriver *drv);

comm_status_t SPI_CreateDriver(
    CommDriver *drv,
    const void *cfg,
    size_t cfg_size,
    uint8_t *tx,
    uint8_t *rx
) COMM_FACTORY_USED_ATTR;

comm_status_t SPI_Init(CommDriver *dev);

comm_status_t SPI_DeInit(CommDriver *dev);

comm_status_t SPI_Send(CommDriver *dev, const void *);

comm_status_t SPI_Read(
    CommDriver *drv,
    void *msg,
    uint8_t length, /* for comapitibilty (fdcan) */
    uint32_t RxFifo0ITs /* for comapitibilty (fdcan) */
);

comm_status_t SPI_Ioctl(CommDriver *dev, int cmd, void *argument);

void SPI_Poll(CommDriver *drv);
uint32_t Spi_HasPendingTransfers(CommDriver *drv);

RingBuffer *Spi_GetSlots(const uint8_t *const buf);
void SPI_ErrorHandler(void);
