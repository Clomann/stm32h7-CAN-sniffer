#include "timer.h"
#include "nvic_irg_config.h"
#include "stm32h7xx_hal_tim.h"
#include <stdint.h>

#define BASE_CONSTANT (1000000ULL * 1000ULL * 10ULL)  // = 10_000_000_000

TIM_HandleTypeDef TimHandle;
TIM_HandleTypeDef TimHalHandle; /* timer used to feed HAL tick */

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

/**
 * @brief Compute the prescaler and ARR to achieve a target frequency.
 * 
 * @param timer_clk       Clock frequency of the timer in Hz.
 * @param target_freq_mHz Target frequency in milli-Hertz (e.g., 15243 = 15.243 Hz).
 * @param max_arr         Maximum value for ARR (typically 0xFFFF for 16-bit timers).
 * @param prescaler_out   Pointer to store the computed prescaler value.
 * @param arr_out         Pointer to store the computed ARR value.
 * 
 * @return uint8_t        0 if successful, 1 if no suitable values were found.
 */
uint8_t ComputePrescalerAndARR(
    uint32_t timer_clk, 
    uint64_t target_freq, 
    uint32_t max_arr,
    uint32_t *prescaler_out, 
    uint32_t *arr_out)
{
    uint8_t res;
    uint32_t prescaler = 0;
    uint32_t arr = 0;
    volatile uint64_t total_counts = 0U;

    res = 1;

    if (target_freq == 0)
        return res;  // Avoid division by zero

    // Upper bound on product (PSC+1)*(ARR+1)
    total_counts = ((uint64_t)timer_clk * 1000ULL) / target_freq;

    for (arr = max_arr; arr > 0; --arr)
    {
        uint64_t divisor = (uint64_t)arr + 1;
        
        if (total_counts % divisor != 0)
            continue;
    
        prescaler = (uint32_t)((total_counts / divisor) - 1);
    
        if ( ((uint64_t)prescaler + 1U) == 0 || ((uint64_t)prescaler + 1U) > 0x10000)
            continue;

        if (prescaler_out) *prescaler_out = prescaler;
        if (arr_out) *arr_out = arr;
        return 0;  // Exact match found
    }

    if (arr_out) *arr_out = arr;
    if (prescaler_out) *prescaler_out = prescaler;

    return res;
}

uint8_t TIMx_Init(uint32_t resolution)
{
    uint8_t res = 0;
    uint32_t InputClock;
    uint32_t MaxArr;
    uint32_t Arr;
    uint32_t Prescaler;
    volatile uint64_t freq;
    volatile uint64_t freq_int;

    InputClock = GetTimerInputClock(TIMx);
    MaxArr = GetTimerMaxARR(TIMx);

    Arr = MaxArr;
    while (Arr > 0)
    {
        // find lowest frequency that yields the desired resolution
        freq_int = BASE_CONSTANT / (resolution * (Arr + 1));
        if (BASE_CONSTANT == freq_int * resolution * (Arr + 1)) {
            freq = freq_int / 10;
            break;
        }
        else
        {
            Arr--;
        }
    }

    if (0 == Arr) 
    {
        TIM_ErrorHandlerHook();
    }

    res = ComputePrescalerAndARR(
            InputClock, 
            freq,
            MaxArr,
            &Prescaler, 
            &Arr);

    if (0 != res) return res;
    
    if (resolution != (InputClock/ (1+Prescaler)) / 1000000U)
    {
        TIM_ErrorHandlerHook();
    }

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

    __HAL_TIM_UIFREMAP_ENABLE(&TimHandle);

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
    if (TIMx == htim->Instance)
    {
        /* TIMx Peripheral clock enable */
        TIMx_CLK_ENABLE();

        /*##-2- Configure the NVIC for TIMx ########################################*/
        /* Set the TIMx priority */
        HAL_NVIC_SetPriority(TIMx_IRQn, TIMx_IRQ_PREEMPT_PRIO, 0);

        /* Enable the TIMx global Interrupt */
        HAL_NVIC_EnableIRQ(TIMx_IRQn);
    }
    else if (TIM_HAL == htim->Instance)
    {
        TIM_HAL_CLK_ENABLE();

        HAL_NVIC_SetPriority(TIM_HAL_IRQn, TIMx_IRQ_PREEMPT_PRIO, 1);

        HAL_NVIC_EnableIRQ(TIM_HAL_IRQn);
    }
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (TIMx == htim->Instance)
    {
        TIM_InterruptCallback();
    }
    else if (TIM_HAL == htim->Instance)
    {
        TIM_HAL_InterruptCallback();
    }
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

void TIM_GetCounterValueAndUpdateInterruptFlag(uint32_t *cnt, uint32_t *uifcpy)
{
    uint32_t CntRaw;
    
    *uifcpy = 0;

    CntRaw = __HAL_TIM_GetCounter(&TimHandle);
    
    if (TimHandle.Instance->CR1 & TIM_CR1_UIFREMAP)
    {
        *uifcpy = __HAL_TIM_GET_UIFCPY(CntRaw);
        CntRaw &= ~TIM_CNT_UIFCPY;
    }

    *cnt = CntRaw;
}

void TIM_GetArrValue(uint16_t *arr)
{
    *arr = __HAL_TIM_GetAutoreload(&TimHandle);
}

uint8_t TIM_HAL_Init(uint32_t freq)
{
    uint8_t res = 0;
    uint32_t InputClock;
    uint32_t MaxArr;
    uint32_t Arr;
    uint32_t Prescaler;

    InputClock = GetTimerInputClock(TIM_HAL);
    MaxArr = GetTimerMaxARR(TIM_HAL);

    res = ComputePrescalerAndARR(
            InputClock, 
            freq,
            MaxArr,
            &Prescaler, 
            &Arr);

    if (0 != res) return res;

    /* Set TIM instance */
    TimHalHandle.Instance = TIM_HAL;

    TimHalHandle.Init.Period            = Arr;
    TimHalHandle.Init.Prescaler         = Prescaler;
    TimHalHandle.Init.ClockDivision     = 0;
    TimHalHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    TimHalHandle.Init.RepetitionCounter = 0;

    if (HAL_TIM_Base_Init(&TimHalHandle) != HAL_OK)
    {
        /* Initialization Error */
        res = 1;
    }

    /*##-2- Start the TIM Base generation in interrupt mode ####################*/
    /* Start Channel1 */
    if (HAL_TIM_Base_Start_IT(&TimHalHandle) != HAL_OK)
    {
        /* Starting Error */
        res = 2;
    }

    return res;
}

void TIM_HAL_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&TimHalHandle);
}

void __attribute__((weak)) TIM_ErrorHandlerHook(void)
{
    ;
}
