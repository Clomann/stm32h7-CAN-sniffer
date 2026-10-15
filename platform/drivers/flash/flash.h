#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef uint8_t FlashStatusType;

#define FLASH_E_OK      ((FlashStatusType)0)
#define FLASH_E_RANGE   ((FlashStatusType)1)
#define FLASH_E_ALIGN   ((FlashStatusType)2)
#define FLASH_E_BUSY    ((FlashStatusType)3)
#define FLASH_E_PROTECT ((FlashStatusType)4)
#define FLASH_E_VERIFY  ((FlashStatusType)5)
#define FLASH_E_HW      ((FlashStatusType)6)
#define FLASH_E_PARAM   ((FlashStatusType)7)

/**
 * @brief Flash sector information.
 *
 * @note The @ref off field is an offset relative to the start of the flash
 *       device. It is not an absolute address.
 */
typedef struct
{
    uint32_t off; /**< Sector start offset relative to flash-device base. */
    uint32_t size; /**< Sector size in bytes. */
} FlashSectorInfoType;

typedef struct
{
    uint32_t base_addr;
    uint32_t end_addr; /*<! exclusive end (first address after flash) */
    uint32_t total_size;
    uint32_t write_alignment; // alignment for writes in bytes
    uint32_t erase_size_min; // smallest erase granularity in bytes
    uint32_t prog_size_min; // smallest program granularity in bytes
    uint32_t page_size; // 0 if not meaningful on this part
    uint32_t sector_size; // 0 if not meaningful on this part
    uint32_t
        source_alignment; /*!< Source alignment minimum required in bytes */
    bool requires_explicit_erase;
} FlashInfoType;

FlashStatusType Flash_Init(void);
FlashStatusType Flash_GetInfo(FlashInfoType *info);
FlashStatusType Flash_Read(uint32_t addr, void *buf, size_t len);
FlashStatusType Flash_Erase(uint32_t addr, size_t len);
FlashStatusType Flash_Write(uint32_t addr, const void *buf, size_t len);
FlashStatusType Flash_Verify(uint32_t addr, const void *buf, size_t len);
FlashStatusType
Flash_GetSectorByAddr(uint32_t addr, FlashSectorInfoType *sector);
