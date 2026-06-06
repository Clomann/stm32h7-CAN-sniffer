/*
 * Sd.c
 *
 *  Created on: Dec 26, 2024
 *      Author: cbromann
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "stm32h7xx_hal.h"

#include "Sd.h"
#include "SpiAbs.h"

#undef COUNTOF
#define COUNTOF(__BUFFER__) (sizeof(__BUFFER__) / sizeof(*(__BUFFER__)))

// Bit positions for OCR fields
#define OCR_BUSY_BIT_POS        31 // Card Power-Up Status (Busy)
#define OCR_CCS_BIT_POS         30 // Card Capacity Status (CCS)
#define OCR_UHSII_BIT_POS       29 // UHS-II Card (always 0 in SPI mode)
#define OCR_SWITCH_1_8V_BIT_POS 24 // Switching to 1.8V accepted

// Bit positions for Voltage Range Support
#define OCR_VOLTAGE_3_5_3_6_POS 23 // Voltage range 3.5V - 3.6V
#define OCR_VOLTAGE_3_4_3_5_POS 22 // Voltage range 3.4V - 3.5V
#define OCR_VOLTAGE_3_3_3_4_POS 21 // Voltage range 3.3V - 3.4V
#define OCR_VOLTAGE_3_2_3_3_POS 20 // Voltage range 3.2V - 3.3V
#define OCR_VOLTAGE_3_1_3_2_POS 19 // Voltage range 3.1V - 3.2V
#define OCR_VOLTAGE_3_0_3_1_POS 18 // Voltage range 3.0V - 3.1V
#define OCR_VOLTAGE_2_9_3_0_POS 17 // Voltage range 2.9V - 3.0V
#define OCR_VOLTAGE_2_8_2_9_POS 16 // Voltage range 2.8V - 2.9V
#define OCR_VOLTAGE_2_7_2_8_POS 15 // Voltage range 2.7V - 2.8V

// Bit mask for reserved fields
#define OCR_RESERVED_25_28_MASK (0x0F << 25) // Bits 25–28
#define OCR_RESERVED_0_14_MASK  (0x7FFF) // Bits 0–14

// Check Power-Up Status (Bit 31)
#define IS_CARD_READY(ocr) (((ocr) >> OCR_BUSY_BIT_POS) & 0x1)

// Check Card Capacity Status (Bit 30)
#define IS_CARD_SDHX(ocr) (((ocr) >> OCR_CCS_BIT_POS) & 0x1)

// Check UHS-II Card Status (Bit 29)
#define IS_UHSII_CARD(ocr) (((ocr) >> OCR_UHSII_BIT_POS) & 0x1)

// Check Switching to 1.8V (Bit 24)
#define IS_SWITCH_1_8V(ocr) (((ocr) >> OCR_SWITCH_1_8V_BIT_POS) & 0x1)

// Check Voltage Range Bits
#define IS_VOLTAGE_3_5_3_6(ocr) (((ocr) >> OCR_VOLTAGE_3_5_3_6_POS) & 0x1)
#define IS_VOLTAGE_3_4_3_5(ocr) (((ocr) >> OCR_VOLTAGE_3_4_3_5_POS) & 0x1)
#define IS_VOLTAGE_3_3_3_4(ocr) (((ocr) >> OCR_VOLTAGE_3_3_3_4_POS) & 0x1)
#define IS_VOLTAGE_3_2_3_3(ocr) (((ocr) >> OCR_VOLTAGE_3_2_3_3_POS) & 0x1)
#define IS_VOLTAGE_3_1_3_2(ocr) (((ocr) >> OCR_VOLTAGE_3_1_3_2_POS) & 0x1)
#define IS_VOLTAGE_3_0_3_1(ocr) (((ocr) >> OCR_VOLTAGE_3_0_3_1_POS) & 0x1)
#define IS_VOLTAGE_2_9_3_0(ocr) (((ocr) >> OCR_VOLTAGE_2_9_3_0_POS) & 0x1)
#define IS_VOLTAGE_2_8_2_9(ocr) (((ocr) >> OCR_VOLTAGE_2_8_2_9_POS) & 0x1)
#define IS_VOLTAGE_2_7_2_8(ocr) (((ocr) >> OCR_VOLTAGE_2_7_2_8_POS) & 0x1)

#define SD_KINGSTON_HEALTH_BLOCK_LEN 512U

static ErrorContextType ErrorContext = {.file = __FILE_NAME__};

/**
 * @brief Debug-only counter of SD hot-reset calls.
 *
 * Used during GDB debugging and may later be surfaced in logging.
 */
static volatile uint32_t HotResetCallCount = 0U;

/* Buffer used for transmission */
static uint8_t SPI_CMD_READ_BUFFER[SD_SDHC_SECTOR_SIZE] = {0};

static const uint8_t SD_KingstonFlashId_SDCIT[] =
    {0x98, 0x3C, 0x98, 0xB3, 0x76, 0x72, 0x08, 0x0E, 0x00};
static const uint8_t SD_KingstonFlashId_SDCIT2[] =
    {0x98, 0x3C, 0x98, 0xB3, 0xF6, 0xE3, 0x08, 0x1E, 0x00};
static const uint8_t SD_KingstonFlashId_SDCE[] =
    {0x98, 0x3E, 0xA8, 0x03, 0x7A, 0xE4, 0x08, 0x16, 0x00};

uint8_t aTxSpiCmd[7];

ALIGN_32BYTES(const uint8_t __attribute__((used, section(".dma_buffer.ro"))
) aTxSpiInit[18]) = {
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF
};

