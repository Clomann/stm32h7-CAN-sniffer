/**
  ******************************************************************************
  * @file    SPI/SPI_FullDuplex_ComDMA/CM7/Src/main.c
  * @author  MCD Application Team
  * @brief   This sample code shows how to use STM32H7xx SPI HAL API to transmit
  *          and receive a data buffer with a communication process based on
  *          DMA transfer.
  *          The communication is done using 1 Board.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#define APP_UPDATE_SETTING_DEFINED

/* Includes ------------------------------------------------------------------*/
#include "main.h"

//#include "SD.h"
//#include "Spi_Cmds.h"

#include <string.h>

#include "FileHandler.h"
#include "HttpAbs.h"
#include "CanLogBuffer.h"
#include "SettingsHandler.h"

/** @addtogroup STM32H7xx_HAL_Examples
  * @{
  */

/** @addtogroup SPI_FullDuplex_ComDMA
  * @{
  */

/* Private typedef -----------------------------------------------------------*/

typedef struct {
  struct {
    const char * filename;
    uint32_t count;
    uint32_t timestamp;
    uint8_t openRes;
    FatFsDeviceType writeFileDevice;
  } CanLog;
  struct {
    const char * filename;
    uint32_t count;
    uint32_t timestamp;
    uint8_t openRes;
    FatFsDeviceType writeFileDevice;
  } Config;
  uint8_t mountRes;
} AppControlDataType;

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

#define USE_HAL_SPI_REGISTER_CALLBACKS = 1U;
#define HSEM_ID_0 (0U) /* HW semaphore 0*/

uint8_t run;
static AppConfigType AppConfig;
static const char CanLogFileName[] = "can.log";
static const char ConfigFileName[] = "conf.txt";

static AppControlDataType AppCtrlData = { 
  .CanLog = {
    .filename = CanLogFileName,
    .count = 0,
    .timestamp = 0,
    .openRes = 1,
    .writeFileDevice.readTargetSize = 0U,
  },
  .Config = {
    .filename = ConfigFileName,
    .count = 0,
    .timestamp = 0,
    .openRes = 1,
    .writeFileDevice.readTargetSize = 0U,
  },
  .mountRes = 1,
};

uint8_t Data[BLOCK_SIZE] = {0};

/* Private function prototypes -----------------------------------------------*/
static void MPU_Config(void);
static void SystemClock_Config(void);
// static uint16_t Buffercmp(uint8_t *pBuffer1, uint8_t *pBuffer2, uint16_t BufferLength);
static void CPU_CACHE_Enable(void);

/* Private functions ---------------------------------------------------------*/

static void appConfigHandlerInit(AppControlDataType *data)
{
  SettingsHandler_Init(&AppConfig);

  if ( RES_OK == data->mountRes)
  {
    data->Config.openRes = FatFS_SD_OpenFileForOverWrite(
                              &(data->Config.writeFileDevice),
                              data->Config.filename);
  }
  else 
  {
    data->Config.openRes = 1U;
  }
}

static void appCanLogHandlerInit(AppControlDataType *data)
{
  CanLogBuffer_Init();

  if ( RES_OK == data->mountRes)
  {
    data->CanLog.openRes = FatFS_SD_OpenFileForWrite(
                                    &(data->CanLog.writeFileDevice), 
                                    data->CanLog.filename);
  }
  else 
  {
    data->CanLog.openRes = 1U;
  }
}

static void appCanLogFillDummyEntry(CanLogEntryType *dummy, uint32_t timestamp)
{
  dummy->timestamp_us.lsb = timestamp & 0xFFFF;
  dummy->timestamp_us.msb = (timestamp >> 16) & 0xFF;
  dummy->dlc = 0x11;
  memset( &dummy->can_id, 0x22, sizeof(dummy->can_id) );
  memset( dummy->data, 0x33, sizeof(dummy->data) );
}

