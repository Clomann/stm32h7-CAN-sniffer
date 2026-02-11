/*
 * FatFsApi.c
 *
 *  Created on: Dec 25, 2024
 *      Author: cbromann
 */

#include <DiskIoFacade.h>

#include "MmcAdapter.h"

PARTITION VolToPart[FF_VOLUMES] = {
    {0, 1}, // "0:" = Physical drive 0, Partition 1
    {0, 2} // "1:" = Physical drive 0, Partition 2
};

static void MMC_ErrorHandler(void)
{
    __asm volatile("nop");
}

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

uint8_t MMC_disk_status(void)
{
    uint8_t result;

    result = 0U;

    if (0 != result)
    {
        MMC_ErrorHandler();
    }

    return result;
}

uint8_t MMC_disk_initialize(void)
{
    uint8_t result;

    result = MMCAdapter_initialize();

    if (0 != result)
    {
        MMC_ErrorHandler();
    }

    return result;
}

uint8_t MMC_disk_read(BYTE *buff, LBA_t sector, UINT count)
{
    uint8_t result;

    result = MMCAdapter_read(buff, sector, count);

    if (0 != result)
    {
        MMC_ErrorHandler();
    }

    return result;
}

uint8_t MMC_disk_write(const BYTE *buff, LBA_t sector, UINT count)
{
    uint8_t result;

    result = MMCAdapter_write(buff, sector, count);

    if (0 != result)
    {
        MMC_ErrorHandler();
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
        MMC_ErrorHandler();
    }

    return result;
}

uint8_t RAM_disk_status(void)
{
    uint8_t result;

    result = 0U;

    if (0 != result)
    {
        MMC_ErrorHandler();
    }

    return result;
}

uint8_t USB_disk_status(void)
{
    uint8_t result;

    result = 0U;

    if (0 != result)
    {
        MMC_ErrorHandler();
    }

    return result;
}

uint8_t RAM_disk_initialize(void)
{
    uint8_t result;

    result = 0U;

    if (0 != result)
    {
        MMC_ErrorHandler();
    }

    return result;
}

uint8_t USB_disk_initialize(void)
{
    uint8_t result;

    result = 0U;

    if (0 != result)
    {
        MMC_ErrorHandler();
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
        MMC_ErrorHandler();
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
        MMC_ErrorHandler();
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
        MMC_ErrorHandler();
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
        MMC_ErrorHandler();
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
        MMC_ErrorHandler();
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
        MMC_ErrorHandler();
    }

    return result;
}