static uint8_t
SD_Spi_CreateCommand(uint8_t cmd, uint32_t payload, uint8_t *buffer)
{
    uint8_t RetVal = SD_E_OK;

    // first dummy byte to give SD card some idle time
    buffer[0] = 0xFF;

    switch (cmd)
    {
    case SD_SPI_CMD0:
        buffer[1] = 0x40;
        buffer[2] = 0x00;
        buffer[3] = 0x00;
        buffer[4] = 0x00;
        buffer[5] = 0x00;
        buffer[6] = 0x95;
        break;
    case SD_SPI_CMD8:
        buffer[1] = 0x48;
        buffer[2] = 0x00;
        buffer[3] = 0x00;
        buffer[4] = 0x01;
        buffer[5] = 0xAA;
        buffer[6] = 0x87;
        break;
    case SD_SPI_CMD9:
        buffer[1] = 0x49;
        buffer[2] = (payload >> 24) & 0xFF;
        buffer[3] = (payload >> 16) & 0xFF;
        buffer[4] = (payload >> 8) & 0xFF;
        buffer[5] = payload & 0xFF;
        buffer[6] = 0x01;
        break;
    case SD_SPI_CMD12:
        buffer[1] = 0x4C;
        buffer[2] = 0x00;
        buffer[3] = 0x00;
        buffer[4] = 0x00;
        buffer[5] = 0x00;
        buffer[6] = 0x01;
        break;
    case SD_SPI_CMD13:
        buffer[1] = 0x4D;
        buffer[2] = 0x00;
        buffer[3] = 0x00;
        buffer[4] = 0x00;
        buffer[5] = 0x00;
        buffer[6] = 0x01;
        break;
    case SD_SPI_CMD16:
        buffer[1] = 0x50;
        buffer[2] = (payload >> 24) & 0xFF;
        buffer[3] = (payload >> 16) & 0xFF;
        buffer[4] = (payload >> 8) & 0xFF;
        buffer[5] = payload & 0xFF;
        buffer[6] = 0x01;
        break;
    case SD_SPI_CMD17:
        buffer[1] = 0x51;
        buffer[2] = (payload >> 24) & 0xFF;
        buffer[3] = (payload >> 16) & 0xFF;
        buffer[4] = (payload >> 8) & 0xFF;
        buffer[5] = payload & 0xFF;
        buffer[6] = 0x01;
        break;
    case SD_SPI_CMD18:
        buffer[1] = 0x52;
        buffer[2] = (payload >> 24) & 0xFF;
        buffer[3] = (payload >> 16) & 0xFF;
        buffer[4] = (payload >> 8) & 0xFF;
        buffer[5] = payload & 0xFF;
        buffer[6] = 0x01;
        break;
    case SD_SPI_CMD24:
        buffer[1] = 0x58;
        buffer[2] = (payload >> 24) & 0xFF;
        buffer[3] = (payload >> 16) & 0xFF;
        buffer[4] = (payload >> 8) & 0xFF;
        buffer[5] = payload & 0xFF;
        buffer[6] = 0x01;
        break;
    case SD_SPI_CMD25:
        buffer[1] = 0x59;
        buffer[2] = (payload >> 24) & 0xFF;
        buffer[3] = (payload >> 16) & 0xFF;
        buffer[4] = (payload >> 8) & 0xFF;
        buffer[5] = payload & 0xFF;
        buffer[6] = 0x01;
        break;
    case SD_SPI_CMD32:
        buffer[1] = 0x40 + 32U;
        buffer[2] = (payload >> 24) & 0xFF;
        buffer[3] = (payload >> 16) & 0xFF;
        buffer[4] = (payload >> 8) & 0xFF;
        buffer[5] = payload & 0xFF;
        buffer[6] = 0x01;
        break;
    case SD_SPI_CMD33:
        buffer[1] = 0x40 + 32U;
        buffer[2] = (payload >> 24) & 0xFF;
        buffer[3] = (payload >> 16) & 0xFF;
        buffer[4] = (payload >> 8) & 0xFF;
        buffer[5] = payload & 0xFF;
        buffer[6] = 0x01;
        break;
    case SD_SPI_CMD38:
        buffer[1] = 0x40 + 38U;
        buffer[2] = 0x00;
        buffer[3] = 0x00;
        buffer[4] = 0x00;
        buffer[5] = 0x00;
        buffer[6] = 0x01;
        break;
    case SD_SPI_CMD55:
        buffer[1] = 0x77;
        buffer[2] = 0x00;
        buffer[3] = 0x00;
        buffer[4] = 0x00;
        buffer[5] = 0x00;
        buffer[6] = 0x01;
        break;
    case SD_SPI_CMD58:
        buffer[1] = 0x7A;
        buffer[2] = 0x00;
        buffer[3] = 0x00;
        buffer[4] = 0x00;
        buffer[5] = 0x00;
        buffer[6] = 0x01;
        break;
    case SD_SPI_CMD56:
        buffer[1] = 0x40 + 56U;
        buffer[2] = (payload >> 24) & 0xFF;
        buffer[3] = (payload >> 16) & 0xFF;
        buffer[4] = (payload >> 8) & 0xFF;
        buffer[5] = payload & 0xFF;
        switch (payload)
        {
        case 0x00000001:
            buffer[6] = 0x45;
            break;
        case 0x00000000:
            buffer[6] = 0x41;
            break;
        case 0x00000010:
            buffer[6] = 0x09;
            break;
        default:
            buffer[6] = 0x01;
            break; // fallback, may not work
        }
        break;
    case SD_SPI_ACMD41:
        buffer[1] = 0x69;
        buffer[2] = (payload >> 24) & 0xFF;
        buffer[3] = (payload >> 16) & 0xFF;
        buffer[4] = (payload >> 8) & 0xFF;
        buffer[5] = payload & 0xFF;
        buffer[6] = 0x01;
        break;
    default:
        buffer[1] = 0xFF;
        buffer[2] = 0xFF;
        buffer[3] = 0xFF;
        buffer[4] = 0xFF;
        buffer[5] = 0xFF;
        buffer[6] = 0xFF;
        RetVal    = SD_E_NOT_OK;
    }

    return RetVal;
}

static void SD_Spi_Ocr2Bitfield(uint32_t ocr, OCR_Register *out)
{
    out->ccs             = IS_CARD_SDHX(ocr);
    out->cpusb           = IS_CARD_READY(ocr);
    out->switching_1_8_V = IS_SWITCH_1_8V(ocr);
    out->uhs_ii_card     = IS_UHSII_CARD(ocr);
    out->voltage_2_7_2_8 = IS_VOLTAGE_2_7_2_8(ocr);
    out->voltage_2_8_2_9 = IS_VOLTAGE_2_8_2_9(ocr);
    out->voltage_2_9_3_0 = IS_VOLTAGE_2_9_3_0(ocr);
    out->voltage_3_0_3_1 = IS_VOLTAGE_3_0_3_1(ocr);
    out->voltage_3_2_3_3 = IS_VOLTAGE_3_2_3_3(ocr);
    out->voltage_3_3_3_4 = IS_VOLTAGE_3_3_3_4(ocr);
    out->voltage_3_4_3_5 = IS_VOLTAGE_3_4_3_5(ocr);
    out->voltage_3_5_3_6 = IS_VOLTAGE_3_5_3_6(ocr);
}

/**
 *
 *
 * \brief
 *
 * \note This function only supports the CSD version 2 type.
 */
