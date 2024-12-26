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

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "main.h"
#include "Spi_Cmds.h"
#include "Sd.h"

/** @addtogroup STM32H7xx_HAL_Examples
  * @{
  */

/** @addtogroup SPI_FullDuplex_ComDMA
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
enum {
  TRANSFER_WAIT,
  TRANSFER_COMPLETE,
  TRANSFER_ERROR
};

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* SPI handler declaration */
SPI_HandleTypeDef SpiHandle1;

/* Buffer used for transmission */
uint8_t aTxBuffer[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // "****SPI - Two Boards communication based on DMA **** SPI Message ********* SPI Message *********";
static uint8_t SPI_CMD_READ_BUFFER[512] = {0};

/* Buffer used for reception */
#define BUFFER_ALIGNED_SIZE (((BUFFERSIZE+31)/32)*32)
ALIGN_32BYTES(uint8_t aRxBuffer[BUFFER_ALIGNED_SIZE]);

/* transfer state */
__IO uint32_t wTransferState = TRANSFER_WAIT;

#define USE_HAL_SPI_REGISTER_CALLBACKS = 1U;
#define HSEM_ID_0 (0U) /* HW semaphore 0*/

#define CS_ACTIVE_HIGH 0

/* Private function prototypes -----------------------------------------------*/
static void MPU_Config(void);
static void SystemClock_Config(void);
static void Error_Handler(void);
static uint16_t Buffercmp(uint8_t *pBuffer1, uint8_t *pBuffer2, uint16_t BufferLength);
static void CPU_CACHE_Enable(void);
uint8_t SD_Spi_PowerUp(void);

//static uint8_t Spi_Receive(uint8_t *r, uint8_t);
static uint8_t Spi_PollForResponse(uint8_t *);
static uint8_t Spi_ParseResponse(const uint8_t *, uint8_t, uint8_t *);
static uint8_t Spi_SendReceiveMsg(const uint8_t *, uint8_t *, uint8_t);

static uint8_t SD_Spi_Initialize(uint8_t);
void SD_Spi_ReadBlock(uint32_t, uint8_t *);
uint8_t SD_Spi_WaitTillIdle();

/* Private functions ---------------------------------------------------------*/

uint8_t SD_readSingleBlock(uint32_t address, Spi_R1Response * pResponse);

uint8_t Spi_CsEnable()
{
#if CS_ACTIVE_HIGH
	HAL_GPIO_WritePin(SPI1_SS_GPIO_PORT, SPI1_SS_PIN, GPIO_PIN_SET);
#else
	HAL_GPIO_WritePin(SPI1_SS_GPIO_PORT, SPI1_SS_PIN, GPIO_PIN_RESET);
#endif
	return 0;
}

static inline uint8_t Spi_CsDisable()
{
#if CS_ACTIVE_HIGH
	HAL_GPIO_WritePin(SPI1_SS_GPIO_PORT, SPI1_SS_PIN, GPIO_PIN_RESET);
#else
	HAL_GPIO_WritePin(SPI1_SS_GPIO_PORT, SPI1_SS_PIN, GPIO_PIN_SET);
#endif
	return 0;
}

uint8_t SD_Spi_SendCommand(uint8_t cmd, uint32_t payload)
{
	SD_Spi_CreateCommand(cmd, payload, aTxSpiCmd);
	Spi_SendReceiveMsg(aTxSpiCmd, aRxBuffer, COUNTOF(aTxSpiCmd));

	return 0;
}

uint8_t SD_Spi_GoIdleState(Spi_R1Response * pResponse)
{
    // assert chip select
//	Spi_SendReceiveMsg(aTxSpiDummy1, aRxBuffer, COUNTOF(aTxSpiDummy1));
	Spi_CsEnable();
//	Spi_SendReceiveMsg(aTxSpiDummy1, aRxBuffer, COUNTOF(aTxSpiDummy1));

    SD_Spi_SendCommand(SD_SPI_CMD0, 0x00000000);
    Spi_PollForResponse(&pResponse->byte);

    // deassert chip select
//	Spi_SendReceiveMsg(aTxSpiDummy1, aRxBuffer, COUNTOF(aTxSpiDummy1));
	Spi_CsDisable();
//	Spi_SendReceiveMsg(aTxSpiDummy1, aRxBuffer, COUNTOF(aTxSpiDummy1));

    return 0;
}

uint8_t SD_Spi_SendIfCond(Spi_R1Response * pResponse)
{
    // assert chip select
//	Spi_SendReceiveMsg(aTxSpiDummy1, aRxBuffer, COUNTOF(aTxSpiDummy1));
	Spi_CsEnable();
//	Spi_SendReceiveMsg(aTxSpiDummy1, aRxBuffer, COUNTOF(aTxSpiDummy1));

    // send CMD8
	SD_Spi_SendCommand(SD_SPI_CMD8, 0x00000000);
	Spi_PollForResponse(&pResponse->byte);

    // deassert chip select
//	Spi_SendReceiveMsg(aTxSpiDummy1, aRxBuffer, COUNTOF(aTxSpiDummy1));
	Spi_CsDisable();
//	Spi_SendReceiveMsg(aTxSpiDummy1, aRxBuffer, COUNTOF(aTxSpiDummy1));

	return 0;
}

uint8_t SD_Spi_SendApp(Spi_R1Response * pResponse)
{
	// assert chip select
//	Spi_SendReceiveMsg(aTxSpiDummy1, aRxBuffer, COUNTOF(aTxSpiDummy1));
	Spi_CsEnable();
//	Spi_SendReceiveMsg(aTxSpiDummy1, aRxBuffer, COUNTOF(aTxSpiDummy1));

	SD_Spi_SendCommand(SD_SPI_CMD55, 0x00000000); // Precede ACMD41 with CMD55
	Spi_CsDisable();

	Spi_CsEnable();
	Spi_PollForResponse(&pResponse->byte);

	// deassert chip select
//	Spi_SendReceiveMsg(aTxSpiDummy1, aRxBuffer, COUNTOF(aTxSpiDummy1));
	Spi_CsDisable();
//	Spi_SendReceiveMsg(aTxSpiDummy1, aRxBuffer, COUNTOF(aTxSpiDummy1));

	return 0;
}

uint8_t SD_Spi_SendOpCond(Spi_R1Response * pResponse)
{
    // assert chip select
//	Spi_CsDisable();
//    Spi_SendReceiveMsg(aTxSpiDummy1, aRxBuffer, COUNTOF(aTxSpiDummy1));
	Spi_CsEnable();
//	Spi_SendReceiveMsg(aTxSpiDummy1, aRxBuffer, COUNTOF(aTxSpiDummy1));

	SD_Spi_SendCommand(SD_SPI_ACMD41, 0x40000000); // Precede ACMD41 with CMD55
	Spi_CsDisable();

	Spi_CsEnable();
	Spi_PollForResponse(&pResponse->byte);
    // deassert chip select
//    Spi_SendReceiveMsg(aTxSpiDummy1, aRxBuffer, COUNTOF(aTxSpiDummy1));
	Spi_CsDisable();
//	Spi_SendReceiveMsg(aTxSpiDummy1, aRxBuffer, COUNTOF(aTxSpiDummy1));

    return 0;
}

uint8_t SD_Spi_ReadOCR(Spi_R1Response * pResponse)
{
    // assert chip select
//	Spi_SendReceiveMsg(aTxSpiDummy1, aRxBuffer, COUNTOF(aTxSpiDummy1));
	Spi_CsEnable();
//	Spi_SendReceiveMsg(aTxSpiDummy1, aRxBuffer, COUNTOF(aTxSpiDummy1));

	SD_Spi_SendCommand(SD_SPI_CMD58, 0x00000000);
	Spi_PollForResponse(&pResponse->byte);

    // deassert chip select
//    Spi_SendReceiveMsg(aTxSpiDummy1, aRxBuffer, COUNTOF(aTxSpiDummy1));
	Spi_CsDisable();
//	Spi_SendReceiveMsg(aTxSpiDummy1, aRxBuffer, COUNTOF(aTxSpiDummy1));

    return 0;
}


uint8_t SD_Spi_ReadRes7(uint8_t * pRxBuffer)
{
	Spi_CsEnable();
	Spi_SendReceiveMsg(aTxSpiDummy4, pRxBuffer, COUNTOF(aTxSpiDummy4)); // Read CMD8 response
	Spi_CsEnable();

	return 0;
}

/**
  * @brief  Initializes the card and determines its type (e.g., SDSC, SDHC, SDXC).
			Sets up the card for further operations.
  * @param  None
  * @retval None
  */
uint8_t SD_Spi_Initialize(uint8_t CsLine)
{
	uint8_t RetVal;
	uint8_t counter;
	uint32_t OcrByte;
	Spi_R1Response response;
	OCR_Register OcrResponse;
	uint8_t ocr[4];

	RetVal = 0x00;

	HAL_Delay(100);
	SD_Spi_PowerUp();
	HAL_Delay(100);
	SD_Spi_GoIdleState(&response);


	if (0x01 == response.byte)
	{
		// SD card successfully initialized
		SD_Spi_WaitTillIdle();
		SD_Spi_SendIfCond(&response);

		if ((response.byte & 0x3F) == 0x01)  // Ignore bit 6 (Erase Reset)
		{
			SD_Spi_ReadRes7(aRxBuffer);
			memcpy(ocr, aRxBuffer, 4);
			SD_Spi_WaitTillIdle();

			if (ocr[2] == 0x01 && ocr[3] == 0xAA) {
				counter = 0;
				do
				{
					SD_Spi_SendApp(&response);

					if (counter > 100)
					{
						RetVal = 3;
					}

					if (response.byte < 0x02 && 0 == RetVal)
					{

					}
					SD_Spi_SendOpCond(&response);

					HAL_Delay(10);

					counter++;
				} while (response.byte != 0x00 && 0 == RetVal);  // Wait for idle state to clear

				while (0U == RetVal && 0U == OcrResponse.cpusb)
				{
					HAL_Delay(10);

					SD_Spi_WaitTillIdle();

					// Send CMD58 to read OCR and check CCS bit
					SD_Spi_ReadOCR(&response);

					if (response.byte == 0x00) {

						SD_Spi_ReadRes7(aRxBuffer);
						OcrByte = (aRxBuffer[0] << 24) 	| (aRxBuffer[1] << 16) | (aRxBuffer[2] << 8) | aRxBuffer[3];
						SD_Spi_Ocr2Bitfield(OcrByte, &OcrResponse);
						if (IS_CARD_READY(OcrByte))
						{
							// card is a ver. 2.0 or later high capacity SD card (SDHC) or extended capacity SD card (SCXC)
						}
						else
						{
							// card is a ver. 2.0 or later standard capacity SD memory card
						}
					}
				}

			}
			else
			{
				RetVal = 2;
			}
		}
	}
	else
	{
		RetVal = 1;
	}

	return RetVal;
}

uint8_t Spi_Send(uint8_t * pTxBuffer, uint8_t TxBytes)
{
	uint8_t RetVal;

	RetVal = HAL_SPI_Transmit_DMA(&SpiHandle1, pTxBuffer, TxBytes);

	if (RetVal == HAL_BUSY)
	{

	}
	else if (RetVal != HAL_OK)
	{
	  /* Transfer error in transmission process */
	  Error_Handler();
	}

	while (wTransferState == TRANSFER_WAIT)
	{
	}

	// Wait until the SPI is no longer busy
	while (HAL_SPI_GetState(&SpiHandle1) != HAL_SPI_STATE_READY) {}

	return RetVal;
}

//uint8_t Spi_Receive(uint8_t * pRxBuffer, uint8_t RxBytes)
//{
//	uint8_t RetVal;
//
//#if CS_ACTIVE_HIGH
//	HAL_GPIO_WritePin(SPI1_SS_GPIO_PORT, SPI1_SS_PIN, GPIO_PIN_SET);
//#else
//	HAL_GPIO_WritePin(SPI1_SS_GPIO_PORT, SPI1_SS_PIN, GPIO_PIN_RESET);
//#endif
//
//	RetVal = HAL_SPI_Receive_DMA(&SpiHandle1, pRxBuffer, RxBytes);
//
//	if (RetVal == HAL_BUSY)
//	{
//
//	}
//	else if (RetVal != HAL_OK)
//	{
//	  /* Transfer error in transmission process */
//	  Error_Handler();
//	}
//
//	while (wTransferState == TRANSFER_WAIT)
//	{
//	}
//
//	// Wait until the SPI is no longer busy
//	while (HAL_SPI_GetState(&SpiHandle1) != HAL_SPI_STATE_READY) {}
//
//#if CS_ACTIVE_HIGH
//	HAL_GPIO_WritePin(SPI1_SS_GPIO_PORT, SPI1_SS_PIN, GPIO_PIN_RESET);
//#else
//	HAL_GPIO_WritePin(SPI1_SS_GPIO_PORT, SPI1_SS_PIN, GPIO_PIN_SET);
//#endif
//
//	SCB_InvalidateDCache_by_Addr ((uint32_t *)pRxBuffer, RxBytes);
//
//	return RetVal;
//}

uint8_t Spi_SendReceiveMsg(const uint8_t * pTxBuffer, uint8_t * pRxBuffer, uint8_t TxBytes)
{
	uint8_t RetVal;

	SCB_CleanDCache_by_Addr ((uint32_t *)pTxBuffer, TxBytes);

	RetVal = HAL_SPI_TransmitReceive_DMA(&SpiHandle1, pTxBuffer, pRxBuffer, TxBytes);

	if (RetVal == HAL_BUSY)
	{

	}
	else if (RetVal != HAL_OK)
	{
	  /* Transfer error in transmission process */
	  Error_Handler();
	}

	while (wTransferState == TRANSFER_WAIT)
	{
	}

	// Wait until the SPI is no longer busy
	while (HAL_SPI_GetState(&SpiHandle1) != HAL_SPI_STATE_READY) {}

	SCB_InvalidateDCache_by_Addr ((uint32_t *)pRxBuffer, TxBytes);

	return RetVal;
}

uint8_t Spi_ParseResponse(const uint8_t * buffer, uint8_t length, uint8_t * response)
{
	uint8_t RetVal = 1;

	*response = 0xFF;

	for (int i=0; i<length; i++)
	{
		if (0xFF != buffer[i])
		{
			*response = buffer[i];
			RetVal = 0;
		}
	}

	return RetVal;
}

uint8_t Spi_readByte(uint8_t * pResponse)
{
	uint8_t RetVal;

	RetVal = Spi_SendReceiveMsg((uint8_t*)aTxSpiDummy1, (uint8_t *)aRxBuffer, COUNTOF(aTxSpiDummy1));

	Spi_ParseResponse(aRxBuffer, COUNTOF(aTxSpiDummy1), pResponse);

	return RetVal;
}

uint8_t Spi_PollForResponse(uint8_t * pResponse)
{
	uint8_t NoResponseReceived;
	uint8_t RetVal;
	uint8_t counter;
	const uint8_t RetryCount = 10;

	counter = 0;
	NoResponseReceived = 1;

	do
	{
		Spi_readByte(pResponse);

		if (0xFF != *pResponse)
		{
			NoResponseReceived = 0;
		}

		counter++;
	} while (NoResponseReceived && (RetryCount > counter) );

	if (0 == NoResponseReceived)
	{
		RetVal = 0;
	}
	else
	{
		RetVal = 1;
	}

	return RetVal;
}

uint8_t Spi_goHighSpeed()
{
	uint8_t RetVal;

	RetVal = 0U;

	if(HAL_SPI_Init(&SpiHandle1) != HAL_OK)
	{
		/* Initialization Error */
		Error_Handler();
		RetVal = 1U;
	}

	/*##-1- Configure the SPI peripheral #######################################*/
	/* Set the SPI1 parameters */
	SpiHandle1.Instance               = SPI1;
	SpiHandle1.Init.Mode              = SPI_MODE_MASTER;
#if TEST_SPI_PLL2
	SpiHandle1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16; // <-----
#else
	SpiHandle1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
#endif
	SpiHandle1.Init.Direction         = SPI_DIRECTION_2LINES;
	SpiHandle1.Init.CLKPhase          = SPI_PHASE_1EDGE;  // CPHA = 0: Data captured on the rising edge
	SpiHandle1.Init.CLKPolarity       = SPI_POLARITY_LOW;  // CPOL = 0: Clock is low when idle
	SpiHandle1.Init.DataSize          = SPI_DATASIZE_8BIT;
	SpiHandle1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
	SpiHandle1.Init.TIMode            = SPI_TIMODE_DISABLE;
	SpiHandle1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
	SpiHandle1.Init.CRCPolynomial     = 7;
	SpiHandle1.Init.CRCLength         = SPI_CRC_LENGTH_8BIT;
	SpiHandle1.Init.NSS               = SPI_NSS_SOFT;
	SpiHandle1.Init.NSSPMode          = SPI_NSS_PULSE_DISABLE;
	SpiHandle1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;  /* Recommended setting to avoid glitches */

	if(HAL_SPI_Init(&SpiHandle1) != HAL_OK)
	{
		/* Initialization Error */
		Error_Handler();
		RetVal = 1U;
	}

	return RetVal;
}

/*******************************************************************************
 Read single 512 byte block
 token = 0xFE - Successful read
 token = 0x0X - Data error
 token = 0xFF - Timeout
*******************************************************************************/
uint8_t SD_Spi_readSingleBlock(uint32_t address, Spi_R1Response * pResponse)
{
	uint8_t RetVal;
	uint8_t readAttempts;
	uint8_t Crc1;
	uint8_t Crc2;
	Spi_R1Response resp;

	RetVal = 0U;

	Spi_CsEnable();

	SD_Spi_SendCommand(SD_SPI_CMD17, address);
	Spi_PollForResponse(&pResponse->byte);

	if(pResponse->byte != 0xFF)
	{
        // wait for a response token (timeout = 100ms)
        readAttempts = 0;
        while(++readAttempts != SD_MAX_READ_ATTEMPTS)
        {
        	Spi_PollForResponse(&pResponse->byte);
        	if(pResponse->byte != 0xFF)
			{
				break;
			}
        }

        // if response token is 0xFE
        if(pResponse->byte == SD_SPI_CMD_START_TOKEN)
        {
            // read 512 byte block
            for(uint16_t i = 0; i < SD_BLOCK_LENGTH; i++)
			{
            	Spi_readByte(&resp.byte);
				SPI_CMD_READ_BUFFER[i] = resp.byte;
			}

            // read 16-bit CRC
            Spi_readByte(&Crc1);
            Spi_readByte(&Crc2);

            if (0 != Crc1 && 0 != Crc2)
            {
            	RetVal = 0U;
            }
        }
        else
        {
        	RetVal = 1U;
        }
	}

	Spi_CsDisable();

	return RetVal;
}

/*
 * @brief Reads the File Allocation Table
 */
uint8_t SD_Spi_readFAT(Spi_R1Response * pResponse)
{
	uint8_t RetVal;

	RetVal = 0U;

	SD_Spi_readSingleBlock(0U, pResponse);

	return RetVal;
}

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  int32_t timeout;
  uint32_t spiClockSource;
  Spi_R1Response resp;

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

#if TEST_SPI_PLL2
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
#endif

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
   /* Set the SPI1 parameters */
  SpiHandle1.Instance               = SPI1;
  SpiHandle1.Init.Mode              = SPI_MODE_MASTER;
#if TEST_SPI_PLL2
  SpiHandle1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
#else
  SpiHandle1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
#endif
  SpiHandle1.Init.Direction         = SPI_DIRECTION_2LINES;
  SpiHandle1.Init.CLKPhase          = SPI_PHASE_1EDGE;  // CPHA = 0: Data captured on the rising edge
  SpiHandle1.Init.CLKPolarity       = SPI_POLARITY_LOW;  // CPOL = 0: Clock is low when idle
  SpiHandle1.Init.DataSize          = SPI_DATASIZE_8BIT;
  SpiHandle1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
  SpiHandle1.Init.TIMode            = SPI_TIMODE_DISABLE;
  SpiHandle1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
  SpiHandle1.Init.CRCPolynomial     = 7;
  SpiHandle1.Init.CRCLength         = SPI_CRC_LENGTH_8BIT;
  SpiHandle1.Init.NSS               = SPI_NSS_SOFT;
  SpiHandle1.Init.NSSPMode          = SPI_NSS_PULSE_DISABLE;
  SpiHandle1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;  /* Recommended setting to avoid glitches */

  if(HAL_SPI_Init(&SpiHandle1) != HAL_OK)
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
	SCB_CleanDCache_by_Addr ((uint32_t *)aTxBuffer, BUFFERSIZE);
#ifdef WAIT_FOR_USER_BUTTON
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

	/*##-2- Start the Full Duplex Communication process ########################*/
	/* While the SPI in TransmitReceive process, user can transmit data through
	   "aTxBuffer" buffer & receive data through "aRxBuffer" */


	while (0 != SD_Spi_Initialize(0));

	for (uint16_t h=0; h < SD_BLOCK_COUNT; h++)
	{
		SD_Spi_readSingleBlock(h, &resp);
	}

	/*##-3- Wait for the end of the transfer ###################################*/
	/*  Before starting a new communication transfer, you must wait the callback call
		to get the transfer complete confirmation or an error detection.
		For simplicity reasons, this example is just waiting till the end of the
		transfer, but application may perform other tasks while transfer operation
		is ongoing. */

	/* Invalidate cache prior to access by CPU */
	SCB_InvalidateDCache_by_Addr ((uint32_t *)aRxBuffer, BUFFERSIZE);

	switch(wTransferState)
	{
	  case TRANSFER_COMPLETE :
	/*##-4- Compare the sent and received
	 *  buffers ##############################*/
		if(Buffercmp((uint8_t*)aTxBuffer, (uint8_t*)aRxBuffer, BUFFERSIZE))
		{
		  /* Processing Error */
		  //Error_Handler();
		  BSP_LED_On(LED3);
		}
		else
		{
			BSP_LED_Off(LED3);
		}
		break;
	  default :
		Error_Handler();
		break;
	}
  }
}


