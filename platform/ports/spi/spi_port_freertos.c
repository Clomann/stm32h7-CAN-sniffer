#include "spi_port_freertos.h"

#include "Spi_Cmds.h"
#include "SpiAbs.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#ifndef MAX_SPI_INSTANCES /* tune to your MCU */
#define MAX_SPI_INSTANCES 3
#endif

typedef struct
{
    SPI_HandleTypeDef *hspi;
    SemaphoreHandle_t xSem;
} SpiSemEntry_t;

static volatile TaskHandle_t *pCanBridgeTaskHdl;
static volatile SpiSemEntry_t xSemTable[MAX_SPI_INSTANCES] = {0};
static volatile StaticSemaphore_t xSemBuffers[MAX_SPI_INSTANCES];

static void spi_port_freertos_init_ll_mutex(void);

uint8_t spi_port_freertos_init(void *handle)
{
    uint8_t init_res;
    void *pSpiHandle1;

#if SPI_PORT_USE_LOCKS
    spi_port_freertos_init_ll_mutex();
#endif

    if (NULL == handle)
    {
        Spi_ErrorHandlerHook();
    }

    pCanBridgeTaskHdl = (TaskHandle_t *)handle;
    init_res          = SpiAbs_Init_Spi1();
    if (init_res != COMM_SUCCESS)
    {
        return init_res;
    }

    pSpiHandle1 = SpiAbs_GetHandle_Spi1();
    if (NULL == pSpiHandle1)
    {
        return COMM_ERROR;
    }

    Spi_NotifyRegister((SPI_HandleTypeDef *)pSpiHandle1);

    return COMM_SUCCESS;
}

/* Create a semaphore for this SPI handle and remember the pair.        */
void Spi_NotifyRegister(void *hspi)
{
    SPI_HandleTypeDef *handle = hspi;

    if (NULL == handle)
    {
        Spi_ErrorHandlerHook();
    }
#if SPI_PORT_USE_SEMAPHORE
    for (int i = 0; i < MAX_SPI_INSTANCES; ++i)
    {
        if (xSemTable[i].hspi == NULL)
        {
            xSemTable[i].hspi = handle;
            xSemTable[i].xSem =
                xSemaphoreCreateBinaryStatic((StaticQueue_t *)&xSemBuffers[i]);
            configASSERT(xSemTable[i].xSem);
            return;
        }
    }
    /* Ran out of slots – stop here so the bug is obvious.               */
    configASSERT(!"MAX_SPI_INSTANCES too small");
#endif
}

#if SPI_PORT_USE_HOOKS

/* Internal utility – fetch the semaphore that belongs to *hspi.        */
static inline SemaphoreHandle_t prvGetSem(SPI_HandleTypeDef *hspi)
{
    for (int i = 0; i < MAX_SPI_INSTANCES; ++i)
    {
        if (xSemTable[i].hspi == hspi)
        {
            return xSemTable[i].xSem;
        }
    }
    return NULL; /* not registered → configuration bug */
}

/* ---------- task-side: called after the DMA transfer is started ------ */
uint8_t Spi_NotifyTransferIssued(SPI_HandleTypeDef *hspi)
{
    BaseType_t res;

    (void)(hspi);

#if SPI_PORT_USE_SEMAPHORE
    SemaphoreHandle_t xSem = prvGetSem(hspi);
    configASSERT(xSem); /* forgot to register?   */

    res = xSemaphoreTake(xSem, portMAX_DELAY);
#else
    ;
    res = ulTaskNotifyTake(pdTRUE /*clearOnExit*/, portMAX_DELAY);
#endif
    if (res != pdTRUE)
    {
        return 1; /* timeout (shouldn’t happen) */
    }

    return 0;
}

/* ---------- ISR side: successful completion -------------------------- */
uint8_t Spi_NotifyTransferComplete(SPI_HandleTypeDef *hspi)
{
    BaseType_t xHigherPrioTaskWoken = pdFALSE;
    volatile uint32_t IrqPrio       = NVIC_GetPriority(SPI1_IRQn);

    (void)(hspi);

    configASSERT(__get_IPSR() != 0);
    configASSERT(IrqPrio >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);

#if SPI_PORT_USE_SEMAPHORE
    SemaphoreHandle_t xSem = prvGetSem(hspi);

    if (xSem)
    {
        xSemaphoreGiveFromISR(xSem, &xHigherPrioTaskWoken);
    }
    else
    {
        Spi_ErrorHandler();
    }
#else
    vTaskNotifyGiveFromISR(*pCanBridgeTaskHdl, &xHigherPrioTaskWoken);
#endif
    portYIELD_FROM_ISR(xHigherPrioTaskWoken);
    return 0;
}

/* ---------- ISR side: error path ------------------------------------- */
uint8_t Spi_NotifyTransferError(SPI_HandleTypeDef *hspi)
{
    BaseType_t xHigherPrioTaskWoken = pdFALSE;

    (void)(hspi);

#if SPI_PORT_USE_SEMAPHORE
    SemaphoreHandle_t xSem = prvGetSem(hspi);

    if (xSem)
    {
        xSemaphoreGiveFromISR(xSem, &xHigherPrioTaskWoken);
    }
    else
    {
        Spi_ErrorHandler();
    }
#else
    vTaskNotifyGiveFromISR(*pCanBridgeTaskHdl, &xHigherPrioTaskWoken);
#endif
    portYIELD_FROM_ISR(xHigherPrioTaskWoken);
    return 0;
}
#endif

#if SPI_PORT_USE_LOCKS

static SemaphoreHandle_t bus_mtx[MAX_SPI_INSTANCES];

void spi_port_freertos_init_ll_mutex(void)
{
    for (unsigned i = 0; i < MAX_SPI_INSTANCES; ++i)
    {
        bus_mtx[i] = xSemaphoreCreateMutex();
    }
}

void Spi_Lock(uint8_t bus_id)
{
    if (bus_id < MAX_SPI_INSTANCES)
    {
        xSemaphoreTake(bus_mtx[bus_id], portMAX_DELAY);
    }
}

void Spi_Unlock(uint8_t bus_id)
{
    if (bus_id < MAX_SPI_INSTANCES)
    {
        xSemaphoreGive(bus_mtx[bus_id]);
    }
}

#endif
