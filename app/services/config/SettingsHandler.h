
#pragma once

#include <stdint.h>

#include "FileHandler.h"

#define SETTINGSHANDLER_PARTITION_NO FILEHANDLER_PARTITION_NO

typedef enum
{
    SETTINGS_OK = 0,
    SETTINGS_NOT_OK,
} SettingsHandlerErrorType;

typedef struct
{
    /*!< Baudrate in bit/s (e.g. 250000 for 250 kbit/s) */
    uint32_t baudrate;
    /*!< CAN operatig mode (1: normal, 2: listen only) */
    uint8_t mode;
} AppFcdanConfigType;

typedef struct
{
    /*!< indicates that settings were updated and 
       are ready to be stored */
    uint8_t updated;
    AppFcdanConfigType can1;
    AppFcdanConfigType can2;
    uint8_t ip[4U];
} AppConfigType;

int SettingsHandler_ParseConfig(char *buff, uint32_t len, AppConfigType *cfg);
uint8_t SettingsHandler_Init(AppConfigType *cfg);
uint8_t SettingsHandler_Poll(AppConfigType *cfg);
int SettingsHandler_CreateJsonString(
    AppConfigType *config,
    char *buffer,
    uint32_t maxLength,
    uint32_t *length
);

/* Shim functions needed to be implemented by the caller */
void SettingsHandler_ApplyRequestCallback();
