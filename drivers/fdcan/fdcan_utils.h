#include <stdint.h>
#include <stdbool.h>

uint16_t CANFD_GetPrescaler(const uint8_t reg_value[4], bool is_data_phase);
uint8_t CANFD_GetSeg1(const uint8_t reg_value[4], bool is_data_phase);
uint8_t CANFD_GetSeg2(const uint8_t reg_value[4], bool is_data_phase);
uint8_t CANFD_GetSJW(const uint8_t reg_value[4], bool is_data_phase);

/**
 * @brief Auto-calculate CAN bit timing and output as 4 bytes (LSB first)
 * @param clock_hz Peripheral clock frequency (Hz)
 * @param bitrate Target CAN bitrate (bps)
 * @param is_data_phase true if calculating DBTP, false for NBTP
 * @param reg_value_out 4-byte array to hold result (little endian order)
 * @return true if successful, false if no valid timing found
 */
uint8_t CANFD_CalculateBitTimingRegister(uint32_t clock_hz, uint32_t bitrate, bool is_data_phase, uint8_t reg_value_out[4]);