static void SD_Spi_Csd2Bitfield(uint8_t *csd, SdCsdRegisterType *out)
{
    uint8_t buf[SD_SPI_CSD_LENGTH];
    uint8_t i = 0U;
    uint8_t CSizeByte0, CSizeByte1, CSizeByte2;
    uint8_t CccByte0, CccByte1;

    for (i = 0U; i < SD_SPI_CSD_LENGTH; i++)
    {
        buf[(SD_SPI_CSD_LENGTH - 1U) - i] = csd[i];
    }

    out->csdStructure = EXTRACT_CSD_BITS(
        buf,
        SD_CSD_CSD_STRUCTURE_MSK,
        SD_CSD_CSD_STRUCTURE_START
    );
    out->taac = EXTRACT_CSD_BITS(buf, SD_CSD_TAAC_MSK, SD_CSD_TAAC_START);
    out->nsac = EXTRACT_CSD_BITS(buf, SD_CSD_NSAC_MSK, SD_CSD_NSAC_START);
    out->tranSpeed =
        EXTRACT_CSD_BITS(buf, SD_CSD_TRAN_SPEED_MSK, SD_CSD_TRAN_SPEED_START);
    CccByte1 = EXTRACT_CSD_BITS(buf, SD_CSD_CCC1_MSK, SD_CSD_CCC1_START);
    CccByte0 = EXTRACT_CSD_BITS(buf, SD_CSD_CCC0_MSK, SD_CSD_CCC0_START);
    out->ccc = (CccByte0 << 8U) | CccByte1;
    out->readBlLen =
        EXTRACT_CSD_BITS(buf, SD_CSD_READ_BL_LEN_MSK, SD_CSD_READ_BL_LEN_START);
    out->readBlPartial = EXTRACT_CSD_BITS(
        buf,
        SD_CSD_READ_BL_PARTIAL_MSK,
        SD_CSD_READ_BL_PARTIAL_START
    );
    //	out->writeBlkMisalign
    //	out->readBlkMisalign
    //	out->dsrImp
    CSizeByte0 =
        EXTRACT_CSD_BITS(buf, SD_CSD_C_SIZE0_MSK, SD_CSD_C_SIZE0_START);
    CSizeByte1 =
        EXTRACT_CSD_BITS(buf, SD_CSD_C_SIZE1_MSK, SD_CSD_C_SIZE1_START);
    CSizeByte2 =
        EXTRACT_CSD_BITS(buf, SD_CSD_C_SIZE2_MSK, SD_CSD_C_SIZE2_START);
    out->cSize = (CSizeByte2 << 16U) | (CSizeByte1 << 8U) | (CSizeByte0);
    //	out->vddRCurrMin
    //	out->vddRCurrMax
    //	out->vddWCurrMin
    //	out->vddWCurrMax
    out->eraseBlkEn = EXTRACT_CSD_BITS(
        buf,
        SD_CSD_ERASE_BLK_EN_MSK,
        SD_CSD_ERASE_BLK_EN_START
    );
    out->sectorSize =
        EXTRACT_CSD_BITS(buf, SD_CSD_SECTOR_SIZE_MSK, SD_CSD_SECTOR_SIZE_START);
    //	out->wpGrpSize = EXTRACT_CSD_BITS(buf, SD_CSD_, SD_CSD_FILE_);
    //	out->wpGrpEnable = EXTRACT_CSD_BITS(buf, SD_CSD_, SD_CSD_FILE_);
    //	out->reserved3 = EXTRACT_CSD_BITS(buf, SD_CSD_, SD_CSD_FILE_);
    //	out->r2wFactor = EXTRACT_CSD_BITS(buf, SD_CSD_, SD_CSD_FILE_);
    //	out->writeBlLen = EXTRACT_CSD_BITS(buf, SD_CSD_, SD_CSD_FILE_);
    //	out->writeBlPartial = EXTRACT_CSD_BITS(buf, SD_CSD_, SD_CSD_FILE_);
    //	out->fileFormatGrp = EXTRACT_CSD_BITS(buf, SD_CSD_, SD_CSD_FILE_);
    //	out->copy = EXTRACT_CSD_BITS(buf, SD_CSD_, SD_CSD_FILE_);
    //	out->permWriteProtect = EXTRACT_CSD_BITS(buf, SD_CSD_, SD_CSD_FILE_);
    //	out->tempWriteProtect = EXTRACT_CSD_BITS(buf, SD_CSD_, SD_CSD_FILE_);
    out->fileFormat =
        EXTRACT_CSD_BITS(buf, SD_CSD_FILE_FORMAT_MSK, SD_CSD_FILE_FORMAT_START);
    out->crc = EXTRACT_CSD_BITS(buf, SD_CSD_CRC_MSK, SD_CSD_CRC_START);
    out->always1 =
        EXTRACT_CSD_BITS(buf, SD_CSD_ALWAYS1_MSK, SD_CSD_ALWAYS1_START);
}

/**
 * @brief Read a big-endian 16-bit value from a byte buffer.
 * @param buffer Source byte buffer.
 * @param offset Start index in @p buffer.
 * @return Decoded 16-bit value.
 */
static uint16_t SD_Spi_ReadBe16(uint8_t const *buffer, uint16_t offset)
{
    return (uint16_t)((uint16_t)buffer[offset] << 8U)
           | (uint16_t)buffer[offset + 1U];
}

/**
 * @brief Read a big-endian 32-bit value from a byte buffer.
 * @param buffer Source byte buffer.
 * @param offset Start index in @p buffer.
 * @return Decoded 32-bit value.
 */
static uint32_t SD_Spi_ReadBe32(uint8_t const *buffer, uint16_t offset)
{
    return ((uint32_t)buffer[offset] << 24U)
           | ((uint32_t)buffer[offset + 1U] << 16U)
           | ((uint32_t)buffer[offset + 2U] << 8U)
           | (uint32_t)buffer[offset + 3U];
}

/**
 * @brief Sum a byte range and return the lower 8 bits.
 * @param buffer Source byte buffer.
 * @param start Inclusive start index.
 * @param end_exclusive Exclusive end index.
 * @return Range sum truncated to 8 bits.
 */
static uint8_t
SD_Spi_SumRangeU8(uint8_t const *buffer, uint16_t start, uint16_t end_exclusive)
{
    uint16_t sum = 0U;
    for (uint16_t i = start; i < end_exclusive; i++)
    {
        sum += buffer[i];
    }
    return (uint8_t)sum;
}

/**
 * @brief Check whether a payload is plausibly valid health data.
 * @param buffer Payload bytes.
 * @param len Payload length in bytes.
 * @return true if payload is neither all 0x00 nor all 0xFF.
 */
static bool SD_Spi_IsDataValid(uint8_t const *buffer, uint16_t len)
{
    bool only00 = true;
    bool onlyFF = true;

    for (uint16_t i = 0U; i < len; i++)
    {
        if (buffer[i] != 0x00U)
        {
            only00 = false;
        }

        if (buffer[i] != 0xFFU)
        {
            onlyFF = false;
        }

        if ((false == only00) && (false == onlyFF))
        {
            break;
        }
    }

    return ((false == only00) && (false == onlyFF));
}

/**
 * @brief Check whether payload starts with a known Kingston signature.
 * @param buffer Payload bytes.
 * @return true if one supported signature matches.
 */
static bool SD_Spi_IsKnownKingstonHealthSignature(uint8_t const *buffer)
{
    return (
        (0
         == memcmp(
             buffer,
             SD_KingstonFlashId_SDCIT,
             sizeof(SD_KingstonFlashId_SDCIT)
         ))
        || (0
            == memcmp(
                buffer,
                SD_KingstonFlashId_SDCIT2,
                sizeof(SD_KingstonFlashId_SDCIT2)
            ))
        || (0
            == memcmp(
                buffer,
                SD_KingstonFlashId_SDCE,
                sizeof(SD_KingstonFlashId_SDCE)
            ))
    );
}

/**
 * @brief Poll until a non-0xFF byte is received.
 * @param out Out: received byte.
 * @param maxAttempts Maximum poll attempts.
 * @return SD_E_OK on success, SD_E_CMD_NO_R1 on timeout.
 */
static uint8_t SD_Spi_WaitNonFF(uint8_t *out, uint32_t maxAttempts)
{
    uint32_t attempts = 0U;

    *out = 0xFFU;
    while (attempts++ < maxAttempts)
    {
        SpiAbs_readByte(SPIABS_DEVICE_1, out);
        if (*out != 0xFFU)
        {
            return SD_E_OK;
        }
    }

    return SD_E_CMD_NO_R1;
}

/**
 * @brief Validate an R1 response returned by a CMD56 transaction.
 * @param r1 Raw R1 response byte.
 * @return SD_E_OK for a ready card, SD_E_HEALTH_NOT_SUPPORTED for cards that
 *         reject CMD56, otherwise SD_E_RESPONSE.
 */
static uint8_t SD_Spi_Cmd56_CheckR1(uint8_t r1)
{
    Spi_R1Response resp = {.byte = r1};

    if (0x00U == resp.byte)
    {
        return SD_E_OK;
    }

    if (resp.illegal_command != 0U)
    {
        return SD_E_HEALTH_NOT_SUPPORTED;
    }

    return SD_E_RESPONSE;
}

/**
 * @brief Send CMD56 and read its immediate R1 response.
 * @param payload CMD56 argument.
 * @param r1 Out: R1 response byte.
 * @return SD_E_OK on success, otherwise SD_E_* error code.
 */
