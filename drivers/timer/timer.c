#include "timer.h"

TIM_HandleTypeDef TimHandle;

uint8_t TIMx_Init(uint32_t prescaler)
{
    uint8_t res = 0;

    /* Set TIMx instance */
    TimHandle.Instance = TIMx;

    /* Initialize TIMx peripheral as follows:
        + Period = 10000 - 1
        + Prescaler = (SystemCoreClock/10000) - 1
        + ClockDivision = 0
        + Counter direction = Up
    */
    TimHandle.Init.Period            = 10000 - 1;
    TimHandle.Init.Prescaler         = prescaler;
    TimHandle.Init.ClockDivision     = 0;
    TimHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    TimHandle.Init.RepetitionCounter = 0;

    if (HAL_TIM_Base_Init(&TimHandle) != HAL_OK)
    {
        /* Initialization Error */
        res = 1;
    }

    /*##-2- Start the TIM Base generation in interrupt mode ####################*/
    /* Start Channel1 */
    if (HAL_TIM_Base_Start_IT(&TimHandle) != HAL_OK)
    {
        /* Starting Error */
        res = 2;
    }

    return res;
}

/**
  * @brief TIM MSP Initialization
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  * @param htim: TIM handle pointer
  * @retval None
  */
 void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
 {
   /*##-1- Enable peripheral clock #################################*/
   /* TIMx Peripheral clock enable */
   TIMx_CLK_ENABLE();
   
   /*##-2- Configure the NVIC for TIMx ########################################*/
   /* Set the TIMx priority */
   HAL_NVIC_SetPriority(TIMx_IRQn, 3, 0);
 
   /* Enable the TIMx global Interrupt */
   HAL_NVIC_EnableIRQ(TIMx_IRQn);
 }

/**
  * @brief  Period elapsed callback in non blocking mode
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    TIM_InterruptCallback();
}

 /**
  * @brief  This function handles TIM interrupt request.
  * @param  None
  * @retval None
  */
 void TIMx_IRQHandler(void)
 {
   HAL_TIM_IRQHandler(&TimHandle);
 }