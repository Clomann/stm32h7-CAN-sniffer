/*
 * FatFsApi.c
 *
 *  Created on: Dec 25, 2024
 *      Author: cbromann
 */

#include <DiskIoFacade.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "MmcAdapter.h"

PARTITION VolToPart[FF_VOLUMES] = {
    {0, 1}, // "0:" = Physical drive 0, Partition 1
    {0, 2} // "1:" = Physical drive 0, Partition 2
};

static ErrorContextType ErrorContext;

static void MMC_SetErrorContext(
    ErrorIdType id,
    ErrorCodeType code,
    uint16_t line,
    const char *function,
    uint32_t arg0,
    uint32_t arg1
)
{
    memset(&ErrorContext, 0, sizeof(ErrorContext));
    ErrorContext.id   = id;
    ErrorContext.code = code;
    ErrorContext.arg0 = arg0;
    ErrorContext.arg1 = arg1;
    ErrorContext.line = line;
    snprintf(ErrorContext.file, sizeof(ErrorContext.file), "%s", __FILE_NAME__);
    snprintf(
        ErrorContext.function,
        sizeof(ErrorContext.function),
        "%s",
        function
    );
}

#define MMC_REPORT_ERROR(id, code, arg0, arg1)                                 \
    do                                                                         \
    {                                                                          \
        MMC_SetErrorContext(                                                   \
            (id),                                                              \
            (ErrorCodeType)(code),                                             \
            (uint16_t)__LINE__,                                                \
            __func__,                                                          \
            (uint32_t)(arg0),                                                  \
            (uint32_t)(arg1)                                                   \
        );                                                                     \
        MMC_ErrorHandler(&ErrorContext);                                       \
    } while (0)

__attribute__((weak)) void MMC_ErrorHandler(ErrorContextType *context)
{
    (void)context;

    __asm volatile("nop");
}

#if !FF_FS_READONLY && !FF_FS_NORTC
/**
  * @brief  Gets Time from RTC (generated when FS_NORTC==0; see ff.c)
  * @param  None
  * @retval Time in DWORD
  */
DWORD get_fattime(void)
{
    /* USER CODE BEGIN get_fattime */
    return 0;
    /* USER CODE END get_fattime */
}
#endif

uint8_t MMC_disk_status(void)
{
    uint8_t result;

    result = 0U;

    if (0 != result)
    {
        MMC_REPORT_ERROR(DISKIOFACADE_ERROR_ID_MMC_STATUS, result, 0U, 0U);
    }

    return result;
}

uint8_t MMC_disk_initialize(void)
{
    uint8_t result;

    result = MMCAdapter_initialize();

    if (0 != result)
    {
        MMC_REPORT_ERROR(DISKIOFACADE_ERROR_ID_MMC_INITIALIZE, result, 0U, 0U);
    }

    return result;
}

uint8_t MMC_disk_read(BYTE *buff, LBA_t sector, UINT count)
{
    uint8_t result;

    result = MMCAdapter_read(buff, sector, count);

    if (0 != result)
    {
        MMC_REPORT_ERROR(DISKIOFACADE_ERROR_ID_MMC_READ, result, sector, count);
    }

    return result;
}

uint8_t MMC_disk_write(const BYTE *buff, LBA_t sector, UINT count)
{
    uint8_t result;

    result = MMCAdapter_write(buff, sector, count);

    if (0 != result)
    {
        MMC_REPORT_ERROR(
            DISKIOFACADE_ERROR_ID_MMC_WRITE,
            result,
            sector,
            count
        );
    }

    return result;
}