static uint8_t SD_Spi_Cmd56_PollR1(uint32_t payload, uint8_t *r1)
{
    uint8_t dummy[SD_SPI_PRE_CMD_CLOCKS];
    uint8_t res;

    SpiAbs_CsDisable(SPIABS_DEVICE_1);
    SpiAbs_Receive_Spi1_Task0(dummy, sizeof(dummy));
    SpiAbs_CsEnable(SPIABS_DEVICE_1);

    SD_Spi_SendCommand(SD_SPI_CMD56, payload);
    res = SD_Spi_WaitNonFF(r1, SD_MAX_READ_RESPONSE_ATTEMPTS);
    if (SD_E_OK == res)
    {
        res = SD_Spi_Cmd56_CheckR1(*r1);
    }

    SpiAbs_CsDisable(SPIABS_DEVICE_1);
    return res;
}

/**
 * @brief Send CMD56 write variant and transmit one helper data block.
 * @param payload CMD56 argument.
 * @return SD_E_OK on success, otherwise SD_E_* error code.
 */
static uint8_t SD_Spi_Cmd56_PreloadWrite(uint32_t payload)
{
    uint8_t res = SD_E_OK;
    uint8_t r1;
    Spi_R1Response resp;
    uint32_t readResponseAttempts;
    uint8_t tx[1U + SD_SECTOR_LENGTH + 2U];
    uint8_t dummy[SD_SPI_PRE_CMD_CLOCKS];

    memset(tx, 0, sizeof(tx));
    tx[0] = SD_DEF_START_DATA_MARKER;

    SpiAbs_CsDisable(SPIABS_DEVICE_1);
    SpiAbs_Receive_Spi1_Task0(dummy, sizeof(dummy));
    SpiAbs_CsEnable(SPIABS_DEVICE_1);

    SD_Spi_SendCommand(SD_SPI_CMD56, payload);
    res = SD_Spi_WaitNonFF(&r1, SD_MAX_READ_RESPONSE_ATTEMPTS * 20U);
    if (SD_E_OK != res)
    {
        SpiAbs_CsDisable(SPIABS_DEVICE_1);
        return res;
    }
    res = SD_Spi_Cmd56_CheckR1(r1);
    if (SD_E_OK != res)
    {
        SpiAbs_CsDisable(SPIABS_DEVICE_1);
        return res;
    }

    SpiAbs_Send_Spi1_Task0(tx, sizeof(tx));
    SpiAbs_PollForResponse(SPIABS_DEVICE_1, &resp.byte);

    if ((resp.byte & 0x1FU) != SD_DEF_DATA_ACCEPTED_TOKEN)
    {
        res = SD_E_CMD_NO_DATA_RESP_TOKEN;
    }

    if (SD_E_OK == res)
    {
        readResponseAttempts = 0U;
        do
        {
            SpiAbs_readByte(SPIABS_DEVICE_1, &resp.byte);
        } while ((resp.byte != 0xFFU)
                 && (++readResponseAttempts < SD_MAX_READ_RESPONSE_ATTEMPTS));

        if (readResponseAttempts >= SD_MAX_READ_RESPONSE_ATTEMPTS)
        {
            res = SD_E_CMD_NO_GOING_IDLE;
        }
    }

    SpiAbs_CsDisable(SPIABS_DEVICE_1);
    return res;
}

/**
 * @brief Send CMD56 read variant and receive one 512-byte block.
 * @param payload CMD56 argument.
 * @param dataOut Out: received 512-byte payload.
 * @return SD_E_OK on success, otherwise SD_E_* error code.
 */
static uint8_t SD_Spi_Cmd56_ReadBlock(uint32_t payload, uint8_t *dataOut)
{
    uint8_t res;
    uint8_t r1;
    uint8_t dummy[SD_SPI_PRE_CMD_CLOCKS];
    uint8_t crc[2];
    Spi_R1Response resp;
    uint32_t readAttempts;

    SpiAbs_CsDisable(SPIABS_DEVICE_1);
    SpiAbs_Receive_Spi1_Task0(dummy, sizeof(dummy));
    SpiAbs_CsEnable(SPIABS_DEVICE_1);

    SD_Spi_SendCommand(SD_SPI_CMD56, payload);
    res = SD_Spi_WaitNonFF(&r1, SD_MAX_READ_RESPONSE_ATTEMPTS * 20U);
    if (SD_E_OK != res)
    {
        SpiAbs_CsDisable(SPIABS_DEVICE_1);
        return res;
    }

    res = SD_Spi_Cmd56_CheckR1(r1);
    if (SD_E_OK != res)
    {
        SpiAbs_CsDisable(SPIABS_DEVICE_1);
        return res;
    }

    readAttempts = 0U;
    while (++readAttempts < SD_MAX_READ_RESPONSE_ATTEMPTS)
    {
        SpiAbs_readByte(SPIABS_DEVICE_1, &resp.byte);
        if (resp.byte != 0xFFU)
        {
            break;
        }
    }

    if (resp.byte != SD_SPI_CMD_START_TOKEN)
    {
        SpiAbs_CsDisable(SPIABS_DEVICE_1);
        return SD_E_CMD_NO_START_TOKEN;
    }

    SpiAbs_Receive_Spi1_Task0(dataOut, SD_KINGSTON_HEALTH_BLOCK_LEN);
    SpiAbs_Receive_Spi1_Task0(crc, sizeof(crc));

    SpiAbs_CsDisable(SPIABS_DEVICE_1);
    return SD_E_OK;
}

static uint8_t SD_Spi_WaitBusy(uint32_t maxAttempts)
{
    uint32_t attempts = 0;
    uint8_t resp;

    do {
        SpiAbs_readByte(SPIABS_DEVICE_1, &resp);
        if (resp == 0xFF) break;
        if (attempts >= 500U)
        {
            Sd_Spi_OsTaskDelayHook(2);
        }
    } while (++attempts < maxAttempts);

    return (resp == 0xFF) ? 0U : 1U;
}

uint8_t SD_Spi_SendCommand(uint8_t cmd, uint32_t payload)
{
    uint8_t res = SD_E_OK;

    res = SD_Spi_CreateCommand(cmd, payload, aTxSpiCmd);

    if (SD_E_OK == res)
    {
        res = SpiAbs_Send_Spi1_Task0((uint8_t *)aTxSpiCmd, COUNTOF(aTxSpiCmd));

        if (SPIABS_E_OK != res)
        {
            res = SD_E_SEND;
        }
    }

    return res;
}

uint8_t
SD_Spi_SendCommandPollResponse(uint8_t cmd, uint32_t payload, uint8_t *resp)
{
    // assert chip select
    SpiAbs_CsEnable(SPIABS_DEVICE_1);

    SD_Spi_SendCommand(cmd, payload);
    SpiAbs_PollForResponse(SPIABS_DEVICE_1, resp);

    // deassert chip select
    SpiAbs_CsDisable(SPIABS_DEVICE_1);

    return 0U;
}

uint8_t SD_Spi_PowerUp(void)
{
    SpiAbs_CsDisable(SPIABS_DEVICE_1);

    SpiAbs_Send_Spi1_Task0((uint8_t *)aTxSpiInit, COUNTOF(aTxSpiInit));

    return 0;
}

uint8_t SD_Spi_WaitTillIdle()
{
    uint8_t RetVal  = 1;
    uint8_t counter = 0;
    uint8_t buffer[1];
    const uint8_t RetryCount = 10;

    SpiAbs_CsEnable(SPIABS_DEVICE_1);

    // wait till card is idle
    do
    {
        SpiAbs_Receive_Spi1_Task0(buffer, sizeof(buffer));
    } while (0xFF != buffer[0] && (RetryCount > counter++));

    SpiAbs_CsDisable(SPIABS_DEVICE_1);

    if (0xFF == buffer[0])
    {
        RetVal = 1;
    }
    else
    {
        RetVal = 0;
    }
    return RetVal;
}

