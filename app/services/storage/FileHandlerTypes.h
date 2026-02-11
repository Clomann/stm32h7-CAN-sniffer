#pragma once

#include <stdint.h>
#include <string.h>

#include "ff.h"

typedef struct
{
    uint32_t readTargetSize;
    FIL file;
    uint32_t fflags;
    uint32_t writeIndex;
} FatFsDeviceType;

typedef struct
{
    DIR dir;
    FILINFO fno;
    const char *prefix;
    size_t prefixLen;
} FatFS_FileIterator;
