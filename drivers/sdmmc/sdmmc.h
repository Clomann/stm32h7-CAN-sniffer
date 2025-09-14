#pragma once

#include <stdint.h>

#define SDMMC_E_OK      0U
#define SDMMC_E_NOT_OK      1U

typedef uint8_t SdmmcErrorType;

void HAL_MMC_MspInit(MMC_HandleTypeDef *hmmc);