#include "spi_port_freertos.h"

#include "Spi_Cmds.h"
#include "SpiAbs.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#ifndef MAX_SPI_INSTANCES          /* tune to your MCU */
  #define MAX_SPI_INSTANCES   3
#endif

typedef struct {
    SPI_HandleTypeDef *hspi;
    SemaphoreHandle_t  xSem;
} SpiSemEntry_t;

static SpiSemEntry_t xSemTable[MAX_SPI_INSTANCES] = { 0 };
static StaticSemaphore_t xSemBuffers[MAX_SPI_INSTANCES];

uint8_t spi_port_freertos_init(void)
{
    void *pSpiHandle1;

    pSpiHandle1 = SpiAbs_GetHandle_Spi1();
    Spi_NotifyRegister((SPI_HandleTypeDef*)pSpiHandle1);
    
    return SpiAbs_Init_Spi1();
}


/* Create a semaphore for this SPI handle and remember the pair.        */
void Spi_NotifyRegister(void *hspi)
{
    SPI_HandleTypeDef * handle = hspi;
    
    if (NULL == handle)
    {
        Spi_ErrorHandler();
    }
    
    for (int i = 0; i < MAX_SPI_INSTANCES; ++i) {
        if (xSemTable[i].hspi == NULL) {
            xSemTable[i].hspi = handle;
            xSemTable[i].xSem =
                xSemaphoreCreateBinaryStatic(&xSemBuffers[i]);
            configASSERT(xSemTable[i].xSem);
            return;
        }
    }
    /* Ran out of slots – stop here so the bug is obvious.               */
    configASSERT(!"MAX_SPI_INSTANCES too small");
}

#if SPI_PORT_USE_HOOKS

/* Internal utility – fetch the semaphore that belongs to *hspi.        */
static inline SemaphoreHandle_t prvGetSem(SPI_HandleTypeDef *hspi)
{
    for (int i = 0; i < MAX_SPI_INSTANCES; ++i)
        if (xSemTable[i].hspi == hspi)
            return xSemTable[i].xSem;
    return NULL;                       /* not registered → configuration bug */
}

/* ---------- task-side: called after the DMA transfer is started ------ */
uint8_t Spi_NotifyTransferIssued(SPI_HandleTypeDef *hspi)
{
    SemaphoreHandle_t xSem = prvGetSem(hspi);
    configASSERT(xSem);                        /* forgot to register?   */

    /* Flush any old token (e.g. previous error path).                   */
    (void)xSemaphoreTake(xSem, 0);

    /* Block until ISR signals completion or error.                      */
    if (xSemaphoreTake(xSem, portMAX_DELAY) != pdTRUE)
        return 1;                            /* timeout (shouldn’t happen) */

    return 0;
}

/* ---------- ISR side: successful completion -------------------------- */
uint8_t Spi_NotifyTransferComplete(SPI_HandleTypeDef *hspi)
{
    BaseType_t xHigherPrioTaskWoken = pdFALSE;
    SemaphoreHandle_t xSem = prvGetSem(hspi);

    if (xSem) {
        xSemaphoreGiveFromISR(xSem, &xHigherPrioTaskWoken);
    }
    portYIELD_FROM_ISR(xHigherPrioTaskWoken);
    return 0;
}

/* ---------- ISR side: error path ------------------------------------- */
uint8_t Spi_NotifyTransferError(SPI_HandleTypeDef *hspi)
{
    BaseType_t xHigherPrioTaskWoken = pdFALSE;
    SemaphoreHandle_t xSem = prvGetSem(hspi);

    if (xSem) {
        xSemaphoreGiveFromISR(xSem, &xHigherPrioTaskWoken);
    }
    portYIELD_FROM_ISR(xHigherPrioTaskWoken);
    return 0;
}

#endif
