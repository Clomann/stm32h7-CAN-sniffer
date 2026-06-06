/**
  ******************************************************************************
  * @file    SPI/SPI_FullDuplex_ComDMA/CM7/Src/stm32h7xx_it.c
  * @author  MCD Application Team
  * @brief   Main Interrupt Service Routines.
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
#include "stm32h7xx_it.h"

/** @addtogroup STM32H7xx_HAL_Examples
  * @{
  */

/** @addtogroup SPI_FullDuplex_ComDMA
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* SPI handler declared in "main.c" file */
extern ETH_HandleTypeDef heth;

typedef struct
{
    volatile uint32_t valid;
    volatile uint32_t stacked_sp;
    volatile uint32_t exc_return;
    volatile uint32_t msp;
    volatile uint32_t psp;
    volatile uint32_t cfsr;
    volatile uint32_t hfsr;
    volatile uint32_t dfsr;
    volatile uint32_t afsr;
    volatile uint32_t mmfar;
    volatile uint32_t bfar;
    volatile uint32_t shcsr;
    volatile uint32_t icsr;
    volatile uint32_t r0;
    volatile uint32_t r1;
    volatile uint32_t r2;
    volatile uint32_t r3;
    volatile uint32_t r12;
    volatile uint32_t lr;
    volatile uint32_t pc;
    volatile uint32_t xpsr;
} Cm7MemFaultContextType;

volatile Cm7MemFaultContextType g_cm7MemFaultContext = {0U};
volatile Cm7MemFaultContextType g_cm7HardFaultContext = {0U};

/* Private function prototypes -----------------------------------------------*/
void SPI1_IRQHandler(void);
void ETH_IRQHandler(void);
void HardFault_HandlerC(uint32_t *stacked_sp, uint32_t exc_return);
void MemManage_HandlerC(uint32_t *stacked_sp, uint32_t exc_return);

/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M7 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief   This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
__attribute__((naked)) void HardFault_Handler(void)
{
    __asm volatile(
        "tst lr, #4 \n"
        "ite eq \n"
        "mrseq r0, msp \n"
        "mrsne r0, psp \n"
        "mov r1, lr \n"
        "b HardFault_HandlerC \n"
    );
}