uint8_t SD_Spi_GoIdleState(Spi_R1Response *pResponse)
{
    uint8_t i;
    uint8_t res = SD_E_OK;

    if (pResponse == NULL) {
        return SD_E_INV_PARAM;
    }

    pResponse->byte = 0xFF;

    SpiAbs_CsDisable(SPIABS_DEVICE_1);

    for (i = 0U; i < SD_SPI_PRE_CMD_CLOCKS; i++)
    {
        res = SpiAbs_readByte(SPIABS_DEVICE_1, &pResponse->byte); // Ensure idle state

        if (SPIABS_E_OK != res)
        {
            return SD_E_NOT_OK;
        }
    }

    // assert chip select
    SpiAbs_CsEnable(SPIABS_DEVICE_1);

    res = SD_Spi_SendCommand(SD_SPI_CMD0, 0x00000000);
    
    if (SD_E_OK == res)
    {
        res = SpiAbs_PollForResponse(SPIABS_DEVICE_1, &pResponse->byte);

        if (SPIABS_E_OK != res)
        {
            res = SD_E_RESPONSE;
        }
    }

    // deassert chip select
    SpiAbs_CsDisable(SPIABS_DEVICE_1);
    
    return res;
}

// uint8_t SD_Spi_SendWakeUp(Spi_R1Response * pResponse)
// {
// 	SpiAbs_CsEnable(SPIABS_DEVICE_1);

// 	SD_Spi_SendCommand(SD_SPI_CMD1, 0);
// 	SpiAbs_PollForResponse(SPIABS_DEVICE_1, &pResponse->byte);

// 	SpiAbs_CsDisable(SPIABS_DEVICE_1);

// 	return 0U;
// }

uint8_t SD_Spi_SendIfCond(Spi_R1Response *pResponse)
{
    // assert chip select
    SpiAbs_CsEnable(SPIABS_DEVICE_1);

    // send CMD8
    SD_Spi_SendCommand(SD_SPI_CMD8, 0x00000000);
    SpiAbs_PollForResponse(SPIABS_DEVICE_1, &pResponse->byte);

    // deassert chip select
    SpiAbs_CsDisable(SPIABS_DEVICE_1);

    return 0;
}

uint8_t SD_Spi_SendApp(Spi_R1Response *pResponse)
{
    // assert chip select

    SpiAbs_CsEnable(SPIABS_DEVICE_1);

    SD_Spi_SendCommand(SD_SPI_CMD55, 0x00000000); // Precede ACMD41 with CMD55
    SpiAbs_CsDisable(SPIABS_DEVICE_1);

    SpiAbs_CsEnable(SPIABS_DEVICE_1);
    SpiAbs_PollForResponse(SPIABS_DEVICE_1, &pResponse->byte);

    // deassert chip select

    SpiAbs_CsDisable(SPIABS_DEVICE_1);

    return 0;
}

uint8_t SD_Spi_SendOpCond(Spi_R1Response *pResponse)
{
    // assert chip select
    //	SpiAbs_CsDisable(SPIABS_DEVICE_1);

    SpiAbs_CsEnable(SPIABS_DEVICE_1);

    SD_Spi_SendCommand(SD_SPI_ACMD41, 0x40000000); // Precede ACMD41 with CMD55
    SpiAbs_CsDisable(SPIABS_DEVICE_1);

    SpiAbs_CsEnable(SPIABS_DEVICE_1);
    SpiAbs_PollForResponse(SPIABS_DEVICE_1, &pResponse->byte);
    // deassert chip select

    SpiAbs_CsDisable(SPIABS_DEVICE_1);

    return 0;
}

uint8_t SD_Spi_ReadOCR(Spi_R1Response *pResponse)
{
    // assert chip select

    SpiAbs_CsEnable(SPIABS_DEVICE_1);

    SD_Spi_SendCommand(SD_SPI_CMD58, 0x00000000);
    SpiAbs_PollForResponse(SPIABS_DEVICE_1, &pResponse->byte);

    // deassert chip select

    SpiAbs_CsDisable(SPIABS_DEVICE_1);

    return 0;
}

uint8_t SD_Spi_SendCSDRequest(Spi_R1Response *pResponse)
{
    SD_Spi_SendCommandPollResponse(SD_SPI_CMD9, 0x00000000, &pResponse->byte);

    return 0;
}

uint8_t SD_Spi_ReadRes7(uint8_t *pRxBuffer)
{
    SpiAbs_CsEnable(SPIABS_DEVICE_1);

    SpiAbs_Receive_Spi1_Task0((uint8_t *)pRxBuffer, 4);

    SpiAbs_CsEnable(SPIABS_DEVICE_1);

    return 0;
}

/**
  * @brief  Initializes the card and determines its type (e.g., SDSC, SDHC, SDXC).
			Sets up the card for further operations.
  * @param  None
  * @retval None
  */
uint8_t SD_Spi_Initialize(uint8_t CsLine)
{
    uint8_t RetVal;
    uint8_t counter;
    uint32_t OcrByte;
    Spi_R1Response response;
    OCR_Register OcrResponse;
    uint8_t ocr[4];
    uint8_t buffer[4];

    RetVal = 0x00;

    HAL_Delay(100);
    SD_Spi_PowerUp();
    HAL_Delay(300);
    RetVal = SD_Spi_GoIdleState(&response);

    if (SD_E_OK != RetVal)
    {
        return RetVal;
    }

    if (0x01 == response.byte)
    {
        // SD card successfully initialized
        SD_Spi_WaitTillIdle();
        SD_Spi_SendIfCond(&response);

        if ((response.byte & 0x3F) == 0x01) // Ignore bit 6 (Erase Reset)
        {
            SD_Spi_ReadRes7(ocr);
            SD_Spi_WaitTillIdle();

            if (ocr[2] == 0x01 && ocr[3] == 0xAA)
            {
                counter = 0;
                do
                {
                    SD_Spi_SendApp(&response);

                    if (counter > 100)
                    {
                        RetVal = 3;
                    }

                    if (response.byte < 0x02 && 0 == RetVal)
                    {
                    }
                    SD_Spi_SendOpCond(&response);

                    HAL_Delay(10);

                    counter++;
                } while (response.byte != 0x00 && 0 == RetVal
                ); // Wait for idle state to clear

                // TODO: if OpComd is rejected, retry with CMD1
                {
                }

                OcrResponse.cpusb = 0;
                while (0U == RetVal && 0U == OcrResponse.cpusb)
                {
                    HAL_Delay(10);

                    SD_Spi_WaitTillIdle();

                    // Send CMD58 to read OCR and check CCS bit
                    SD_Spi_ReadOCR(&response);

                    if (response.byte == 0x00)
                    {

                        SD_Spi_ReadRes7(buffer);
                        OcrByte = (buffer[0] << 24) | (buffer[1] << 16)
                                  | (buffer[2] << 8) | buffer[3];
                        SD_Spi_Ocr2Bitfield(OcrByte, &OcrResponse);
                        if (IS_CARD_READY(OcrByte))
                        {
                            // card is a ver. 2.0 or later high capacity SD card (SDHC) or extended capacity SD card (SCXC)
                        }
                        else
                        {
                            // card is a ver. 2.0 or later standard capacity SD memory card
                        }
                    }
                }
            }
            else
            {
                RetVal = 2;
            }
        }
    }
    else
    {
        RetVal = 1;
    }

    if (0U == RetVal)
    {
        RetVal = SpiAbs_GoHighSpeed(SPIABS_DEVICE_1);
    }
    else
    {
        RetVal = RES_ERROR;
    }

    return RetVal;
}

