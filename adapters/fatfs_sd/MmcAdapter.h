/*
 * MmcAdapter.h
 *
 *  Created on: Dec 29, 2024
 *      Author: cbromann
 */

#ifndef CM7_INC_MMCADAPTER_H_
#define CM7_INC_MMCADAPTER_H_

#include <stdint.h>

#include "Sd.h"

#define MMC_SPI_MAX_DEVICES		1U

#define MMC_CARD_TYPE_MMCV3		1U
#define MMC_CARD_TYPE_SDV1		2U
#define MMC_CARD_TYPE_SDV2PLUS	4U

typedef DSTATUS MmcStatusType;

typedef struct {
    uint8_t pdrv;           /*!< Physical drive number */
    uint8_t cs_pin;      	/*!< Chip select pin */
    uint8_t initialized;    /*!< Is the device initialized? */
    uint8_t in_use;         /*!< Is the device in use? */
    MmcStatusType status;
    uint32_t cardSize;			/*!< size of the card in bytes */
    uint32_t sectorSize;
    uint32_t blockSize;
    uint32_t sectorCount;
} MmcSpiDeviceType;

/**
 * \brief The disk_initialize function is called to initializes the storage device.
 */
uint8_t MMCAdapter_initialize(void);

uint8_t MMCAdapter_read(
		BYTE *buff,		/*!< Data buffer to store read data */
		LBA_t sector,	/*!< Start sector in LBA */
		UINT count		/*!< Number of sectors to read */
);

uint8_t MMCAdapter_write(
		const BYTE *buff,	/*!< Data to be written */
		LBA_t sector,		/*!< Start sector in LBA */
		UINT count			/*!< Number of sectors to write */
);

/**
 * \brief Makes sure that the device has finished pending write process. If
 * the disk I/O layer or storage device has a write-back cache, the dirty
 * cache data must be committed to the medium immediately. Nothing to do for
 * this command if each write operation to the medium is completed in the
 * disk_write function.
 */
uint8_t MMCAdapter_CtrlSync(void);

/**
 *
 * \brief Retrieves number of available sectors (the largest allowable LBA + 1)
 *  on the drive into the LBA_t variable that pointed by buff. This command is
 *  used by f_mkfs and f_fdisk function to determine the size of
 *  volume/partition to be created.
 */
uint8_t MMCAdapter_GetSectorCount(void *buff);

/**
 *
 * \brief Retrieves sector size (minimum data unit for generic read/write) into
 * the WORD variable that pointed by buff. Valid sector sizes are 512, 1024, 2048
 * and 4096. This command is required only if FF_MAX_SS > FF_MIN_SS. When
 * FF_MAX_SS == FF_MIN_SS, this command will never be used and the disk_read and
 * disk_write function must work in FF_MAX_SS bytes/sector.
 *
 * The sector size in the context of an SD card and the FatFS implementation
 * refers to the smallest unit of data that the file system reads or writes.
 * FatFS uses the term "sector" synonymously with "block" as defined by the
 * SD card specifications. For SDHC/SDXC cards, the sector size is always
 * 512 bytes, while for some older standard-capacity SD cards (SDSC), it can
 * vary.
 */
uint8_t MMCAdapter_GetSectorSize(void *buff);

/**
 * \brief Retrieves erase block size in unit of sector of the flash memory
 * media into the DWORD variable that pointed by buff. The allowable value is
 * 1 to 32768 in power of 2. Return 1 if it is unknown or in non flash memory
 * media. This command is used by f_mkfs function with block size not specified
 * and it attempts to align the data area on the suggested block boundary.
 * Note that FatFs does not have FTL (flash translation layer), so that either
 * disk I/O layter or storage device must have an FTL in it.
 */
uint8_t MMCAdapter_GetBlockSize(void *buff);

/**
 * \brief Informs the disk I/O layter or the storage device that the data on
 * the block of sectors is no longer needed and it can be erased. The sector
 * block is specified in an LBA_t array {<Start LBA>, <End LBA>} that pointed
 * by buff. This is an identical command to Trim of ATA device. Nothing to do
 * for this command if this funcion is not supported or not a flash memory
 * device. FatFs does not check the result code and the file function is not
 * affected even if the sector block was not erased well. This command is
 * called on remove a cluster chain and in the f_mkfs function. It is required
 * when FF_USE_TRIM == 1.
 */
uint8_t MMCAdapter_CtrlTrim(void *buff);

/**
 * \brief Reads SDSTATUS register and sets it into a 64-byte buffer pointed by
 * buff. (SDC specific command)
 */
uint8_t MMCAdapter_MmcGetSdstat(void *buff);

/**
 * \brief Reads OCR register and sets it into a 4-byte buffer pointed by buff.
 * (MMC/SDC specific command)
 */
uint8_t MMCAdapter_MmcGetOcr(void *buff);

/**
 * \brief Reads CID register and sets it into a 16-byte buffer pointed by buff.
 * (MMC/SDC specific command)
 */
uint8_t MMCAdapter_MmcGetCid(void *buff);

/**
 * \brief Reads CSD register and sets it into a 16-byte buffer pointed by buff.
 * (MMC/SDC specific command)
 */
uint8_t MMCAdapter_MmcGetCsd(void *buff);

/**
 * \brief Gets card type. The type flags, bit0:MMCv3, bit1:SDv1, bit2:SDv2+
 * and bit3:LBA, is stored to a BYTE variable pointed by buff. (MMC/SDC specific command)
 */
uint8_t MMCAdapter_MmcGetType(void *buff);

#endif /* CM7_INC_MMCADAPTER_H_ */