void HardFault_HandlerC(uint32_t *stacked_sp, uint32_t exc_return)
{
    g_cm7HardFaultContext.valid      = 0U;
    g_cm7HardFaultContext.stacked_sp = (uint32_t)stacked_sp;
    g_cm7HardFaultContext.exc_return = exc_return;
    g_cm7HardFaultContext.msp        = __get_MSP();
    g_cm7HardFaultContext.psp        = __get_PSP();
    g_cm7HardFaultContext.cfsr       = SCB->CFSR;
    g_cm7HardFaultContext.hfsr       = SCB->HFSR;
    g_cm7HardFaultContext.dfsr       = SCB->DFSR;
    g_cm7HardFaultContext.afsr       = SCB->AFSR;
    g_cm7HardFaultContext.mmfar      = SCB->MMFAR;
    g_cm7HardFaultContext.bfar       = SCB->BFAR;
    g_cm7HardFaultContext.shcsr      = SCB->SHCSR;
    g_cm7HardFaultContext.icsr       = SCB->ICSR;

    if (stacked_sp != NULL)
    {
        g_cm7HardFaultContext.r0   = stacked_sp[0];
        g_cm7HardFaultContext.r1   = stacked_sp[1];
        g_cm7HardFaultContext.r2   = stacked_sp[2];
        g_cm7HardFaultContext.r3   = stacked_sp[3];
        g_cm7HardFaultContext.r12  = stacked_sp[4];
        g_cm7HardFaultContext.lr   = stacked_sp[5];
        g_cm7HardFaultContext.pc   = stacked_sp[6];
        g_cm7HardFaultContext.xpsr = stacked_sp[7];
    }
    else
    {
        g_cm7HardFaultContext.r0   = 0U;
        g_cm7HardFaultContext.r1   = 0U;
        g_cm7HardFaultContext.r2   = 0U;
        g_cm7HardFaultContext.r3   = 0U;
        g_cm7HardFaultContext.r12  = 0U;
        g_cm7HardFaultContext.lr   = 0U;
        g_cm7HardFaultContext.pc   = 0U;
        g_cm7HardFaultContext.xpsr = 0U;
    }

    g_cm7HardFaultContext.valid = 1U;

    __DSB();
    __ISB();

    while (1)
    {
        __NOP();
    }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
__attribute__((naked)) void MemManage_Handler(void)
{
    __asm volatile(
        "tst lr, #4 \n"
        "ite eq \n"
        "mrseq r0, msp \n"
        "mrsne r0, psp \n"
        "mov r1, lr \n"
        "b MemManage_HandlerC \n"
    );
}

void MemManage_HandlerC(uint32_t *stacked_sp, uint32_t exc_return)
{
    g_cm7MemFaultContext.valid      = 0U;
    g_cm7MemFaultContext.stacked_sp = (uint32_t)stacked_sp;
    g_cm7MemFaultContext.exc_return = exc_return;
    g_cm7MemFaultContext.msp        = __get_MSP();
    g_cm7MemFaultContext.psp        = __get_PSP();
    g_cm7MemFaultContext.cfsr       = SCB->CFSR;
    g_cm7MemFaultContext.hfsr       = SCB->HFSR;
    g_cm7MemFaultContext.dfsr       = SCB->DFSR;
    g_cm7MemFaultContext.afsr       = SCB->AFSR;
    g_cm7MemFaultContext.mmfar      = SCB->MMFAR;
    g_cm7MemFaultContext.bfar       = SCB->BFAR;
    g_cm7MemFaultContext.shcsr      = SCB->SHCSR;
    g_cm7MemFaultContext.icsr       = SCB->ICSR;

    if (stacked_sp != NULL)
    {
        g_cm7MemFaultContext.r0   = stacked_sp[0];
        g_cm7MemFaultContext.r1   = stacked_sp[1];
        g_cm7MemFaultContext.r2   = stacked_sp[2];
        g_cm7MemFaultContext.r3   = stacked_sp[3];
        g_cm7MemFaultContext.r12  = stacked_sp[4];
        g_cm7MemFaultContext.lr   = stacked_sp[5];
        g_cm7MemFaultContext.pc   = stacked_sp[6];
        g_cm7MemFaultContext.xpsr = stacked_sp[7];
    }
    else
    {
        g_cm7MemFaultContext.r0   = 0U;
        g_cm7MemFaultContext.r1   = 0U;
        g_cm7MemFaultContext.r2   = 0U;
        g_cm7MemFaultContext.r3   = 0U;
        g_cm7MemFaultContext.r12  = 0U;
        g_cm7MemFaultContext.lr   = 0U;
        g_cm7MemFaultContext.pc   = 0U;
        g_cm7MemFaultContext.xpsr = 0U;
    }

    g_cm7MemFaultContext.valid = 1U;

    __DSB();
    __ISB();

    while (1)
    {
        __NOP();
    }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
    /* Go to infinite loop when Bus Fault exception occurs */
    while (1)
    {
    }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
    /* Go to infinite loop when Usage Fault exception occurs */
    while (1)
    {
    }
}

#if !defined(vPortSVCHandler) && (vPortSVCHandler != SVC_Handler)

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

#endif

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

#if !defined(xPortPendSVHandler) && (xPortPendSVHandler != PendSV_Handler)

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

#endif

#if !defined(xPortSysTickHandler) && (xPortSysTickHandler != SysTick_Handler)

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

#endif

/******************************************************************************/
/*                 STM32H7xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32h7xx.s).                                               */
/******************************************************************************/

/**
  * @brief This function handles Ethernet global interrupt.
  */
void ETH_IRQHandler(void)
{
    /* USER CODE BEGIN ETH_IRQn 0 */

    /* USER CODE END ETH_IRQn 0 */
    HAL_ETH_IRQHandler(&heth);
    /* USER CODE BEGIN ETH_IRQn 1 */

    /* USER CODE END ETH_IRQn 1 */
}

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */

/**
  * @}
  */