uint8_t Spi_SendReceive()
{
	return 0;
}

uint8_t SD_Spi_PowerUp(void)
{
	Spi_CsDisable();
	Spi_SendReceiveMsg((uint8_t*)aTxSpiInit, (uint8_t *)aRxBuffer, COUNTOF(aTxSpiInit));
	return 0;
}

uint8_t SD_Spi_WaitTillIdle()
{
	uint8_t RetVal = 1;
	uint8_t counter;
	const uint8_t RetryCount = 10;

	Spi_CsEnable();

	// wait till card is idle
	do
	{
		Spi_SendReceiveMsg(aTxSpiDummy1, aRxBuffer, COUNTOF(aTxSpiDummy1));
	} while( 0xFF != aRxBuffer[0] && ( RetryCount > counter++) );

	Spi_CsDisable();

	if (0xFF == aRxBuffer[0])
	{
		RetVal = 1;
	}
	else
	{
		RetVal = 0;
	}
	return RetVal;
}

void SD_Spi_ReadBlock(uint32_t blockAddress, uint8_t *buffer) {
    uint8_t response;

    SD_Spi_CreateCommand(SD_SPI_CMD17, blockAddress, aTxSpiCmd);

    Spi_SendReceiveMsg((uint8_t *)aTxSpiCmd, (uint8_t *)aRxBuffer, COUNTOF(aTxSpiCmd));

    do {
    	Spi_PollForResponse(&response);
    } while (response != 0xFE);

    // Assert CS (low)
    HAL_GPIO_WritePin(SPI1_SS_GPIO_PORT, SPI1_SS_PIN, GPIO_PIN_RESET);

    // Read the data block (512 bytes)
    HAL_SPI_Receive_DMA(&SpiHandle1, buffer, 512);

    // Read the 2-byte CRC
    uint8_t crc[2];
    HAL_SPI_Receive_DMA(&SpiHandle1, crc, 2);

    // Deassert CS (high)
    HAL_GPIO_WritePin(SPI1_SS_GPIO_PORT, SPI1_SS_PIN, GPIO_PIN_SET);
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
  *            Flash Latency(WS)              = 4
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
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;

  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_1;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if(ret != HAL_OK)
  {
    Error_Handler();
  }

#if TEST_SPI_PLL2
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  PeriphClkInitStruct.Spi45ClockSelection = RCC_SPI45CLKSOURCE_PLL2;
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI1 | RCC_PERIPHCLK_SPI4;
  PeriphClkInitStruct.PLL2.PLL2M = 2;
  PeriphClkInitStruct.PLL2.PLL2N = 96;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 1;
  PeriphClkInitStruct.PLL2.PLL2P = 20;
  PeriphClkInitStruct.PLL2.PLL2Q = 20;
  PeriphClkInitStruct.PLL2.PLL2R = 2;

  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_2;

  ret = HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
  // Configure and enable PLL2
  if (HAL_OK != ret) {
      Error_Handler();  // Configuration failed
  }
#endif

/* Select PLL as system clock source and configure  bus clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 | \
                                 RCC_CLOCKTYPE_PCLK2  | RCC_CLOCKTYPE_D3PCLK1);

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
  if(ret != HAL_OK)
  {
    Error_Handler();
  }

}
/**
  * @brief  TxRx Transfer completed callback.
  * @param  hspi: SPI handle
  * @note   This example shows a simple way to report end of DMA TxRx transfer, and
  *         you can add your own implementation.
  * @retval None
  */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{

  /* Turn LED1 on: Transfer in transmission process is complete */
  BSP_LED_On(LED1);
  /* Turn LED2 on: Transfer in reception process is complete */
  BSP_LED_On(LED2);
  wTransferState = TRANSFER_COMPLETE;
}


/**
  * @brief  SPI error callbacks.
  * @param  hspi: SPI handle
  * @note   This example shows a simple way to report transfer error, and you can
  *         add your own implementation.
  * @retval None
  */
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
  wTransferState = TRANSFER_ERROR;
}


/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
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
static uint16_t Buffercmp(uint8_t* pBuffer1, uint8_t* pBuffer2, uint16_t BufferLength)
{
  while (BufferLength--)
  {
    if((*pBuffer1) != *pBuffer2)
    {
      return BufferLength;
    }
    pBuffer1++;
    pBuffer2++;
  }

  return 0;
}


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