uint8_t SD_Spi_ReadCSD(SdCsdRegisterType *csd)
{
    Spi_R1Response resp;
    uint32_t readAttempts;
    uint8_t Crc    = 0;
    uint8_t RetVal = 0;

    SD_Spi_SendCommandPollResponse(SD_SPI_CMD9, 0x00000000, &resp.byte);

    SpiAbs_CsEnable(SPIABS_DEVICE_1);

    if (resp.byte != 0xFF)
    {
        // wait for a response token (timeout = 100ms)
        readAttempts = 0;
        while (++readAttempts < SD_MAX_READ_RESPONSE_ATTEMPTS)
        {
            SpiAbs_PollForResponse(SPIABS_DEVICE_1, &resp.byte);
            if (resp.byte != 0xFF)
            {
                break;
            }
        }

        // if response token is 0xFE
        if (resp.byte == SD_SPI_CMD_START_TOKEN)
        {
            SpiAbs_Receive_Spi1_Task0(SPI_CMD_READ_BUFFER, SD_SPI_CSD_LENGTH);

            // read 16-bit CRC
            SpiAbs_Receive_Spi1_Task0((uint8_t *)&Crc, sizeof(Crc));

            if (0 != Crc)
            {
                RetVal = 0U;
            }
        }
        else
        {
            RetVal = 1U;
        }
    }

    SpiAbs_CsDisable(SPIABS_DEVICE_1);

    if (0U == RetVal)
    {
        SD_Spi_Csd2Bitfield(SPI_CMD_READ_BUFFER, csd);
    }

    return RetVal;
}

/*******************************************************************************
 Read single 512 byte block
 token = 0xFE - Successful read
 token = 0x0X - Data error
 token = 0xFF - Timeout
*******************************************************************************/
uint8_t SD_Spi_readSingleBlock(uint32_t address, Spi_R1Response *pResponse)
{
    uint8_t RetVal;
    uint32_t readAttempts, tokenPollCount;
    uint8_t *readBuffer;
    uint8_t CardStatus, Dummy[SD_SPI_PRE_CMD_CLOCKS];
    uint16_t Crc = 0;
    Spi_R1Response resp;
    uint8_t GotResponse;
    uint8_t Run = 0U;

    (void)Run;

    RetVal     = 0U;
    readBuffer = SPI_CMD_READ_BUFFER;

    memset(readBuffer, 0U, SD_SDHC_SECTOR_SIZE);

    SpiAbs_CsDisable(SPIABS_DEVICE_1);

    SpiAbs_Receive_Spi1_Task0(Dummy, sizeof(Dummy));

    SpiAbs_CsEnable(SPIABS_DEVICE_1);

    SD_Spi_SendCommand(SD_SPI_CMD13, 0); // CMD13 to check card status
    SpiAbs_PollForResponse(SPIABS_DEVICE_1, &pResponse->byte);

    switch (pResponse->byte)
    {
    case 0x00: // card is ready
        SpiAbs_Receive_Spi1_Task0(&CardStatus, sizeof(CardStatus));
        break;
    case 0x01: // idle state
        SpiAbs_Receive_Spi1_Task0(&CardStatus, sizeof(CardStatus));
        RetVal = SD_Spi_Initialize(0U);
        break;
    case 0xFF: // not responding
    default:
        SpiAbs_CsDisable(SPIABS_DEVICE_1);
        // TODO: power on and off (somehow the SD card gets unresponsive after a while)
        RetVal = SD_Spi_Initialize(0U);
        break;
    }

    if (0U != RetVal)
    {
        return RetVal;
    }

    SpiAbs_CsDisable(SPIABS_DEVICE_1);

    SpiAbs_Receive_Spi1_Task0(Dummy, sizeof(Dummy));

    SpiAbs_CsEnable(SPIABS_DEVICE_1);

    readAttempts = 0;
    do
    {
        SD_Spi_SendCommand(SD_SPI_CMD17, address);
        SpiAbs_PollForResponse(SPIABS_DEVICE_1, &pResponse->byte);

        if (pResponse->byte != 0xFF)
        {
            // wait for a response token (timeout = 100ms)
            tokenPollCount = 0;
            GotResponse    = 0U;
            while (++tokenPollCount < SD_MAX_READ_RESPONSE_ATTEMPTS)
            {
                SpiAbs_PollForResponse(SPIABS_DEVICE_1, &pResponse->byte);
                if (pResponse->byte != 0xFF)
                {
                    GotResponse = 1U;
                    break;
                }
            }
        }
        else
        {
            RetVal = 2U;
            break;
        }
    } while ((0U == GotResponse) && (3 > readAttempts++));

    if (pResponse->byte == SD_SPI_CMD_START_TOKEN) // if response token is 0xFE
    {
    }
    else if (pResponse->byte
             != SD_SPI_CMD_START_TOKEN) // if response token is 0xFE
    {
        SpiAbs_PollForResponse(SPIABS_DEVICE_1, &resp.byte);

        if (SD_SPI_CMD_START_TOKEN != resp.byte)
        {
            RetVal = 3U;
        }
    }
    else
    {
        RetVal = 1U;
    }

    if (0U == RetVal)
    {
        for (uint32_t i = 0; i < SD_SECTOR_LENGTH;
             i += SD_SPI_SECTOR_CHUNK_SIZE)
        {
            if (SD_SECTOR_LENGTH - i > SD_SPI_SECTOR_CHUNK_SIZE)
            {
                SpiAbs_Receive_Spi1_Task0(
                    &readBuffer[i],
                    SD_SPI_SECTOR_CHUNK_SIZE
                );
            }
            else
            {
                SpiAbs_Receive_Spi1_Task0(&readBuffer[i], SD_SECTOR_LENGTH - i);
            }
        }

        // read 16-bit CRC
        SpiAbs_Receive_Spi1_Task0((uint8_t *)&Crc, sizeof(Crc));

        if (0 != Crc)
        {
            RetVal = 0U;
        }
    }

    return RetVal;
}

