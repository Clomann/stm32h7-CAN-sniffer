/*
 * Spi_Cmds.h
 *
 *  Created on: Dec 4, 2024
 *      Author: cbromann
 */

#ifndef CM7_INC_SPI_CMDS_H_
#define CM7_INC_SPI_CMDS_H_

#include <stdint.h>

typedef union {
	struct {
		uint8_t idle_state : 1;         // Bit 7: Idle state
		uint8_t erase_reset : 1;        // Bit 6: Erase reset
		uint8_t illegal_command : 1;    // Bit 5: Illegal command
		uint8_t crc_error : 1;          // Bit 4: CRC error
		uint8_t erase_seq_error : 1;    // Bit 3: Erase sequence error
		uint8_t address_error : 1;      // Bit 2: Address error
		uint8_t parameter_error : 1;    // Bit 1: Parameter error
		uint8_t reserved : 1;           // Bit 0: Always 0 in SPI mode
	};
	uint8_t byte;
} Spi_R1Response;


typedef union {
	struct {
		uint32_t cpusb : 1;           // Bit 31: Card Power up Status Bit (busy)
		uint32_t ccs : 1;      		 // Bit 30: Card Capacity Status (CCS)
		uint32_t uhs_ii_card : 1;     // Bit 29: UHS-II card (always 0 in SPI mode)
		uint32_t reserved_2 : 4;     // Bits 25–28: Reserved (always 0)
		uint32_t switching_1_8_V : 1; // Bit 24: Switching to 1.8 V accepted
		uint32_t voltage_3_5_3_6 : 1; // Bit 23: Voltage range 3.5-3.6V support
		uint32_t voltage_3_4_3_5 : 1; // Bit 22: Voltage range 3.4-3.5V support
		uint32_t voltage_3_3_3_4 : 1; // Bit 21: Voltage range 3.3-3.4V support
		uint32_t voltage_3_2_3_3 : 1; // Bit 20: Voltage range 3.2-3.3V support
		uint32_t voltage_3_1_3_2 : 1; // Bit 19: Voltage range 3.1-3.2V support
		uint32_t voltage_3_0_3_1 : 1; // Bit 18: Voltage range 3.0-3.1V support
		uint32_t voltage_2_9_3_0 : 1; // Bit 17: Voltage range 2.9-3.0V support
		uint32_t voltage_2_8_2_9 : 1; // Bit 16: Voltage range 2.8-2.9V support
		uint32_t voltage_2_7_2_8 : 1; // Bit 15: Voltage range 2.7-2.8V support
		uint32_t reserved_1 : 15;     // Bits 0–14: Reserved (always 0)
	};
	uint32_t bytes;
} OCR_Register;

enum SD_Spi_Commands {
	SD_SPI_CMD0,
	SD_SPI_CMD8,
	SD_SPI_CMD16, // SET_BLOCKLEN: set block length of Standard Capacity SD cards (SDHC and SDXC cards have block length always set to 512 bytes)
	SD_SPI_CMD17, // READ_SINGLE_BLOCK
	SD_SPI_CMD24,
	SD_SPI_CMD55,
	SD_SPI_CMD58,
	SD_SPI_ACMD41,
	SD_SPI_CMDn
};