DRESULT MMC_disk_ioctl(
    BYTE cmd, /* Control code */
    void *buff /* Buffer to send/receive control data */
)
{
    DRESULT result;

    result = RES_ERROR;

    switch (cmd)
    {
    /* Generic command (Used by FatFs) */
    case CTRL_SYNC:
        result = MMCAdapter_CtrlSync();
        break;
    case GET_SECTOR_COUNT:
        result = MMCAdapter_GetSectorCount(buff);
        break;
    case GET_SECTOR_SIZE:
        result = MMCAdapter_GetSectorSize(buff);
        break;
    case GET_BLOCK_SIZE:
        result = MMCAdapter_GetBlockSize(buff);
        break;
    case CTRL_TRIM:
        result = MMCAdapter_CtrlTrim(buff);
        break;

    /* MMC/SDC specific ioctl command */
    case MMC_GET_TYPE:
        result = MMCAdapter_MmcGetType(buff);
        break;
    case MMC_GET_CSD:
        result = MMCAdapter_MmcGetCsd(buff);
        break;
    case MMC_GET_CID:
        result = MMCAdapter_MmcGetCid(buff);
        break;
    case MMC_GET_OCR:
        result = MMCAdapter_MmcGetOcr(buff);
        break;
    case MMC_GET_SDSTAT:
        result = MMCAdapter_MmcGetSdstat(buff);
        break;
    case ISDIO_READ:
        result = RES_ERROR;
        break;
    case ISDIO_WRITE:
        result = RES_ERROR;
        break;
    case ISDIO_MRITE:
        result = RES_ERROR;
        break;
    default:
        result = RES_PARERR;
        break;
    }

    if (0 != result)
    {
        MMC_REPORT_ERROR(DISKIOFACADE_ERROR_ID_MMC_IOCTL, result, cmd, 0U);
    }

    return result;
}

uint8_t RAM_disk_status(void)
{
    uint8_t result;

    result = 0U;

    if (0 != result)
    {
        MMC_ErrorHandler(NULL);
    }

    return result;
}

uint8_t USB_disk_status(void)
{
    uint8_t result;

    result = 0U;

    if (0 != result)
    {
        MMC_ErrorHandler(NULL);
    }

    return result;
}

uint8_t RAM_disk_initialize(void)
{
    uint8_t result;

    result = 0U;

    if (0 != result)
    {
        MMC_ErrorHandler(NULL);
    }

    return result;
}

uint8_t USB_disk_initialize(void)
{
    uint8_t result;

    result = 0U;

    if (0 != result)
    {
        MMC_ErrorHandler(NULL);
    }

    return result;
}

uint8_t RAM_disk_read(BYTE *buff, LBA_t sector, UINT count)
{
    uint8_t result;

    result = 0U;

    (void)buff;
    (void)sector;
    (void)count;

    if (0 != result)
    {
        MMC_ErrorHandler(NULL);
    }

    return result;
}

uint8_t USB_disk_read(BYTE *buff, LBA_t sector, UINT count)
{
    uint8_t result;

    result = 0U;

    (void)buff;
    (void)sector;
    (void)count;

    if (0 != result)
    {
        MMC_ErrorHandler(NULL);
    }

    return result;
}

uint8_t RAM_disk_write(const BYTE *buff, LBA_t sector, UINT count)
{
    uint8_t result;

    result = 0U;

    (void)buff;
    (void)sector;
    (void)count;

    if (0 != result)
    {
        MMC_ErrorHandler(NULL);
    }

    return result;
}

uint8_t USB_disk_write(const BYTE *buff, LBA_t sector, UINT count)
{
    uint8_t result;

    result = 0U;

    (void)buff;
    (void)sector;
    (void)count;

    if (0 != result)
    {
        MMC_ErrorHandler(NULL);
    }

    return result;
}

DRESULT RAM_disk_ioctl(
    BYTE cmd, /* Control code */
    void *buff /* Buffer to send/receive control data */
)
{
    DRESULT result;

    result = 0U;

    (void)cmd;
    (void)buff;

    if (0 != result)
    {
        MMC_ErrorHandler(NULL);
    }

    return result;
}

DRESULT USB_disk_ioctl(
    BYTE cmd, /* Control code */
    void *buff /* Buffer to send/receive control data */
)
{
    DRESULT result;

    result = 0U;

    (void)cmd;
    (void)buff;

    if (0 != result)
    {
        MMC_ErrorHandler(NULL);
    }

    return result;
}