static void appCanLogHandlerPoll(AppControlDataType *data)
{
  uint32_t timestamp;
  uint32_t timedelta;
  uint8_t BlockIsReady;
  uint32_t DataLength;
  CanLogEntryType NewEntry;

  timestamp = HAL_GetTick();
  timedelta = timestamp - data->CanLog.timestamp;
  
  appCanLogFillDummyEntry(&NewEntry, timestamp);
  
  CanLogBuffer_AddEntry(&NewEntry);

  CanLogBuffer_IsBlockReady(&BlockIsReady);

  if ( 0 != data->CanLog.openRes || timedelta<1000 )
  {    
    /* quit */
  }
  else if ( 0 == BlockIsReady )
  {
    /* quit since block is not ready to be written */
  }
  else if ( 0 !=  CanLogBuffer_ReadNextBlock(Data, &DataLength) )
  {
    /* quit since data could not be read */
  }
  else if (FR_OK == FatFS_SD_WriteFile(
              &(data->CanLog.writeFileDevice), 
              (const char *)Data, 
              DataLength) )
  {
    FatFS_SD_Flush(&(data->CanLog.writeFileDevice));
    data->CanLog.timestamp =  timestamp;
  }
  else
  {
    data->CanLog.timestamp =  timestamp;
  }
}

static void appCanLogHandlerDeInit(AppControlDataType * data)
{
  if ( 0 == data->CanLog.openRes )
  { 
    FatFS_SD_CloseFile(&(data->CanLog.writeFileDevice));
  }
}

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  static FatFsDeviceType ConfigReadFileDevice;
  int32_t timeout;
  uint32_t spiClockSource;
  HAL_StatusTypeDef HalStatus;
  struct Config { 
    char data[1024U];
    uint32_t len;
  } Config = {0U};

  /* Configure the MPU attributes */
  MPU_Config();

  /* Enable the CPU Cache */
  CPU_CACHE_Enable();

  /* Wait until CPU2 boots and enters in stop mode or timeout*/
  timeout = 0xFFFF;
  while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) != RESET) && (timeout-- > 0));
  if ( timeout < 0 )
  {
    Error_Handler();
  }

  /* STM32H7xx HAL library initialization:
       - Systick timer is configured by default as source of time base, but user
         can eventually implement his proper time base source (a general purpose
         timer for example or other time source), keeping in mind that Time base
         duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
         handled in milliseconds basis.
       - Set NVIC Group Priority to 4
       - Low Level Initialization
     */
  HAL_Init();

  /* Configure the system clock to 400 MHz */
  SystemClock_Config();

  /* When system initialization is finished, Cortex-M7 will release Cortex-M4  by means of
     HSEM notification */

  // Wait for PLL2 to lock
  while (!__HAL_RCC_GET_FLAG(RCC_FLAG_PLL2RDY)) {
      // Optional: Timeout handling can be implemented here
  }

  // Check SPI1 clock source
    if (__HAL_RCC_GET_SPI1_SOURCE() != RCC_SPI1CLKSOURCE_PLL2) {
  	  Error_Handler();
    }

    if (__HAL_RCC_GET_SPI4_SOURCE() != RCC_SPI1CLKSOURCE_PLL2) {
	  Error_Handler();
	}

    // Verify that HSI is enabled
    if (0 == __HAL_RCC_GET_FLAG(RCC_FLAG_HSIRDY)) {
        Error_Handler();
    }


  /*HW semaphore Clock enable*/
  __HAL_RCC_HSEM_CLK_ENABLE();

  /*Take HSEM */
  HAL_HSEM_FastTake(HSEM_ID_0);
  /*Release HSEM in order to notify the CPU2(CM4)*/
  HAL_HSEM_Release(HSEM_ID_0,0);

  /* wait until CPU2 wakes up from stop mode */
  timeout = 0xFFFF;
  while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) == RESET) && (timeout-- > 0));
  if ( timeout < 0 )
  {
    Error_Handler();
  }

  /* Configure LED1, LED2 and LED3 */
  BSP_LED_Init(LED1);
  BSP_LED_Init(LED2);
  BSP_LED_Init(LED3);

   /*##-1- Configure the SPI peripheral #######################################*/

  HalStatus = SPI_Init();

  if(HalStatus != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  spiClockSource = __HAL_RCC_GET_SPI1_SOURCE();
  (void)spiClockSource;

  /* Infinite loop */
  while (1)
  {
	//aTxBuffer[0] = counter++;
//	SCB_CleanDCache_by_Addr ((uint32_t *)aTxBuffer, BUFFERSIZE);
#if WAIT_FOR_USER_BUTTON
	/* Configure User push-button button */
	BSP_PB_Init(BUTTON_USER,BUTTON_MODE_GPIO);
	/* Wait for User push-button press before starting the Communication */
	while (BSP_PB_GetState(BUTTON_USER) != GPIO_PIN_SET)
	{
	  BSP_LED_Toggle(LED1);
	  HAL_Delay(100);
	}
	BSP_LED_Off(LED1);
#endif



  /* USER CODE END 5 */

	/*##-2- Start the Full Duplex Communication process ########################*/
	/* While the SPI in TransmitReceive process, user can transmit data through
	   "aTxBuffer" buffer & receive data through "aRxBuffer" */
  Spi_PwrOn();
  if (0U == FatFS_SD_LoadConfig(&ConfigReadFileDevice, Config.data, &Config.len) )
  {
    SettingsHandler_ParseConfig(Config.data, Config.len, &AppConfig);
    SettingsHandler_Init(&AppConfig);
  }

  http_init();

  run = 1U;
  
  if (0 != AppCtrlData.mountRes)
    AppCtrlData.mountRes = FatFS_SD_Mount();

  appCanLogHandlerInit(&AppCtrlData);
  
  appConfigHandlerInit(&AppCtrlData);

  while (run)
  {
    http_poll();

    appCanLogHandlerPoll(&AppCtrlData);

    if (0 == AppCtrlData.Config.openRes)
    {
      SettingsHandler_Poll(&AppCtrlData.Config.writeFileDevice, &AppConfig);
    }
  }

  appCanLogHandlerDeInit(&AppCtrlData);

  if (0 == AppCtrlData.mountRes)
    FatFS_SD_Unmount();

  Spi_PwrOff();
  

	/*##-3- Wait for the end of the transfer ###################################*/
	/*  Before starting a new communication transfer, you must wait the callback call
		to get the transfer complete confirmation or an error detection.
		For simplicity reasons, this example is just waiting till the end of the
		transfer, but application may perform other tasks while transfer operation
		is ongoing. */

//	/* Invalidate cache prior to access by CPU */
//	SCB_InvalidateDCache_by_Addr ((uint32_t *)aRxBuffer, BUFFERSIZE);
//
//	switch(wTransferState)
//	{
//	  case TRANSFER_COMPLETE :
//	/*##-4- Compare the sent and received
//	 *  buffers ##############################*/
//		if(Buffercmp((uint8_t*)aTxBuffer, (uint8_t*)aRxBuffer, BUFFERSIZE))
//		{
//		  /* Processing Error */
//		  //Error_Handler();
//		  BSP_LED_On(LED3);
//		}
//		else
//		{
//			BSP_LED_Off(LED3);
//		}
//		break;
//	  default :
//		Error_Handler();
//		break;
//	}
  }
}