const uint8_t aTxSpiInit[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
const uint8_t aTxSpiDummy1[] = {0xFF};
const uint8_t aTxSpiDummy4[] = {0xFF, 0xFF, 0xFF, 0xFF};

uint8_t aTxSpiCmd[6];

#define SPI1_TX_BUFFER_SIZE 128
#define SPI1_RX_BUFFER_SIZE 128
#define SPI2_TX_BUFFER_SIZE 256
#define SPI2_RX_BUFFER_SIZE 256

uint8_t spi1_tx_buffer[SPI1_TX_BUFFER_SIZE];
uint8_t spi1_rx_buffer[SPI1_RX_BUFFER_SIZE];
uint8_t spi2_tx_buffer[SPI2_TX_BUFFER_SIZE];
uint8_t spi2_rx_buffer[SPI2_RX_BUFFER_SIZE];

static uint8_t SD_Spi_CreateCommand(uint8_t cmd, uint32_t payload, uint8_t * buffer)
{
	uint8_t RetVal;


	switch (cmd) {
	case SD_SPI_CMD0:
		buffer[0] = 0x40;
		buffer[1] = 0x00;
		buffer[2] = 0x00;
		buffer[3] = 0x00;
		buffer[4] = 0x00;
		buffer[5] = 0x95;
		break;
	case SD_SPI_CMD8:
		buffer[0] = 0x48;
		buffer[1] = 0x00;
		buffer[2] = 0x00;
		buffer[3] = 0x01;
		buffer[4] = 0xAA;
		buffer[5] = 0x87;
		break;
	case SD_SPI_CMD16:
		buffer[0] = 0x50;
		buffer[1] = (payload >> 24) & 0xFF;
		buffer[2] = (payload >> 16) & 0xFF;
		buffer[3] = (payload >> 8) & 0xFF;
		buffer[4] = payload & 0xFF;
		buffer[5] = 0x01;
	case SD_SPI_CMD17:
		buffer[0] = 0x51;
		buffer[1] = (payload >> 24) & 0xFF;
		buffer[2] = (payload >> 16) & 0xFF;
		buffer[3] = (payload >> 8) & 0xFF;
		buffer[4] = payload & 0xFF;
		buffer[5] = 0x01;
		break;
	case SD_SPI_CMD24:
		buffer[0] = 0x58;
		buffer[1] = (payload >> 24) & 0xFF;
		buffer[2] = (payload >> 16) & 0xFF;
		buffer[3] = (payload >> 8) & 0xFF;
		buffer[4] = payload & 0xFF;
		buffer[5] = 0x01;
		break;
	case SD_SPI_CMD55:
		buffer[0] = 0x77;
		buffer[1] = 0x00;
		buffer[2] = 0x00;
		buffer[3] = 0x00;
		buffer[4] = 0x00;
		buffer[5] = 0x01;
		break;
	case SD_SPI_CMD58:
		buffer[0] = 0x7A;
		buffer[1] = 0x00;
		buffer[2] = 0x00;
		buffer[3] = 0x00;
		buffer[4] = 0x00;
		buffer[5] = 0x01;
		break;
	case SD_SPI_ACMD41:
		buffer[0] = 0x69;
		buffer[1] = (payload >> 24) & 0xFF;
		buffer[2] = (payload >> 16) & 0xFF;
		buffer[3] = (payload >> 8) & 0xFF;
		buffer[4] = payload & 0xFF;
		buffer[5] = 0x01;
		break;
	default:
		buffer[0] = 0xFF;
		buffer[1] = 0xFF;
		buffer[2] = 0xFF;
		buffer[3] = 0xFF;
		buffer[4] = 0xFF;
		buffer[5] = 0xFF;
		RetVal = 1;
	}

	return RetVal;
}

// Bit positions for OCR fields
#define OCR_BUSY_BIT_POS          31  // Card Power-Up Status (Busy)
#define OCR_CCS_BIT_POS           30  // Card Capacity Status (CCS)
#define OCR_UHSII_BIT_POS         29  // UHS-II Card (always 0 in SPI mode)
#define OCR_SWITCH_1_8V_BIT_POS   24  // Switching to 1.8V accepted

// Bit positions for Voltage Range Support
#define OCR_VOLTAGE_3_5_3_6_POS   23  // Voltage range 3.5V - 3.6V
#define OCR_VOLTAGE_3_4_3_5_POS   22  // Voltage range 3.4V - 3.5V
#define OCR_VOLTAGE_3_3_3_4_POS   21  // Voltage range 3.3V - 3.4V
#define OCR_VOLTAGE_3_2_3_3_POS   20  // Voltage range 3.2V - 3.3V
#define OCR_VOLTAGE_3_1_3_2_POS   19  // Voltage range 3.1V - 3.2V
#define OCR_VOLTAGE_3_0_3_1_POS   18  // Voltage range 3.0V - 3.1V
#define OCR_VOLTAGE_2_9_3_0_POS   17  // Voltage range 2.9V - 3.0V
#define OCR_VOLTAGE_2_8_2_9_POS   16  // Voltage range 2.8V - 2.9V
#define OCR_VOLTAGE_2_7_2_8_POS   15  // Voltage range 2.7V - 2.8V

// Bit mask for reserved fields
#define OCR_RESERVED_25_28_MASK   (0x0F << 25)  // Bits 25–28
#define OCR_RESERVED_0_14_MASK    (0x7FFF)      // Bits 0–14

// Check Power-Up Status (Bit 31)
#define IS_CARD_READY(ocr)        (((ocr) >> OCR_BUSY_BIT_POS) & 0x1)

// Check Card Capacity Status (Bit 30)
#define IS_CARD_SDHX(ocr)         (((ocr) >> OCR_CCS_BIT_POS) & 0x1)

// Check UHS-II Card Status (Bit 29)
#define IS_UHSII_CARD(ocr)        (((ocr) >> OCR_UHSII_BIT_POS) & 0x1)

// Check Switching to 1.8V (Bit 24)
#define IS_SWITCH_1_8V(ocr)       (((ocr) >> OCR_SWITCH_1_8V_BIT_POS) & 0x1)

// Check Voltage Range Bits
#define IS_VOLTAGE_3_5_3_6(ocr)   (((ocr) >> OCR_VOLTAGE_3_5_3_6_POS) & 0x1)
#define IS_VOLTAGE_3_4_3_5(ocr)   (((ocr) >> OCR_VOLTAGE_3_4_3_5_POS) & 0x1)
#define IS_VOLTAGE_3_3_3_4(ocr)   (((ocr) >> OCR_VOLTAGE_3_3_3_4_POS) & 0x1)
#define IS_VOLTAGE_3_2_3_3(ocr)   (((ocr) >> OCR_VOLTAGE_3_2_3_3_POS) & 0x1)
#define IS_VOLTAGE_3_1_3_2(ocr)   (((ocr) >> OCR_VOLTAGE_3_1_3_2_POS) & 0x1)
#define IS_VOLTAGE_3_0_3_1(ocr)   (((ocr) >> OCR_VOLTAGE_3_0_3_1_POS) & 0x1)
#define IS_VOLTAGE_2_9_3_0(ocr)   (((ocr) >> OCR_VOLTAGE_2_9_3_0_POS) & 0x1)
#define IS_VOLTAGE_2_8_2_9(ocr)   (((ocr) >> OCR_VOLTAGE_2_8_2_9_POS) & 0x1)
#define IS_VOLTAGE_2_7_2_8(ocr)   (((ocr) >> OCR_VOLTAGE_2_7_2_8_POS) & 0x1)

static inline void SD_Spi_Ocr2Bitfield(uint32_t ocr, OCR_Register * out)
{
	out->ccs = IS_CARD_SDHX(ocr);
	out->cpusb = IS_CARD_READY(ocr);
	out->switching_1_8_V = IS_SWITCH_1_8V(ocr);
	out->uhs_ii_card = IS_UHSII_CARD(ocr);
	out->voltage_2_7_2_8 = IS_VOLTAGE_2_7_2_8(ocr);
	out->voltage_2_8_2_9 = IS_VOLTAGE_2_8_2_9(ocr);
	out->voltage_2_9_3_0 = IS_VOLTAGE_2_9_3_0(ocr);
	out->voltage_3_0_3_1 = IS_VOLTAGE_3_0_3_1(ocr);
	out->voltage_3_2_3_3 = IS_VOLTAGE_3_2_3_3(ocr);
	out->voltage_3_3_3_4 = IS_VOLTAGE_3_3_3_4(ocr);
	out->voltage_3_4_3_5 = IS_VOLTAGE_3_4_3_5(ocr);
	out->voltage_3_5_3_6 = IS_VOLTAGE_3_5_3_6(ocr);

}

//uint8_t SD_Spi_SendCommand(uint8_t cmd, uint32_t payload);
//uint8_t SD_Spi_GoIdleState(Spi_R1Response * pResponse);
//uint8_t SD_Spi_SendIfCond(Spi_R1Response * pResponse);
//uint8_t SD_Spi_SendApp(Spi_R1Response * pResponse);
//uint8_t SD_Spi_SendOpCond(Spi_R1Response * pResponse);
//uint8_t SD_Spi_ReadOCR(Spi_R1Response * pResponse);
//uint8_t SD_Spi_ReadRes7(uint8_t * pRxBuffer);
//uint8_t SD_readSingleBlock(uint8_t CsLine, uint32_t address, Spi_R1Response * pResponse);

#endif /* CM7_INC_SPI_CMDS_H_ */
