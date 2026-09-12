#include "irq_handlers.h"

#define WEAK_IRQ_HANDLER __attribute__((weak))

void Default_Handler(void)
{
    while (1)
    {
    }
}

WEAK_IRQ_HANDLER void NMI_Handler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void HardFault_Handler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void MemManage_Handler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void BusFault_Handler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void UsageFault_Handler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void SVC_Handler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void DebugMon_Handler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void PendSV_Handler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void SysTick_Handler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void WWDG_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void PVD_AVD_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void TAMP_STAMP_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void RTC_WKUP_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void FLASH_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void RCC_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void EXTI0_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void EXTI1_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void EXTI2_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void EXTI3_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void EXTI4_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void DMA1_Stream0_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void DMA1_Stream1_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void DMA1_Stream2_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void DMA1_Stream3_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void DMA1_Stream4_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void DMA1_Stream5_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void DMA1_Stream6_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void ADC_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void FDCAN1_IT0_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void FDCAN2_IT0_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void FDCAN1_IT1_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void FDCAN2_IT1_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void EXTI9_5_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void TIM1_BRK_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void TIM1_UP_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void TIM1_TRG_COM_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void TIM1_CC_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void TIM2_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void TIM3_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void TIM4_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void I2C1_EV_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void I2C1_ER_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void I2C2_EV_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void I2C2_ER_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void SPI1_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void SPI2_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void USART1_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void USART2_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void USART3_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void EXTI15_10_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void RTC_Alarm_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void TIM8_BRK_TIM12_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void TIM8_UP_TIM13_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void TIM8_TRG_COM_TIM14_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void TIM8_CC_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void DMA1_Stream7_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void FMC_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void SDMMC1_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void TIM5_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void SPI3_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void UART4_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void UART5_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void TIM6_DAC_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void TIM7_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void DMA2_Stream0_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void DMA2_Stream1_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void DMA2_Stream2_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void DMA2_Stream3_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void DMA2_Stream4_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void ETH_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void ETH_WKUP_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void FDCAN_CAL_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void CM7_SEV_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void CM4_SEV_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void DMA2_Stream5_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void DMA2_Stream6_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void DMA2_Stream7_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void USART6_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void I2C3_EV_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void I2C3_ER_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void OTG_HS_EP1_OUT_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void OTG_HS_EP1_IN_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void OTG_HS_WKUP_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void OTG_HS_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void DCMI_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void RNG_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void FPU_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void UART7_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void UART8_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void SPI4_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void SPI5_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void SPI6_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void SAI1_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void LTDC_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void LTDC_ER_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void DMA2D_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void SAI2_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void QUADSPI_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void LPTIM1_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void CEC_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void I2C4_EV_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void I2C4_ER_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void SPDIF_RX_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void OTG_FS_EP1_OUT_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void OTG_FS_EP1_IN_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void OTG_FS_WKUP_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void OTG_FS_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void DMAMUX1_OVR_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void HRTIM1_Master_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void HRTIM1_TIMA_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void HRTIM1_TIMB_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void HRTIM1_TIMC_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void HRTIM1_TIMD_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void HRTIM1_TIME_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void HRTIM1_FLT_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void DFSDM1_FLT0_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void DFSDM1_FLT1_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void DFSDM1_FLT2_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void DFSDM1_FLT3_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void SAI3_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void SWPMI1_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void TIM15_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void TIM16_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void TIM17_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void MDIOS_WKUP_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void MDIOS_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void JPEG_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void MDMA_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void SDMMC2_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void HSEM1_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void HSEM2_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void ADC3_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void DMAMUX2_OVR_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void BDMA_Channel0_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void BDMA_Channel1_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void BDMA_Channel2_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void BDMA_Channel3_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void BDMA_Channel4_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void BDMA_Channel5_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void BDMA_Channel6_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void BDMA_Channel7_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void COMP1_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void LPTIM2_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void LPTIM3_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void LPTIM4_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void LPTIM5_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void LPUART1_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void WWDG_RST_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void CRS_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void ECC_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void SAI4_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void HOLD_CORE_IRQHandler(void)
{
    Default_Handler();
}

WEAK_IRQ_HANDLER void WAKEUP_PIN_IRQHandler(void)
{
    Default_Handler();
}