/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE BYPASS)
  *            SYSCLK(Hz)                     = 400000000 (CPU Clock)
  *            HCLK(Hz)                       = 200000000 (AXI and AHBs Clock)
  *            AHB Prescaler                  = 2
  *            D1 APB3 Prescaler              = 2 (APB3 Clock  100MHz)
  *            D2 APB1 Prescaler              = 2 (APB1 Clock  100MHz)
  *            D2 APB2 Prescaler              = 2 (APB2 Clock  100MHz)
  *            D3 APB4 Prescaler              = 2 (APB4 Clock  100MHz)
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 4
  *            PLL_N                          = 400
  *            PLL_P                          = 2
  *            PLL_Q                          = 4
  *            PLL_R                          = 2
  *            VDD(V)                         = 3.3
  *    run        Flash Latency(WS)              = 4
  * @param  None
  * @retval None
  */
 static void SystemClock_Config(void)
 {
   RCC_ClkInitTypeDef RCC_ClkInitStruct;
   RCC_OscInitTypeDef RCC_OscInitStruct;
   HAL_StatusTypeDef ret = HAL_OK;
 
   /*!< Supply configuration update enable */
   HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);
   /* The voltage scaling allows optimizing the power consumption when the device is
      clocked below the maximum system frequency, to update the voltage scaling value
      regarding system frequency refer to product datasheet.  */
   __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
 
   while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
 
   /* Enable HSE Oscillator and activate PLL with HSE as source */
   RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
   RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
   RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
   RCC_OscInitStruct.CSIState = RCC_CSI_OFF;
   RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
   RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
 
   RCC_OscInitStruct.PLL.PLLM = 4;
   RCC_OscInitStruct.PLL.PLLN = 400;
   RCC_OscInitStruct.PLL.PLLFRACN = 0;
   RCC_OscInitStruct.PLL.PLLP = 2; /* PLL1P: drives SYSCLK */
   RCC_OscInitStruct.PLL.PLLR = 2;
   RCC_OscInitStruct.PLL.PLLQ = 20; /* PLL1Q: drives FDCAN */
 
   RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
   RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_1;
   ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
   if(ret != HAL_OK)
   {
     Error_Handler();
   }
   
   RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
   PeriphClkInitStruct.FdcanClockSelection = RCC_FDCANCLKSOURCE_PLL;
   PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL2;
   PeriphClkInitStruct.Spi45ClockSelection = RCC_SPI45CLKSOURCE_PLL2;
   PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI1 | RCC_PERIPHCLK_SPI4 | RCC_PERIPHCLK_FDCAN;
   PeriphClkInitStruct.PLL2.PLL2M = 2;
   PeriphClkInitStruct.PLL2.PLL2N = 96;
   PeriphClkInitStruct.PLL2.PLL2FRACN = 1;
   PeriphClkInitStruct.PLL2.PLL2P = 20; /* PLL2P: drives SPI1 */
   PeriphClkInitStruct.PLL2.PLL2Q = 20; /* PLL2P: drives SPI4 */
   PeriphClkInitStruct.PLL2.PLL2R = 2;
 
   PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
   PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_2;
 
   ret = HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
   // Configure and enable PLL2
   if (HAL_OK != ret) {
       Error_Handler();  // Configuration failed
   }
 
 /* Select PLL as system clock source and configure  bus clocks dividers */
   RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | \
                                  RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 | \
                                  RCC_CLOCKTYPE_PCLK2  | RCC_CLOCKTYPE_D3PCLK1);
 
   RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
   RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
   RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
   RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
   RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
   RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
   RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
   ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
   if(ret != HAL_OK)
   {
     Error_Handler();
   }
 
 }

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
  BSP_LED_Off(LED1);
  /* Turn LED3 on */
  BSP_LED_On(LED3);

}

