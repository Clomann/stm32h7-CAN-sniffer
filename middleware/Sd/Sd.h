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
#include "Spi_Cmds.h"


#define SD_SPI_CMD_DUMMY_DATA			0xFF
#define SD_SPI_CMD_START_TOKEN			0xFE

#define SD_TOTAL_SIZE					0x800000000	/* 32 Gb in bytes */
#define SD_SECTOR_LENGTH				0x000000200	/* Sector lemngth in bytes (512 bytes) */
#define SD_SECTOR_COUNT					(SD_TOTAL_SIZE/SD_BLOCK_LENGTH)
#define SD_MAX_READ_RESPONSE_ATTEMPTS	(1563U)

/*!The sector is the smallest individual reference-able regions on a disk.*/
#define SD_SDHC_SECTOR_SIZE		512U 

#define SD_SPI_CSD_LENGTH		16U	/*!< Length of the CSD data send by the SD card in bytes. */

#define SD_DEF_DATA_RESP_TOKEN 		0x05
#define SD_DEF_START_DATA_MARKER	0xFE

#define SD_E_OK						 0U 
#define SD_E_CMD_NO_R1				 1U /*!< The card did not send R1 after receiving a command */
#define SD_E_CMD_NO_DATA_RESP_TOKEN	 2U /*!< The card sent 0xFF after receiving a command */
#define SD_E_CMD_NO_GOING_IDLE	 	 3U /*!< The card did never sent 0xFF */
#define SD_E_CMD_LOST_CONNECTON	 	 4U /*!< The card did never sent 0xFF */

/**
 * Enumeration listing the implemented SPI commands.
 * 
 * \note Details of the commands can be found here: https://chlazza.nfshost.com/sdcardinfo.html
 */
enum SD_Spi_Commands {
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
	/*!< SEND_STATUS: Asks the selected card to send its status register. */
	SD_SPI_CMD13, 
	/*!<  SET_BLOCKLEN: set block length of Standard Capacity SD cards (SDHC and SDXC cards have block length always set to 512 bytes) */
	SD_SPI_CMD16, 
	/*!< READ_SINGLE_BLOCK */
	SD_SPI_CMD17, 
	/*!< WRITE_BLOCK: Writes a block of the size selected by the SET_BLOCKLEN command. */
	SD_SPI_CMD24, 
	SD_SPI_CMD55,
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
uint8_t SD_Spi_GoIdleState(Spi_R1Response * pResponse);
uint8_t SD_Spi_SendIfCond(Spi_R1Response * pResponse);
uint8_t SD_Spi_SendApp(Spi_R1Response * pResponse);
uint8_t SD_Spi_SendOpCond(Spi_R1Response * pResponse);
uint8_t SD_Spi_ReadOCR(Spi_R1Response * pResponse);
uint8_t SD_Spi_ReadRes7(uint8_t * pRxBuffer);
uint8_t SD_Spi_readSingleBlock(uint32_t address, Spi_R1Response * pResponse);
uint8_t SD_Spi_writeBlock(uint32_t address, uint8_t const *buff);
uint8_t SD_Spi_ReadCSD(SdCsdRegisterType * csd);

uint8_t SD_Spi_GetReadBytes(uint8_t * buff);

#endif /* CM7_INC_SD_H_ */
