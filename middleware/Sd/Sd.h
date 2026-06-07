/*
 * Sd.h
 *
 *  Created on: Dec 22, 2024
 *      Author: cbromann
 */

#ifndef CM7_INC_SD_H_
#define CM7_INC_SD_H_

#include "diskio.h"
#include "SdTypes.h"
#include "ErrorContext.h"

#define SD_SPI_CMD_DUMMY_DATA  0xFF
#define SD_SPI_CMD_START_TOKEN 0xFE

/*!< 32 Gb in bytes */
#define SD_TOTAL_SIZE 0x800000000
/*!< Sector lemngth in bytes (512 bytes) */
#define SD_SECTOR_LENGTH 0x000000200
#define SD_SECTOR_COUNT  (SD_TOTAL_SIZE / SD_BLOCK_LENGTH)
/**
 * @brief Max poll iterations while waiting for a read response token.
 *
 * The value (15625U) targets a ~100 ms timeout at the current SPI polling
 * cadence; adjust if SPI timing or polling changes.
 */
#define SD_MAX_READ_RESPONSE_ATTEMPTS (15625U)

/**
 * @brief Max poll iterations while waiting for CMD38 erase busy to clear.
 *
 * MLC NAND block erase takes 2-5 ms per block; with SD_ERASE_CHUNK_BLOCKS
 * blocks per CMD38 and no FTL-level parallelism, a single chunk can occupy
 * the bus for several seconds. Sized for a 30 s worst-case per chunk.
 */
#define SD_ERASE_BUSY_TIMEOUT_ATTEMPTS (SD_MAX_READ_RESPONSE_ATTEMPTS * 300U)

/*!The sector is the smallest individual reference-able regions on a disk.*/
#define SD_SDHC_SECTOR_SIZE 512U

/*!< maximum byte count that can be trasmitted in one SPI operation */
#define SD_SPI_MAX_TRANSFER_SIZE (512U)

#define SD_SPI_SECTOR_CHUNK_SIZE                                               \
    ((SD_SDHC_SECTOR_SIZE > SD_SPI_MAX_TRANSFER_SIZE                           \
          ? SD_SPI_MAX_TRANSFER_SIZE                                           \
          : SD_SDHC_SECTOR_SIZE))

/*!< Length of the CSD data send by the SD card in bytes. */
#define SD_SPI_CSD_LENGTH 16U

#define SD_DEF_DATA_RESP_TOKEN         0x05
#define SD_DEF_START_DATA_MARKER       0xFE
#define SD_DEF_MULTI_BLOCK_START_TOKEN 0xFC
#define SD_DEF_MULTI_BLOCK_STOP_TOKEN  0xFD
#define SD_DEF_DATA_ACCEPTED_TOKEN     0x05
#define SD_DEF_CRC_ERROR_TOKEN         0x0B
#define SD_DEF_WRITE_ERROR_TOKEN       0x0D

#define SD_E_OK        0U
#define SD_E_NOT_OK    1U
#define SD_E_INV_PARAM 2U
#define SD_E_NOT_IDLE  3U
#define SD_E_SEND      4U
#define SD_E_RESPONSE  5U

/*!< The card did not send R1 after receiving a command */
#define SD_E_CMD_NO_R1 6U
/*!< The card sent 0xFF after receiving a command */
#define SD_E_CMD_NO_DATA_RESP_TOKEN 7U
/*!< The card did never sent 0xFF */
#define SD_E_CMD_NO_GOING_IDLE 8U
/*!< The card did never sent start data token */
#define SD_E_CMD_NO_START_TOKEN 9U
/*!< The card did never sent start data token */
#define SD_E_CMD_NO_STOP_TRANSMISSION_RESPONSE 10U
/*!< CMD32 (erase range start) failed */
#define SD_E_ERASE_START_FAILED 12U
/*!< CMD33 (erase range end) failed */
#define SD_E_ERASE_END_FAILED 13U
/*!< CMD38 (erase execute) failed */
#define SD_E_ERASE_CMD_FAILED 14U
/*!< Requested operation is not supported for current card type */
#define SD_E_UNSUPPORTED_CARD_TYPE 15U
/*!< Computed card capacity exceeds 32-bit addressable block count */
#define SD_E_CAPACITY_OVERFLOW 16U

/**
 * Enumeration listing the implemented SPI commands.
 * 
 * \note Details of the commands can be found here: https://chlazza.nfshost.com/sdcardinfo.html
 */
