#pragma once

#include <stdint.h>

typedef struct {
    uint32_t baudrate;
    uint8_t ip[4U];
} AppConfigType;

int FileHandler_LoadConfig(char *buff, uint32_t len, AppConfigType *cfg);