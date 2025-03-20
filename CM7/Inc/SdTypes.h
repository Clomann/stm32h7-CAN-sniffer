/*
 * SdTypes.h
 *
 *  Created on: Jan 1, 2025
 *      Author: cbromann
 */

#ifndef CM7_INC_SDTYPES_H_
#define CM7_INC_SDTYPES_H_

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

/**
 * Card-Specific Data (CSD) register structure v. 2.0
 *
 * \note Detailed information: http://problemkaputt.de/gbatek-dsi-sd-mmc-protocol-csd-register-128bit-card-specific-data-version-2-0.htm
 */
typedef struct {
	uint8_t csdStructure : 2;      /*!< [127:126] CSD structure version */
	uint8_t reserved1 : 6;         /*!< [125:120] Reserved bits */
	uint8_t taac : 8;              /*!< [119:112] Data read access time 1 */
	uint8_t nsac : 8;              /*!< [111:104] Data read access time 2 in CLK cycles */
	uint8_t tranSpeed : 8;         /*!< [103:96] Max data transfer rate */
	uint16_t ccc : 12;             /*!< [95:84] Card command classes */
	uint8_t readBlLen : 4;         /*!< [83:80] Max read data block length */
	uint8_t readBlPartial : 1;     /*!< [79] Partial block read allowed */
	uint8_t writeBlkMisalign : 1;  /*!< [78] Write block misalignment */
	uint8_t readBlkMisalign : 1;   /*!< [77] Read block misalignment */
	uint8_t dsrImp : 1;            /*!< [76] DSR implemented */
	uint8_t reserved2 : 6;         /*!< [75:70] Reserved bits */
	uint32_t cSize : 22;           /*!< [69:48] Device size (memory capacity = (C_SIZE+1) * 512KByte) */
	uint8_t reserved3: 1;
	uint8_t eraseBlkEn : 1;        /*!< [46] Erase single block enable */
	uint8_t sectorSize : 7;        /*!< [45:39] Erase sector size */
	uint8_t wpGrpSize : 7;         /*!< [38:32] Write protect group size */
	uint8_t wpGrpEnable : 1;       /*!< [31] Write protect group enable */
	uint8_t reserved4 : 2;         /*!< [30:29] Reserved bits */
	uint8_t r2wFactor : 3;         /*!< [28:26] Write speed factor */
	uint8_t writeBlLen : 4;        /*!< [25:22] Max write block length */
	uint8_t writeBlPartial : 1;    /*!< [21] Partial block write allowed */
	uint8_t reserved5 : 5;         /*!< [20:16] Reserved bits */
	uint8_t fileFormatGrp : 1;     /*!< [15] File format group */
	uint8_t copy : 1;              /*!< [14] Copy flag */
	uint8_t permWriteProtect : 1;  /*!< [13] Permanent write protection */
	uint8_t tempWriteProtect : 1;  /*!< [12] Temporary write protection */
	uint8_t fileFormat : 2;        /*!< [11:10] File format */
	uint8_t reserved6: 2;
	uint8_t crc : 7;               /*!< [7:1] CRC checksum */
	uint8_t always1 : 1;           /*!< [0] Always 1 */
} SdCsdRegisterType;

#define DEFAULT_SPEED_MODE		(0x32) /*!< SDSC/SDHC/SDXC in Default Speed mode (25MHz) */
#define HIGH_SPEED_MODE			(0x5A) /*!< SDSC/SDHC/SDXC in High Speed mode  (50MHz) */
#define SDR50_OR_DDR50_MODE		(0x0B) /*!< SDHC/SDXC in SDR50 or DDR50 mode (100Mbit/sec) */
#define SDR104_MODE				(0x2B) /*!< SDHC/SDXC in SDR104 mode (200Mbit/sec) */

typedef uint8_t SdSpiTranSpeed;

// Field bit masks and starting positions
#define SD_CSD_CSD_STRUCTURE_MSK    	0x03 /*!< Mask for [127:126] */
#define SD_CSD_CSD_STRUCTURE_START  	126U /*!< Start bit position */

#define SD_CSD_TAAC_MSK             	0xFF /*!< Mask for [119:112] */
#define SD_CSD_TAAC_START           	112U /*!< Start bit position */

#define SD_CSD_NSAC_MSK             	0xFF /*!< Mask for [111:104] */
#define SD_CSD_NSAC_START           	104U /*!< Start bit position */

#define SD_CSD_TRAN_SPEED_MSK      		0xFF /*!< Mask for [103:96] */
#define SD_CSD_TRAN_SPEED_START     	96U  /*!< Start bit position */

#define SD_CSD_CCC1_MSK              	0x0F /*!< Mask for [95:92] */
#define SD_CSD_CCC1_START            	92U  /*!< Start bit position */

#define SD_CSD_CCC0_MSK              	0xFF /*!< Mask for [91:84] */
#define SD_CSD_CCC0_START            	84U  /*!< Start bit position */

