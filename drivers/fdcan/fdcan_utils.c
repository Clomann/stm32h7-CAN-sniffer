#include "fdcan_utils.h"

/* ============================= */

#define RES_OK      0
#define RES_NOT_OK  1


/* Register Field Macros - NBTP/DBTP formats */
#define FDCAN_NBTP_NBRP_Pos   (16U)
#define FDCAN_NBTP_NBRP_Msk   (0x1FFUL << FDCAN_NBTP_NBRP_Pos)
#define FDCAN_NBTP_NSJW_Pos   (24U)
#define FDCAN_NBTP_NSJW_Msk   (0x7FUL << FDCAN_NBTP_NSJW_Pos)
#define FDCAN_NBTP_NTSEG1_Pos (8U)
#define FDCAN_NBTP_NTSEG1_Msk (0xFFUL << FDCAN_NBTP_NTSEG1_Pos)
#define FDCAN_NBTP_NTSEG2_Pos (0U)
#define FDCAN_NBTP_NTSEG2_Msk (0x7FUL << FDCAN_NBTP_NTSEG2_Pos)

#define FDCAN_DBTP_DBRP_Pos   (16U)
#define FDCAN_DBTP_DBRP_Msk   (0x1FUL << FDCAN_DBTP_DBRP_Pos)
#define FDCAN_DBTP_DSJW_Pos   (8U)
#define FDCAN_DBTP_DSJW_Msk   (0xFUL << FDCAN_DBTP_DSJW_Pos)
#define FDCAN_DBTP_DTSEG1_Pos (4U)
#define FDCAN_DBTP_DTSEG1_Msk (0xFUL << FDCAN_DBTP_DTSEG1_Pos)
#define FDCAN_DBTP_DTSEG2_Pos (0U)
#define FDCAN_DBTP_DTSEG2_Msk (0xFUL << FDCAN_DBTP_DTSEG2_Pos)

static uint32_t CANFD_MergeRegisterFromArray(const uint8_t reg_value[4])
{
    return ((uint32_t)reg_value[3] << 24) |
           ((uint32_t)reg_value[2] << 16) |
           ((uint32_t)reg_value[1] << 8) |
           ((uint32_t)reg_value[0] << 0);
}

uint16_t CANFD_GetPrescaler(const uint8_t reg_value[4], bool is_data_phase)
{
    uint32_t reg = CANFD_MergeRegisterFromArray(reg_value);
    if (is_data_phase)
        return ((reg & FDCAN_DBTP_DBRP_Msk) >> FDCAN_DBTP_DBRP_Pos) + 1;
    else
        return ((reg & FDCAN_NBTP_NBRP_Msk) >> FDCAN_NBTP_NBRP_Pos) + 1;
}

uint8_t CANFD_GetSeg1(const uint8_t reg_value[4], bool is_data_phase)
{
    uint32_t reg = CANFD_MergeRegisterFromArray(reg_value);
    if (is_data_phase)
        return ((reg & FDCAN_DBTP_DTSEG1_Msk) >> FDCAN_DBTP_DTSEG1_Pos) + 1;
    else
        return ((reg & FDCAN_NBTP_NTSEG1_Msk) >> FDCAN_NBTP_NTSEG1_Pos) + 1;
}

uint8_t CANFD_GetSeg2(const uint8_t reg_value[4], bool is_data_phase)
{
    uint32_t reg = CANFD_MergeRegisterFromArray(reg_value);
    if (is_data_phase)
        return ((reg & FDCAN_DBTP_DTSEG2_Msk) >> FDCAN_DBTP_DTSEG2_Pos) + 1;
    else
        return ((reg & FDCAN_NBTP_NTSEG2_Msk) >> FDCAN_NBTP_NTSEG2_Pos) + 1;
}

uint8_t CANFD_GetSJW(const uint8_t reg_value[4], bool is_data_phase)
{
    uint32_t reg = CANFD_MergeRegisterFromArray(reg_value);
    if (is_data_phase)
        return ((reg & FDCAN_DBTP_DSJW_Msk) >> FDCAN_DBTP_DSJW_Pos) + 1;
    else
        return ((reg & FDCAN_NBTP_NSJW_Msk) >> FDCAN_NBTP_NSJW_Pos) + 1;
}

/* ============================= */

uint8_t CANFD_CalculateBitTimingRegister(uint32_t clock_hz, uint32_t bitrate, uint32_t sample_point, bool is_data_phase, uint8_t reg_value_out[4])
{
    uint32_t MaxTq;
    #define SAMPLE_POINT_FACTOR 10000

    if (!reg_value_out || clock_hz == 0 || bitrate == 0) {
        return false;
    }

    MaxTq = clock_hz / bitrate;
    
    if (MaxTq > 512U)
    {
        MaxTq = 512U;
    }

    for (uint8_t tq_num = MaxTq; tq_num >= 8; tq_num--) {
        uint32_t total_tq_freq = bitrate * tq_num;
        uint32_t prescaler = clock_hz / total_tq_freq;

        if ((tq_num * sample_point) % SAMPLE_POINT_FACTOR != 0) {
            continue;
        }

        if (prescaler == 0 || (is_data_phase && prescaler > 32) || (!is_data_phase && prescaler > 512)) {
            continue;
        }

        uint32_t actual_bitrate = clock_hz / (prescaler * tq_num);
        if (actual_bitrate != bitrate) {
            continue;
        }

        uint8_t seg1 = (tq_num * sample_point) / SAMPLE_POINT_FACTOR - 1;
        uint8_t seg2 = tq_num - seg1 - 1;
        uint8_t sjw = 1; // <<< FORCE SJW = 1 always

        if (seg1 > 0 && seg2 > 0) {
            uint32_t reg_value = 0;

            if (is_data_phase) {
                reg_value =
                    ((prescaler - 1) << FDCAN_DBTP_DBRP_Pos) |
                    ((sjw - 1) << FDCAN_DBTP_DSJW_Pos) |
                    ((seg1 - 1) << FDCAN_DBTP_DTSEG1_Pos) |
                    ((seg2 - 1) << FDCAN_DBTP_DTSEG2_Pos);
            } else {
                reg_value =
                    ((prescaler - 1) << FDCAN_NBTP_NBRP_Pos) |
                    ((sjw - 1) << FDCAN_NBTP_NSJW_Pos) |
                    ((seg1 - 1) << FDCAN_NBTP_NTSEG1_Pos) |
                    ((seg2 - 1) << FDCAN_NBTP_NTSEG2_Pos);
            }

            // Output 4 bytes little-endian
            reg_value_out[0] = (uint8_t)(reg_value >> 0);
            reg_value_out[1] = (uint8_t)(reg_value >> 8);
            reg_value_out[2] = (uint8_t)(reg_value >> 16);
            reg_value_out[3] = (uint8_t)(reg_value >> 24);

            return RES_OK;
        }
    }

    return RES_NOT_OK;
}
