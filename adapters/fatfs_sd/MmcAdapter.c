/*
 * MmcAdapter.c
 *
 *  Created on: Dec 29, 2024
 *      Author: cbromann
 */

#include "MmcAdapter.h"

// static uint8_t SPI_CMD_WRITE_BUFFER[SD_SDHC_SECTOR_SIZE] = {0};

// static MmcSpiDeviceType SpiDevicesData[MMC_SPI_MAX_DEVICES] = {
// 		{
// 				.pdrv = 0xFF,
// 				.cs_pin = 0U,
// 				.in_use = 0U,
// 				.initialized = 0U,
// 				.status = (1U << STA_PROTECT) | (1U << STA_NODISK)  | (1U << STA_NOINIT)
// 		} /**!< Initialize with unused state */
// };

uint8_t MMCAdapter_initialize(void)
{
	SdCsdRegisterType CsdReg;
	uint8_t RetVal;

	// TODO: interface specific selection logic (SPI, SDIO)
	RetVal = SD_Spi_Initialize(0U);

	if (0U == RetVal)
	{
		RetVal = SD_Spi_ReadCSD(&CsdReg);
	}

	return RetVal;
}

uint8_t MMCAdapter_read(BYTE *buff, LBA_t sector, UINT count)
{
	Spi_R1Response resp;
	uint8_t RetVal = RES_OK;
	uint8_t RetryCount = 0U;

	for (uint32_t i=0; i<count; i++)
	{
		do
		{
			RetVal = SD_Spi_readSingleBlock(sector + i, &resp);
			RetryCount++;
		} while (0U != RetVal && RetryCount<10);

		if (0U == RetVal)
		{
			SD_Spi_GetReadBytes(&buff[i * SD_SECTOR_LENGTH]);
		}
		else if (RetVal == SD_E_CMD_LOST_CONNECTON)
		{
			/* caller needs to re-power and re-initialize the SD card */
		}
		else
		{
			RetVal = RES_ERROR;
		}
	}

	return RetVal;
}

uint8_t MMCAdapter_write(const BYTE *buff, LBA_t sector, UINT count)
{
	for (uint32_t i=0; i<count; i++)
	{
		SD_Spi_writeBlock(sector + i, &buff[i * SD_SECTOR_LENGTH]);
	}

	return 0U;
}

uint8_t MMCAdapter_CtrlSync(void)
{
	return RES_OK;
}

uint8_t MMCAdapter_GetSectorCount(void *buff)
{
	(void) buff;
	return 0U;
}

uint8_t MMCAdapter_GetSectorSize(void *buff)
{
	(void) buff;
	return 0U;
}

uint8_t MMCAdapter_GetBlockSize(void *buff)
{
	(void) buff;
	return 0U;
}

uint8_t MMCAdapter_CtrlTrim(void *buff)
{
	(void) buff;
	return 0U;
}

uint8_t MMCAdapter_MmcGetSdstat(void *buff)
{
	(void) buff;
	return 0U;
}

uint8_t MMCAdapter_MmcGetOcr(void *buff)
{
	(void) buff;
	return 0U;
}

uint8_t MMCAdapter_MmcGetCid(void *buff)
{
	(void) buff;
	return 0U;
}

uint8_t MMCAdapter_MmcGetCsd(void *buff)
{
	(void) buff;
	return 0U;
}

uint8_t MMCAdapter_MmcGetType(void *buff)
{
	(void) buff;
	return 0U;
}
