/*
 * FatFsApi.c
 *
 *  Created on: Dec 25, 2024
 *      Author: cbromann
 */

#include <DiskIoFacade.h>

#include "MmcAdapter.h"

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

uint8_t MMC_disk_status()
{
	uint8_t result;

	result = 0U;

	return result;
}

uint8_t MMC_disk_initialize()
{
	uint8_t result;

	result = MMCAdapter_initialize();

	return result;
}

uint8_t MMC_disk_read(BYTE * buff, LBA_t sector, UINT count)
{
	uint8_t result;

    (void) (buff);
    (void) (sector);
    (void) (count);

	result = MMCAdapter_read(buff, sector, count);

	return result;
}

uint8_t MMC_disk_write(const BYTE *buff, LBA_t sector, UINT count)
{
	uint8_t result;

    (void) (buff);
    (void) (sector);
    (void) (count);

	result = MMCAdapter_write(buff, sector, count);

	return result;
}

DRESULT MMC_disk_ioctl(
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
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
	MMCAdapter_GetSectorCount(buff);
	break;
case GET_SECTOR_SIZE:
	MMCAdapter_GetSectorSize(buff);
	break;
case GET_BLOCK_SIZE:
	MMCAdapter_GetBlockSize(buff);
	break;
case CTRL_TRIM:
	MMCAdapter_CtrlTrim(buff);
	break;

/* MMC/SDC specific ioctl command */
case MMC_GET_TYPE:
	MMCAdapter_MmcGetType(buff);
	break;
case MMC_GET_CSD:
	MMCAdapter_MmcGetCsd(buff);
	break;
case MMC_GET_CID:
	MMCAdapter_MmcGetCid(buff);
	break;
case MMC_GET_OCR:
	MMCAdapter_MmcGetOcr(buff);
	break;
case MMC_GET_SDSTAT:
	MMCAdapter_MmcGetSdstat(buff);
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
return result;
}

uint8_t RAM_disk_status()
{
	uint8_t result;

	result = 0U;

	return result;
}

uint8_t USB_disk_status()
{
	uint8_t result;

	result = 0U;

	return result;
}

uint8_t RAM_disk_initialize()
{
	uint8_t result;

	result = 0U;

	return result;
}

uint8_t USB_disk_initialize()
{
	uint8_t result;

	result = 0U;

	return result;
}

uint8_t RAM_disk_read(BYTE * buff, LBA_t sector, UINT count)
{
	uint8_t result;

    (void) (buff);
    (void) (sector);
    (void) (count);

	result = 0U;

	return result;
}

uint8_t USB_disk_read(BYTE * buff, LBA_t sector, UINT count)
{
	uint8_t result;
    
    (void) (buff);
    (void) (sector);
    (void) (count);

	result = 0U;

	return result;
}

uint8_t RAM_disk_write(const BYTE * buff, LBA_t sector, UINT count)
{
	uint8_t result;

    (void) (buff);
    (void) (sector);
    (void) (count);

	result = 0U;

	return result;
}
uint8_t USB_disk_write(const BYTE * buff, LBA_t sector, UINT count)

{
	uint8_t result;

    (void) (buff);
    (void) (sector);
    (void) (count);

	result = 0U;

	return result;
}

DRESULT RAM_disk_ioctl(
		BYTE cmd,		/* Control code */
		void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT result;

    (void) (cmd);
    (void) (buff);

	result = 0U;

	return result;
}

DRESULT USB_disk_ioctl(
		BYTE cmd,		/* Control code */
		void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT result;

    (void) (cmd);
    (void) (buff);
    
	result = 0U;

	return result;
}