enum SD_Spi_Commands
{
    /*!< */
    SD_SPI_CMD0,
    /*!< SEND_OP_COND: Sends host capacity support information and activates
	the card's initialization process. HCS is effective when card receives 
	SEND_IF_COND command. Reserved bits shall be set to '0'.*/
    SD_SPI_CMD1,
    /*!< */
    SD_SPI_CMD8,
    /*!< SPI CMD9: Read the Card-Specific Data (CSD) register */
    SD_SPI_CMD9,
    /*!< STOP_TRANSMISSION: The host can send the STOP_TRANSMISSION (CMD12) 
        command on the CMD pin at any time while a data transfer is in progress. */
    SD_SPI_CMD12,
    /*!< SEND_STATUS: Asks the selected card to send its status register. */
    SD_SPI_CMD13,
    /*!<  SET_BLOCKLEN: set block length of Standard Capacity SD cards 
    (SDHC and SDXC cards have block length always set to 512 bytes) */
    SD_SPI_CMD16,
    /*!< READ_SINGLE_BLOCK */
    SD_SPI_CMD17,
    /*!< READ_MULTI_BLOCK */
    SD_SPI_CMD18,
    /*!< WRITE_BLOCK: Writes a block of the size selected by the SET_BLOCKLEN command. */
    SD_SPI_CMD24,
    /*!< WRITE_MULTI_BLOCK: . */
    SD_SPI_CMD25,
    /*!< ERASE_WR_BLK_START_ADDR: Sets the address of the first write block to be erased.
        Argument: [31:0] Data Address
        Response format: R1

        \note SDSC Card (CCS=0) uses byte unit address and SDHC and SDXC Cards (CCS=1) use block unit address (512 bytes unit)
    */
    SD_SPI_CMD32,
    /*!< ERASE_WR_BLK_END_ADDR: Sets the address of the last write block of the continuous range to be erased. 
        Argument: [31:0] Data Address
        Response format: R1

        \note SDSC Card (CCS=0) uses byte unit address and SDHC and SDXC Cards (CCS=1) use block unit address (512 bytes unit)
    */
    SD_SPI_CMD33,
    /*!< ERASE: Erases all previously selected write blocks 
        Argument: [31:0] Stuff Bits
        Response format: R1b

        \note  R1b: R1 response with an optional trailing busy signal
    */
    SD_SPI_CMD38,
    SD_SPI_CMD55,
    /*!< READ_OCR: . */
    SD_SPI_CMD58,
    /*! SD_SEND_OP_COND: Sends host capacity support information and 
	activates the card's initialization process.
	Reserved bits shall be set to '0'.*/
    SD_SPI_ACMD41,
    SD_SPI_CMDn
};

uint8_t SD_Spi_SendCommand(uint8_t cmd, uint32_t payload);
uint8_t SD_Spi_PowerUp(void);
uint8_t SD_Spi_Initialize(uint8_t);
uint8_t SD_Spi_WaitTillIdle();
uint8_t SD_Spi_GoIdleState(Spi_R1Response *pResponse);
uint8_t SD_Spi_SendIfCond(Spi_R1Response *pResponse);
uint8_t SD_Spi_SendApp(Spi_R1Response *pResponse);
uint8_t SD_Spi_SendOpCond(Spi_R1Response *pResponse);
uint8_t SD_Spi_ReadOCR(Spi_R1Response *pResponse);
uint8_t SD_Spi_ReadRes7(uint8_t *pRxBuffer);
uint8_t SD_Spi_readSingleBlock(uint32_t address, Spi_R1Response *pResponse);
uint8_t
SD_Spi_readMultiBlock(uint32_t address, uint8_t *const buff, uint32_t cnt);
uint8_t SD_Spi_writeBlock(uint32_t address, uint8_t const *buff);
uint8_t
SD_Spi_writeMultiBlock(uint32_t address, uint8_t const *buff, uint32_t cnt);
uint8_t SD_Spi_ReadCSD(SdCsdRegisterType *csd);

/*
 * Trims the entire SD card by issuing sequential ERASE/TRIM ranges
 * (CMD32 / CMD33 / CMD38) across all blocks.
 *
 * This invalidates all card blocks at FTL level without host-side
 * write amplification from zero-filling.
 *
 * WARNING: destroys all data and filesystem structures on the card.
 * The card must be reformatted (FAT32) before FatFS can use it again.
 *
 * @param blocks_trimmed  Out: number of blocks successfully trimmed before
 *                        any error. NULL is accepted.
 * @return SD_E_OK on completion, error code on failure.
 */
uint8_t SD_Spi_TrimAll(uint32_t *blocks_trimmed);

/*
 * Erases the entire SD card using the SD erase command sequence
 * (CMD32 / CMD33 / CMD38), resetting FTL internal state by marking
 * all physical blocks as free without copy-on-write overhead.
 *
 * More effective than a zero-fill for restoring new-card write latency
 * behaviour after FTL fragmentation has developed.
 *
 * WARNING: destroys all data and filesystem structures on the card.
 * The card must be reformatted (FAT32) before FatFS can use it again.
 *
 * @param blocks_erased  Out: number of blocks successfully erased before
 *                       any error. NULL is accepted.
 * @return SD_E_OK on completion, error code on failure.
 */
uint8_t SD_Spi_EraseAll(uint32_t *blocks_erased);

DRESULT SD_Spi_hotReset(void);

uint8_t SD_Spi_GetReadBytes(uint8_t *buff);

void Sd_Spi_ErrorHandlerHook(ErrorContextType *context);

uint8_t Sd_Spi_OsTaskDelayHook(uint32_t delay);

#endif /* CM7_INC_SD_H_ */