#define SD_CSD_READ_BL_LEN_MSK      	0x0F /*!< Mask for [83:80] */
#define SD_CSD_READ_BL_LEN_START    	80U  /*!< Start bit position */

#define SD_CSD_READ_BL_PARTIAL_MSK  	0x01 /*!< Mask for [79] */
#define SD_CSD_READ_BL_PARTIAL_START 	79U  /*!< Start bit position */

#define SD_CSD_WRITE_BLK_MISALIGN_MSK 	0x01 /*!< Mask for [78] */
#define SD_CSD_WRITE_BLK_MISALIGN_START 78U  /*!< Start bit position */

#define SD_CSD_READ_BLK_MISALIGN_MSK 	0x01 /*!< Mask for [77] */
#define SD_CSD_READ_BLK_MISALIGN_START 	77U  /*!< Start bit position */

#define SD_CSD_DSR_IMP_MSK          	0x01 /*!< Mask for [76] */
#define SD_CSD_DSR_IMP_START        	76U  /*!< Start bit position */

#define SD_CSD_C_SIZE2_MSK           	0x3F /*!< Mask for [69:64] */
#define SD_CSD_C_SIZE2_START         	64U  /*!< Start bit position */

#define SD_CSD_C_SIZE1_MSK           	0xFF /*!< Mask for [63:56] */
#define SD_CSD_C_SIZE1_START         	56U  /*!< Start bit position */

#define SD_CSD_C_SIZE0_MSK           	0xFF /*!< Mask for [55:48] */
#define SD_CSD_C_SIZE0_START         	48U  /*!< Start bit position */

#define SD_CSD_ERASE_BLK_EN_MSK     	0x01 /*!< Mask for [46] */
#define SD_CSD_ERASE_BLK_EN_START   	46U  /*!< Start bit position */

#define SD_CSD_SECTOR_SIZE_MSK      	0x7F /*!< Mask for [45:39] */
#define SD_CSD_SECTOR_SIZE_START    	39U  /*!< Start bit position */

#define SD_CSD_WP_GRP_SIZE_MSK      	0x7F /*!< Mask for [38:32] */
#define SD_CSD_WP_GRP_SIZE_START    	32U  /*!< Start bit position */

#define SD_CSD_WP_GRP_ENABLE_MSK    	0x01 /*!< Mask for [31] */
#define SD_CSD_WP_GRP_ENABLE_START  	31U  /*!< Start bit position */

#define SD_CSD_R2W_FACTOR_MSK       	0x07 /*!< Mask for [28:26] */
#define SD_CSD_R2W_FACTOR_START     	26U  /*!< Start bit position */

#define SD_CSD_WRITE_BL_LEN_MSK     	0x0F /*!< Mask for [25:22] */
#define SD_CSD_WRITE_BL_LEN_START   	22U  /*!< Start bit position */

#define SD_CSD_WRITE_BL_PARTIAL_MSK 	0x01 /*!< Mask for [21] */
#define SD_CSD_WRITE_BL_PARTIAL_START 	21U  /*!< Start bit position */

#define SD_CSD_FILE_FORMAT_GRP_MSK  	0x01 /*!< Mask for [15] */
#define SD_CSD_FILE_FORMAT_GRP_START 	15U  /*!< Start bit position */

#define SD_CSD_COPY_MSK             	0x01 /*!< Mask for [14] */
#define SD_CSD_COPY_START           	14U  /*!< Start bit position */

#define SD_CSD_PERM_WRITE_PROTECT_MSK 	0x01 /*!< Mask for [13] */
#define SD_CSD_PERM_WRITE_PROTECT_START 13U  /*!< Start bit position */

#define SD_CSD_TEMP_WRITE_PROTECT_MSK 	0x01 /*!< Mask for [12] */
#define SD_CSD_TEMP_WRITE_PROTECT_START 12U  /*!< Start bit position */

#define SD_CSD_FILE_FORMAT_MSK      	0x03 /*!< Mask for [11:10] */
#define SD_CSD_FILE_FORMAT_START    	10U  /*!< Start bit position */

#define SD_CSD_CRC_MSK              	0x7F /*!< Mask for [7:1] */
#define SD_CSD_CRC_START            	1U   /*!< Start bit position */

#define SD_CSD_ALWAYS1_MSK              0x01 /*!< Mask for [0] */
#define SD_CSD_ALWAYS1_START            0U   /*!< Start bit position */

#define GET_BYTE_INDEX(bitpos, elemSize) \
	((bitpos) / elemSize)

#define EXTRACT_CSD_BITS(array, mask, start) \
		( array[GET_BYTE_INDEX(start, 8U)] >> (start - (GET_BYTE_INDEX(start, 8U) * 8U)) & mask )

#endif /* CM7_INC_SDTYPES_H_ */
