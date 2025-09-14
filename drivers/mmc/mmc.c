#include <assert.h>

#include "mmc.h"
#include "stm32h7xx_hal.h"

#define DATA_SIZE              ((uint32_t)0x06400000U) /* Data Size 100Mo */

/* ------ Buffer Size ------ */
#define BUFFER_SIZE            ((uint32_t)0x00040000U) /* 256Ko */

#define NB_BUFFER              DATA_SIZE / BUFFER_SIZE
#define NB_BLOCK_BUFFER        BUFFER_SIZE / BLOCKSIZE /* Number of Block (512o) by Buffer */
#define BUFFER_WORD_SIZE       (BUFFER_SIZE>>2)        /* Buffer size in Word */


#define MMC_TIMEOUT            ((uint32_t)0x00100000U)
#define ADDRESS                ((uint32_t)0x00000400U) /* MMC Address to write/read data */
#define DATA_PATTERN           ((uint32_t)0xB5F3A5F3U) /* Data pattern to write */

/* Size of buffers */
#define BUFFERSIZE                 (COUNTOF(aMmcTxBuffer) - 1)
/* Exported macro ------------------------------------------------------------*/
#define COUNTOF(__BUFFER__)        (sizeof(__BUFFER__) / sizeof(*(__BUFFER__)))

static MMC_HandleTypeDef MMCHandle;
__attribute__((section (".dma_buffer"))) uint8_t aMmcTxBuffer[BUFFER_WORD_SIZE*4];

static void Error_Handler()
{
    Mmc_Errorhandler();
}

void SDMMC1_IRQHandler(void);

void Mmc_Errorhandler()
{

}

/**
  * @brief  Wait MMC Card ready status
  * @param  None
  * @retval None
  */
static uint8_t Wait_MMCCARD_Ready(void)
{
  uint32_t loop = MMC_TIMEOUT;
  
  /* Wait for the Erasing process is completed */
  /* Verify that MMC card is ready to use after the Erase */
  while(loop > 0)
  {
    loop--;
    if(HAL_MMC_GetCardState(&MMCHandle) == HAL_MMC_CARD_TRANSFER)
    {
        return HAL_OK;
    }
  }
  return HAL_ERROR;
}

void Mmc_Init()
{
    uint32_t MmcClock;
    HAL_MMC_CardCIDTypeDef pCID;
    HAL_MMC_CardCSDTypeDef pCSD;

    MmcClock = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_SDMMC);

    assert(200000000 == MmcClock);

    MMCHandle.Instance = SDMMC1;
    HAL_MMC_DeInit(&MMCHandle);

    /* if CLKDIV = 0 then SDMMC Clock frequency = SDMMC Kernel Clock
    else SDMMC Clock frequency = SDMMC Kernel Clock / [2 * CLKDIV]. 
    SDMMC Kernel Clock = 200MHz, DMMC Clock frequency = 50MHz  */
    MMCHandle.Init.ClockEdge           = SDMMC_CLOCK_EDGE_RISING;
    MMCHandle.Init.ClockPowerSave      = SDMMC_CLOCK_POWER_SAVE_DISABLE;
    MMCHandle.Init.BusWide             = SDMMC_BUS_WIDE_8B;
    MMCHandle.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;
    MMCHandle.Init.ClockDiv            = 2;


    if(HAL_MMC_Init(&MMCHandle) != HAL_OK)
    {
        Error_Handler();
    }
    if(HAL_MMC_ConfigWideBusOperation(&MMCHandle,SDMMC_BUS_WIDE_8B) != HAL_OK)
    {
        Error_Handler();
    }

    if(HAL_MMC_Erase(&MMCHandle, ADDRESS, ADDRESS+BUFFERSIZE) != HAL_OK)
    {
        Error_Handler();
    }
    if(Wait_MMCCARD_Ready() != HAL_OK)
    {
        Error_Handler();
    }

    HAL_MMC_GetCardCID(&MMCHandle, &pCID);
    HAL_MMC_GetCardCSD(&MMCHandle, &pCSD);
}


/**
  * @brief MMC MSP Initialization
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration
  *           - DMA configuration for requests by peripheral 
  *           - NVIC configuration for DMA and MMC interrupts
  * @param hmmc: MMC handle pointer
  * @retval None
  */

void HAL_MMC_MspInit(MMC_HandleTypeDef *hmmc)
{
    /* __weak function can be modified by the application */

    GPIO_InitTypeDef gpio_init_structure;

    /* Enable SDIO clock */
    __HAL_RCC_SDMMC1_CLK_ENABLE();

    /* Enable GPIOs clock */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();


    /* Common GPIO configuration */
    gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
    gpio_init_structure.Pull      = GPIO_PULLUP;
    gpio_init_structure.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    
    /* SDMMC GPIO CLKIN PB8, CDIR PB9 D0 PC8, D1 PC9, D2 PC10, D3 PC11, CK PC12, CMD PD2 */
    /* GPIOC configuration */
    gpio_init_structure.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
    gpio_init_structure.Alternate = GPIO_AF12_SDIO1;
    HAL_GPIO_Init(GPIOC, &gpio_init_structure);

    /* GPIOD configuration */
    gpio_init_structure.Pin = GPIO_PIN_2;
    gpio_init_structure.Alternate = GPIO_AF12_SDIO1;
    HAL_GPIO_Init(GPIOD, &gpio_init_structure);

    gpio_init_structure.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    gpio_init_structure.Alternate = GPIO_AF7_SDIO1;
    HAL_GPIO_Init(GPIOB, &gpio_init_structure);

    /* NVIC configuration for SDIO interrupts */
    HAL_NVIC_SetPriority(SDMMC1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(SDMMC1_IRQn);

}

/**
  * @brief MMC MSP De-Initialization
  *        This function frees the hardware resources used in this example:
  *          - Disable the Peripheral's clock
  *          - Revert GPIO, DMA and NVIC configuration to their default state
  * @param hmmc: MMC handle pointer
  * @retval None
  */
void HAL_MMC_MspDeInit(MMC_HandleTypeDef *hmmc)
{
  
  /* DeInit GPIO pins can be done in the application 
  (by surcharging this __weak function) */
  
    /* Enable GPIOs clock */
  __HAL_RCC_GPIOB_CLK_DISABLE();
  __HAL_RCC_GPIOC_CLK_DISABLE();
  __HAL_RCC_GPIOD_CLK_DISABLE();
  
  /* Disable SDMMC1 clock */
  __HAL_RCC_SDMMC1_CLK_DISABLE();
}


/**
  * @brief  This function handles MMC interrupt request.
  * @param  None
  * @retval None
  */
void SDMMC1_IRQHandler(void)
{
  HAL_MMC_IRQHandler(&MMCHandle);
}