uint8_t
SD_Spi_readMultiBlock(uint32_t address, uint8_t *const buff, uint32_t cnt)
{
    uint8_t res;
    uint32_t tokenPollCount;
    uint16_t Crc = 0U;
    Spi_R1Response resp;
    uint8_t Dummy[SD_SPI_PRE_CMD_CLOCKS];
    static uint8_t Tmp[SD_SECTOR_LENGTH + sizeof(Crc)];
    bool stopTransmission;

    res              = 0U;
    stopTransmission = false;

    SpiAbs_CsDisable(SPIABS_DEVICE_1);

    SpiAbs_Receive_Spi1_Task0(Dummy, sizeof(Dummy));

    SpiAbs_CsEnable(SPIABS_DEVICE_1);

    SD_Spi_SendCommand(SD_SPI_CMD18, address);

    SpiAbs_PollForResponse(SPIABS_DEVICE_1, &resp.byte);

    if (resp.byte == 0xFF)
    {
        res = 2U;
    }
    else
    {
        stopTransmission = true;
    }

    if (0 == res)
    {
        for (uint32_t j = 0; j < cnt; j++)
        {
            // wait for a response token (timeout = 100ms)
            tokenPollCount = 0;
            while (++tokenPollCount < SD_MAX_READ_RESPONSE_ATTEMPTS)
            {
                SpiAbs_PollForResponse(SPIABS_DEVICE_1, &resp.byte);
                if (resp.byte == SD_DEF_START_DATA_MARKER)
                {
                    break;
                }
            }

            if (resp.byte != SD_DEF_START_DATA_MARKER)
            {
                res = SD_E_CMD_NO_START_TOKEN;
            }
            else
            {
                res = SpiAbs_Receive_Spi1_Task0(Tmp, sizeof(Tmp));

                memcpy(&buff[j * SD_SECTOR_LENGTH], Tmp, SD_SECTOR_LENGTH);
                memcpy(&Crc, &Tmp[SD_SECTOR_LENGTH], sizeof(Crc));
            }

            if (0 != res)
            {
                break;
            }
        }

        if (stopTransmission)
        {
            SD_Spi_SendCommand(SD_SPI_CMD12, address);
        }
    }

    if (stopTransmission)
    {
        if ((SpiAbs_PollForIdle(
                 SPIABS_DEVICE_1,
                 &resp.byte,
                 SD_MAX_READ_RESPONSE_ATTEMPTS
             ) != 0)
            && (0 == res))
        {
            res = SD_E_CMD_NO_GOING_IDLE;
        }
    }

    if (0 != res)
    {
        ErrorContext.line = __LINE__;
        ErrorContext.code = res;
        snprintf(
            ErrorContext.function,
            ERRORCONTEXT_FUNCTION_NAME_LENGTH,
            "%s",
            "SD_Spi_readMultiBlock"
        );

        Sd_Spi_ErrorHandlerHook(&ErrorContext);
    }

    SpiAbs_CsDisable(SPIABS_DEVICE_1);

    return res;
}

DRESULT SD_Spi_hotReset(void)
{
    uint8_t res;
    uint8_t WaitRes = 0;
    Spi_R1Response resp;
    uint8_t dummy[8];

    HotResetCallCount++;

    memset(dummy, SD_SPI_CMD_DUMMY_DATA, sizeof(dummy));

    SpiAbs_CsDisable(SPIABS_DEVICE_1);
    SpiAbs_Receive_Spi1_Task0(dummy, sizeof(dummy));
    SpiAbs_CsEnable(SPIABS_DEVICE_1);

    SD_Spi_SendCommand(SD_SPI_CMD12, 0U);

    SpiAbs_readByte(SPIABS_DEVICE_1, &resp.byte); // discard stuff byte
    SpiAbs_PollForResponse(SPIABS_DEVICE_1, &resp.byte);

    if (0xFF == resp.byte)
    {
        SpiAbs_CsDisable(SPIABS_DEVICE_1);
        return RES_NOTRDY;
    }

    WaitRes = SD_Spi_WaitBusy(SD_MAX_READ_RESPONSE_ATTEMPTS);

    SpiAbs_CsDisable(SPIABS_DEVICE_1);

    if (0 != WaitRes)
    {
        res = RES_NOTRDY;
    }
    else
    {
        res = RES_OK;
    }

    return res;
}

/**
  * @brief Polls the slave untill 0xFF is sent from the slave and returns 
  *        'bytes' number of bytes after the first 0xFF to the caller.
  * @param buffer: pointer to a buffer with size 'bytes'
  * @param min_bytes: number of bytes read with each poll (max 10).
  * @param bytes: number of bytes kept after first 
  *               occurence of 0xFF sent from slave.
  */
static uint8_t __attribute__((unused))
SpiAbs_waitTillNotBusy(uint8_t *buffer, uint16_t min_bytes, uint16_t bytes)
{
    uint32_t readResponseAttempts;
    uint8_t Buf[64U];
    uint8_t ReadyTokenReceived;
    uint32_t BytesRemaining;
    uint32_t index;

    if (min_bytes > sizeof(Buf))
    {
        return 1;
    }

    if ((bytes > 0) && (!buffer))
    {
        return 1;
    }

    ReadyTokenReceived   = 0;
    readResponseAttempts = 0;

    do
    { //Waiting for the end of the state BUSY
        if (0 == SpiAbs_Receive_Spi1_Task0(Buf, min_bytes))
        {
            for (index = 0; index < min_bytes; index++)
            {
                if (0xFF == Buf[index])
                {
                    ReadyTokenReceived = 1;
                    BytesRemaining     = min_bytes - (index + 1);
                    break;
                }
            }
        }
    } while ((ReadyTokenReceived == 0) && (++readResponseAttempts < 3U));

    if (0 == ReadyTokenReceived)
    {
        return 1;
    }
    else if (0 == bytes)
    {
    }
    else if (0 == BytesRemaining)
    {
        SpiAbs_Receive_Spi1_Task0(buffer, bytes);
    }
    else if (BytesRemaining >= bytes)
    {
        memcpy(buffer, &Buf[index + 1], bytes);
    }
    else if (BytesRemaining < bytes)
    {
        memcpy(buffer, &Buf[index + 1], BytesRemaining);
        SpiAbs_Receive_Spi1_Task0(
            &buffer[BytesRemaining],
            bytes - BytesRemaining
        );
    }

    return (uint8_t)(1 != ReadyTokenReceived);
}

uint8_t
SD_Spi_writeMultiBlock(uint32_t address, uint8_t const *buff, uint32_t cnt)
{
    uint8_t res;
    uint16_t Crc = 0U;
    Spi_R1Response resp;
    bool TransmissionStarted     = false;
    const uint8_t StartDataToken = SD_DEF_MULTI_BLOCK_START_TOKEN;
    const uint8_t StopDataToken  = SD_DEF_MULTI_BLOCK_STOP_TOKEN;
    uint8_t Tmp[sizeof(StartDataToken) + SD_SECTOR_LENGTH + sizeof(Crc)];

    res = 0U;

    SpiAbs_CsEnable(SPIABS_DEVICE_1);

    // // Add pre-erase notification
    // SD_Spi_SendCommand(SD_SPI_CMD55, 0);  // APP_CMD
    // SpiAbs_PollForResponse(SPIABS_DEVICE_1, &resp.byte);

    // SD_Spi_SendCommand(SD_SPI_ACMD23, cnt);  // SET_WR_BLK_ERASE_COUNT
    // SpiAbs_PollForResponse(SPIABS_DEVICE_1, &resp.byte);

    SD_Spi_SendCommand(SD_SPI_CMD25, address);

    SpiAbs_PollForResponse(SPIABS_DEVICE_1, &resp.byte);

    if (resp.byte == 0xFF)
    {
        res = SD_E_CMD_NO_R1;
    }

    if (0 == res)
    {
        TransmissionStarted = true;

        for (uint32_t j = 0; j < cnt; j++)
        {
            memcpy(&Tmp[0], &StartDataToken, sizeof(StartDataToken));
            memcpy(
                &Tmp[sizeof(StartDataToken)],
                &buff[j * SD_SECTOR_LENGTH],
                SD_SECTOR_LENGTH
            );
            memcpy(
                &Tmp[sizeof(StartDataToken) + SD_SECTOR_LENGTH],
                &Crc,
                sizeof(Crc)
            );

            SpiAbs_Send_Spi1_Task0(Tmp, sizeof(Tmp));

            SpiAbs_PollForResponse(SPIABS_DEVICE_1, &resp.byte);

            if ((resp.byte & 0x1F) == SD_DEF_DATA_ACCEPTED_TOKEN)
            {
            }
            else if ((resp.byte & 0x1F) == SD_DEF_CRC_ERROR_TOKEN)
            {
                res = SD_E_CMD_NO_DATA_RESP_TOKEN;
            }
            else if ((resp.byte & 0x1F) == SD_DEF_WRITE_ERROR_TOKEN)
            {
                res = SD_E_CMD_NO_DATA_RESP_TOKEN;
            }

            if (0 == res)
            {
                uint8_t WaitRes = 0;

                WaitRes = SD_Spi_WaitBusy(SD_MAX_READ_RESPONSE_ATTEMPTS);
                
                if (0 != WaitRes)
                {
                    res = SD_E_CMD_NO_GOING_IDLE;
                }
            }

            if (0 != res)
            {
                break;
            }
        }
    }

    if (TransmissionStarted)
    {
        SpiAbs_writByte(SPIABS_DEVICE_1, &StopDataToken);

        memset(&address, 0, sizeof(address)); // clear for dummy usage
        SD_Spi_SendCommand(SD_SPI_CMD12, address); // Send CMD12

        SpiAbs_PollForResponse(SPIABS_DEVICE_1, &resp.byte);
        if (resp.byte == 0xFF)
        {
            res = SD_E_CMD_NO_STOP_TRANSMISSION_RESPONSE;
        }

        if (0 == res)
        {
            uint8_t WaitRes = 0;

            WaitRes = SD_Spi_WaitBusy(SD_MAX_READ_RESPONSE_ATTEMPTS * 20U);
            
            if (0 != WaitRes)
            {
                res = SD_E_CMD_NO_GOING_IDLE;
            }
        }
    }

    if (0 != res)
    {
        ErrorContext.code = res;
        ErrorContext.line = __LINE__;
        snprintf(
            ErrorContext.function,
            ERRORCONTEXT_FUNCTION_NAME_LENGTH,
            "%s",
            "SD_Spi_writeMultiBlock"
        );

        Sd_Spi_ErrorHandlerHook(&ErrorContext);
    }

    SpiAbs_CsDisable(SPIABS_DEVICE_1);

    return res;
}

