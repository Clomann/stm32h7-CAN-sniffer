
#pragma once

#include <stdint.h>

#include "http_cgi_ssi.h"
#include "FileHandler.h"

typedef struct {
    /*!< indicates that settings were updated and 
       are ready to be stored */
    uint8_t updated; 
    /*!< Baudrate in bit/s (e.g. 250000 for 250 kbit/s) */
    uint32_t baudrate;
    /*!< CAN operatig mode (1: normal, 2: listen only) */
    uint8_t mode; 
    uint8_t ip[4U];
} AppConfigType;

int SettingsHandler_ParseConfig(char *buff, uint32_t len, AppConfigType *cfg);
int FatFS_SD_LoadConfig(FatFsDeviceType *dev, char *data, uint32_t *len);
uint8_t SettingsHandler_Init(AppConfigType *cfg);
uint8_t SettingsHandler_Poll(FatFsDeviceType *dev, AppConfigType *cfg);