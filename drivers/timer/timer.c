#include "timer.h"

TIM_HandleTypeDef TimHandle;

uint32_t GetTimerMaxARR(TIM_TypeDef *htim)
{
    if (htim == TIM2 || htim == TIM5)
        return 0xFFFFFFFF;  // 32-bit timers
    else
        return 0xFFFF;      // 16-bit timers
}

uint32_t GetTimerInputClock(TIM_TypeDef *htim)
{
    RCC_ClkInitTypeDef clkconfig;
    uint32_t flash_latency;
    uint32_t pclk;
    uint32_t timer_clk;

    // Get current clock configuration
    HAL_RCC_GetClockConfig(&clkconfig, &flash_latency);

    if (htim == TIM1 || htim == TIM8 ||
        htim == TIM15 || htim == TIM16 || htim == TIM17)
    {
        // APB2 timer
        pclk = HAL_RCC_GetPCLK2Freq();

        // Check if APB2 prescaler is > 1
        timer_clk = (clkconfig.APB2CLKDivider != RCC_HCLK_DIV1) ? pclk * 2 : pclk;
    }
    else
    {
        // APB1 timer
        pclk = HAL_RCC_GetPCLK1Freq();

        // Check if APB1 prescaler is > 1
        timer_clk = (clkconfig.APB1CLKDivider != RCC_HCLK_DIV1) ? pclk * 2 : pclk;
    }

    return timer_clk;
}

uint8_t ComputePrescalerAndARR(uint32_t timer_clk, uint32_t target_freq, uint32_t max_arr,
    uint32_t *prescaler_out, uint32_t *arr_out)
{
    uint8_t res;
    uint32_t prescaler = 0;
    uint32_t arr = 0;
    uint64_t total_counts = 0U;

    res = 1;

    // Upper bound on product (PSC+1)*(ARR+1)
    total_counts = (uint64_t)timer_clk / target_freq;
    (volatile) total_counts;


    for (arr = max_arr; arr > 0; --arr)
    {
        uint64_t divisor = (uint64_t)arr + 1;
        if (total_counts < divisor)
            continue;
    
        prescaler = (uint32_t)((total_counts / divisor) - 1);
    
        if (((uint64_t)(prescaler + 1)) * divisor <= total_counts)
        {
            res = 0;
            break;
        }
    }

    if (arr_out) *arr_out = arr;
    if (prescaler_out) *prescaler_out = prescaler;

    return res;
}

uint8_t TIMx_Init(uint32_t freq)
{
    uint8_t res = 0;
    uint32_t InputClock;
    uint32_t MaxArr;
    uint32_t Arr;
    uint32_t Prescaler;

    InputClock = GetTimerInputClock(TIMx);
    MaxArr = GetTimerMaxARR(TIMx);

    res = ComputePrescalerAndARR(
            InputClock, 
            freq,
            MaxArr,
            &Prescaler, 
            &Arr);

    if (0 != res) return res;

    /* Set TIMx instance */
    TimHandle.Instance = TIMx;

    TimHandle.Init.Period            = Arr;
    TimHandle.Init.Prescaler         = Prescaler;
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