uint8_t SD_Spi_writeBlock(uint32_t address, uint8_t const *buff)
{
    uint8_t RetVal;
    uint16_t Crc = 0U;
    Spi_R1Response resp;
    const uint8_t StartDataToken = SD_DEF_START_DATA_MARKER;

    RetVal = 0U;

    SpiAbs_CsEnable(SPIABS_DEVICE_1);

    SD_Spi_SendCommand(SD_SPI_CMD24, address);
    SpiAbs_PollForResponse(SPIABS_DEVICE_1, &resp.byte);

    if (resp.byte == 0xFF)
    {
        RetVal = SD_E_CMD_NO_R1;
    }

    if (0 == RetVal)
    {
        // Send 0xFE start token
        SpiAbs_writByte(SPIABS_DEVICE_1, &StartDataToken);

        for (uint32_t i = 0; i < SD_SECTOR_LENGTH;
             i += SD_SPI_SECTOR_CHUNK_SIZE)
        {
            if (SD_SECTOR_LENGTH - i > SD_SPI_SECTOR_CHUNK_SIZE)
            {
                SpiAbs_Send_Spi1_Task0(&buff[i], SD_SPI_SECTOR_CHUNK_SIZE);
            }
            else
            {
                SpiAbs_Send_Spi1_Task0(&buff[i], SD_SECTOR_LENGTH - i);
            }
        }

        SpiAbs_Send_Spi1_Task0((uint8_t *)&Crc, sizeof(Crc));

        SpiAbs_PollForResponse(SPIABS_DEVICE_1, &resp.byte);
        if (SD_DEF_DATA_RESP_TOKEN != (0x0F & resp.byte))
        {
            RetVal = SD_E_CMD_NO_DATA_RESP_TOKEN;
        }
    }

    if (0 == RetVal)
    {
        uint8_t WaitRes = 0;

        WaitRes = SD_Spi_WaitBusy(SD_MAX_READ_RESPONSE_ATTEMPTS);

        if (0 != WaitRes)
        {
            RetVal = SD_E_CMD_NO_GOING_IDLE;
        }
    }

    SpiAbs_CsDisable(SPIABS_DEVICE_1);

    return RetVal;
}

uint8_t SD_Spi_ReadKingstonHealth(SD_KingstonHealthType *health)
{
    uint8_t res = SD_E_OK;
    uint8_t data[SD_KINGSTON_HEALTH_BLOCK_LEN];
    uint8_t r1;

    if (NULL == health)
    {
        return SD_E_NOT_OK;
    }

    res = SD_Spi_Cmd56_PollR1(1U, &r1);
    if (SD_E_OK == res)
    {
        HAL_Delay(1000U);
        res = SD_Spi_Cmd56_ReadBlock(0x0U, data);
    }
    else
    {
        res = SD_Spi_Cmd56_PreloadWrite(0x10U);
        if (SD_E_OK == res)
        {
            HAL_Delay(1000U);
            res = SD_Spi_Cmd56_ReadBlock(0x21U, data);
        }
    }

    if (SD_E_OK != res)
    {
        if ((SD_E_CMD_NO_R1 == res) || (SD_E_HEALTH_NOT_SUPPORTED == res))
        {
            return SD_E_HEALTH_NOT_SUPPORTED;
        }

        ErrorContext.code = res;
        ErrorContext.line = __LINE__;
        snprintf(
            ErrorContext.function,
            ERRORCONTEXT_FUNCTION_NAME_LENGTH,
            "%s",
            "SD_Spi_ReadKingstonHealth"
        );
        Sd_Spi_ErrorHandlerHook(&ErrorContext);
        return res;
    }

    if ((false == SD_Spi_IsDataValid(data, sizeof(data)))
        || (false == SD_Spi_IsKnownKingstonHealthSignature(data)))
    {
        return SD_E_HEALTH_NOT_SUPPORTED;
    }

    memset(health, 0, sizeof(*health));

    health->spareBlockCount      = (uint8_t)SD_Spi_ReadBe16(data, 16U);
    health->initialBadBlockCount = SD_Spi_SumRangeU8(data, 32U, 64U);
    health->goodBlockRatePercent = (float)SD_Spi_ReadBe16(data, 64U) / 100.0f;
    health->totalEraseCount      = SD_Spi_ReadBe32(data, 80U);
    health->enduranceRemainLifePercent =
        (float)SD_Spi_ReadBe16(data, 96U) / 100.0f;
    health->avgEraseCount = ((uint32_t)SD_Spi_ReadBe16(data, 104U) << 16U)
                            + SD_Spi_ReadBe16(data, 98U);
    health->minEraseCount = ((uint32_t)SD_Spi_ReadBe16(data, 106U) << 16U)
                            + SD_Spi_ReadBe16(data, 100U);
    health->maxEraseCount = ((uint32_t)SD_Spi_ReadBe16(data, 108U) << 16U)
                            + SD_Spi_ReadBe16(data, 102U);
    health->powerUpCount          = SD_Spi_ReadBe32(data, 112U);
    health->abnormalPowerOffCount = SD_Spi_ReadBe16(data, 128U);
    health->laterBadBlockCount    = SD_Spi_SumRangeU8(data, 184U, 216U);

    return SD_E_OK;
}

uint8_t SD_Spi_GetReadBytes(uint8_t *buff)
{
    memcpy(buff, SPI_CMD_READ_BUFFER, SD_SDHC_SECTOR_SIZE);
    return 0U;
}

__attribute__((weak))
uint8_t Sd_Spi_OsTaskDelayHook(uint32_t delay)
{
    (void) delay;

    return 0;
}
