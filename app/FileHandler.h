#pragma once

#include <stdint.h>

typedef struct {
    uint32_t baudrate;
    /* CAN operatig mode (1: normal, 2: listen only)*/
    uint8_t mode; 
    uint8_t ip[4U];
} AppConfigType;

int FileHandler_ParseConfig(char *buff, uint32_t len, AppConfigType *cfg);