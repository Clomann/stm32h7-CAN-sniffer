
#pragma once

#include "http_cgi_ssi.h"
#include "FileHandler.h"

typedef struct {
    uint32_t baudrate;
    /* CAN operatig mode (1: normal, 2: listen only)*/
    uint8_t mode; 
    uint8_t ip[4U];
} AppConfigType;

int SettingsHandler_ParseConfig(char *buff, uint32_t len, AppConfigType *cfg);
int FatFS_SD_LoadConfig(FatFsDeviceType *dev, char *data, uint32_t *len);
uint8_t SettingsHandler_Init(AppConfigType *cfg);
