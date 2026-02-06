
#include <stdint.h>

/* Shim functions that need to be implemented by the caller */
uint8_t FsCustom_GetCanLogHeadIndex(uint32_t *index);
uint8_t FsCustom_GetCanLogTailIndex(uint32_t *index);
uint8_t FsCustom_GetCanLogCapacity(uint32_t *capacity);
uint8_t FsCustom_GetBusloadCan1(float *busload);
uint8_t FsCustom_GetBusloadCan2(float *busload);
uint8_t FsCustom_GetRb1BytesHighWater(uint32_t *bytes);
uint8_t FsCustom_GetCanAbsRxHighWaterCan1(uint32_t *frames);
uint8_t FsCustom_GetCanAbsRxHighWaterCan2(uint32_t *frames);
uint8_t FsCustom_GetCanAbsRxCapacity(uint32_t *frames);
uint8_t FsCustom_GetFdcanMsgPortHighWater(uint32_t *bytes);
uint8_t FsCustom_GetFdcanMsgPortCapacity(uint32_t *bytes);
uint8_t FsCustom_GetCanLogFrameCount(uint64_t *count);
uint8_t FsCustom_GetPreallocErrorFlag(uint8_t *flag);

uint8_t FsCustom_IsTracerRunning(uint8_t *running);
_Bool FsCustom_IsAnyFrameLostFlag(void);
