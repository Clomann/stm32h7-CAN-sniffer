
#include <stdint.h>

/* Shim functions that need to be implemented by the caller */
uint8_t FsCustom_GetCanLogHeadIndex(uint32_t *index);
uint8_t FsCustom_GetCanLogTailIndex(uint32_t *index);
uint8_t FsCustom_GetCanLogCapacity(uint32_t *capacity);

uint8_t FsCustom_IsTracerRunning(uint8_t *running);