/**
  * @brief  Compares two buffers.
  * @param  pBuffer1, pBuffer2: buffers to be compared.
  * @param  BufferLength: buffer's length
  * @retval 0  : pBuffer1 identical to pBuffer2
  *         >0 : pBuffer1 differs from pBuffer2
  */
// static uint16_t Buffercmp(uint8_t* pBuffer1, uint8_t* pBuffer2, uint16_t BufferLength)
// {
//   while (BufferLength--)
//   {
//     if((*pBuffer1) != *pBuffer2)
//     {
//       return BufferLength;
//     }
//     pBuffer1++;
//     pBuffer2++;
//   }

//   return 0;
// }


/**
  * @brief  Configure the MPU attributes
  * @param  None
  * @retval None
  */
static void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;

  /* Disable the MPU */
  HAL_MPU_Disable();

  /* Configure the MPU as Strongly ordered for not defined regions */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x00;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Configure the MPU attributes as Device not cacheable
     for ETH DMA descriptors */
     MPU_InitStruct.Enable = MPU_REGION_ENABLE;
     MPU_InitStruct.BaseAddress = 0x30000000;
     MPU_InitStruct.Size = MPU_REGION_SIZE_1KB;
     MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
     MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
     MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
     MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
     MPU_InitStruct.Number = MPU_REGION_NUMBER1;
     MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
     MPU_InitStruct.SubRegionDisable = 0x00;
     MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
   
     HAL_MPU_ConfigRegion(&MPU_InitStruct);
   
     /* Configure the MPU attributes as Normal Non Cacheable
        for LwIP RAM heap which contains the Tx buffers */
     MPU_InitStruct.Enable = MPU_REGION_ENABLE;
     MPU_InitStruct.BaseAddress = 0x30004000;
     MPU_InitStruct.Size = MPU_REGION_SIZE_16KB;
     MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
     MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
     MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
     MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
     MPU_InitStruct.Number = MPU_REGION_NUMBER2;
     MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
     MPU_InitStruct.SubRegionDisable = 0x00;
     MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
   
     HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}

/**
  * @}
  */

/**
  * @}
  */

