/*
 * MmcAdapter.c
 *
 *  Created on: Dec 29, 2024
 *      Author: cbromann
 */

#include <stdint.h>
#include <string.h>

#include "MmcAdapter.h"

static volatile SdCsdRegisterType Csd_Sd1 = {0};

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
	uint8_t RetVal;

	// TODO: interface specific selection logic (SPI, SDIO)
	RetVal = SD_Spi_Initialize(0U);

	if (0U == RetVal)
	{
		RetVal = SD_Spi_ReadCSD(&Csd_Sd1);
	}

	return RetVal;
}

uint8_t MMCAdapter_read(BYTE *buff, LBA_t sector, UINT count)
{
	Spi_R1Response resp;
	uint8_t RetVal = RES_OK;
	uint8_t RetryCount = 0U;

    if (SD_E_OK == SD_Spi_readMultiBlock(sector, buff, count))
    {
        return RES_OK;
    }
    else
    {
        return RES_ERROR;
    }

	return RetVal;
}

uint8_t MMCAdapter_write(const BYTE *buff, LBA_t sector, UINT count)
{
	if (SD_E_OK == SD_Spi_writeMultiBlock(sector, buff, count))
    {
        return RES_OK;
    }
    else
    {
        return RES_ERROR;
    }

}

uint8_t MMCAdapter_CtrlSync(void)
{
	return RES_OK;
}

uint8_t MMCAdapter_GetSectorCount(void *buff)
{
    uint32_t SectorCount;

    if (NULL == buff ||0 ==  Csd_Sd1.cSize) {
        return RES_ERROR;
    }

    // Using your parsed CSD data
    if (Csd_Sd1.csdStructure >= 1) 
    {  
        // CSD v2.0
        // SectorCount = (cSize + 1) * 1024; Capacity = SectorCount * 512 bytes
        SectorCount = (Csd_Sd1.cSize + 1) * 1024;
        memcpy(buff, &SectorCount, sizeof(SectorCount));
        
        return RES_OK;
    } 
    else 
    {  
        // CSD v1.0
        // uint32_t mult = 1 << (Csd_Sd1.cSizeMult + 2);
        // uint32_t blocknr = (Csd_Sd1.cSize + 1) * mult;
        // uint32_t block_len = 1 << Csd_Sd1.readBlLen;
        // SectorCount = blocknr * block_len / 512;

        // memcpy(buff, &SectorCount, sizeof(SectorCount));

        return RES_ERROR;
    }
}

uint8_t MMCAdapter_GetSectorSize(void *buff)
{
	uint32_t SectorSize;

    if (NULL == buff ||0 ==  Csd_Sd1.cSize) {
        return RES_ERROR;
    }

    // Using your parsed CSD data
    if (Csd_Sd1.csdStructure >= 1) 
    {  
        // CSD v2.0
        SectorSize = 1 << Csd_Sd1.sectorSize;
        memcpy(buff, &SectorSize, sizeof(SectorSize));
        
        return RES_OK;
    } 
    else 
    {  
        // CSD v1.0

        return RES_ERROR;
    }
}

uint8_t MMCAdapter_GetBlockSize(void *buff)
{
    uint32_t BlockSize;

    if (NULL == buff ||0 ==  Csd_Sd1.cSize) {
        return RES_ERROR;
    }

    // Using your parsed CSD data
    if (Csd_Sd1.csdStructure >= 1) 
    {  
        // CSD v2.0
        BlockSize = 1 << Csd_Sd1.readBlLen;
        memcpy(buff, &BlockSize, sizeof(BlockSize));
        
        return RES_OK;
    } 
    else 
    {  
        // CSD v1.0

        return RES_ERROR;
    }
}

uint8_t MMCAdapter_CtrlTrim(void *buff)
{
    LBA_t StartSector, EndSector;
    LBA_t *Buf;

    if (!Csd_Sd1.eraseBlkEn || NULL == buff) {
        return RES_ERROR;
    }
    
    Buf = (LBA_t *) buff;

    memcpy(&StartSector, &Buf[0], sizeof(StartSector));
    memcpy(&EndSector, &Buf[1], sizeof(EndSector));

    SD_Spi_SendCommand(SD_SPI_CMD32, StartSector);
    SD_Spi_SendCommand(SD_SPI_CMD33, EndSector);
    SD_Spi_SendCommand(SD_SPI_CMD38, 0);

	return RES_OK;
}

uint8_t MMCAdapter_MmcGetSdstat(void *buff)
{
	(void) buff;
	return RES_ERROR;
}

uint8_t MMCAdapter_MmcGetOcr(void *buff)
{
	(void) buff;
	return RES_ERROR;
}

uint8_t MMCAdapter_MmcGetCid(void *buff)
{
	(void) buff;
	return RES_ERROR;
}

uint8_t MMCAdapter_MmcGetCsd(void *buff)
{
	(void) buff;
	return RES_ERROR;
}

uint8_t MMCAdapter_MmcGetType(void *buff)
{
	(void) buff;
	return RES_ERROR;
}
