#include <stdint.h>

#include "stm32h7xx.h"

__attribute__((noreturn)) void Default_Handler(void)
{
    while (1)
    {
    }
}

__attribute__((weak)) void NMI_Handler(void)
{
    Default_Handler();
}
__attribute__((weak)) void HardFault_Handler(void)
{
    Default_Handler();
}
__attribute__((weak)) void MemManage_Handler(void)
{
    Default_Handler();
}
__attribute__((weak)) void BusFault_Handler(void)
{
    Default_Handler();
}
__attribute__((weak)) void UsageFault_Handler(void)
{
    Default_Handler();
}
__attribute__((weak)) void SVC_Handler(void)
{
    Default_Handler();
}
__attribute__((weak)) void DebugMon_Handler(void)
{
    Default_Handler();
}
__attribute__((weak)) void PendSV_Handler(void)
{
    Default_Handler();
}
__attribute__((weak)) void SysTick_Handler(void)
{
    Default_Handler();
}
__attribute__((weak)) void WWDG_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void PVD_AVD_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void TAMP_STAMP_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void RTC_WKUP_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void FLASH_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void RCC_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void EXTI0_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void EXTI1_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void EXTI2_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void EXTI3_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void EXTI4_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void DMA1_Stream0_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void DMA1_Stream1_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void DMA1_Stream2_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void DMA1_Stream3_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void DMA1_Stream4_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void DMA1_Stream5_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void DMA1_Stream6_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void ADC_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void FDCAN1_IT0_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void FDCAN2_IT0_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void FDCAN1_IT1_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void FDCAN2_IT1_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void EXTI9_5_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void TIM1_BRK_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void TIM1_UP_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void TIM1_TRG_COM_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void TIM1_CC_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void TIM2_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void TIM3_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void TIM4_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void I2C1_EV_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void I2C1_ER_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void I2C2_EV_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void I2C2_ER_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void SPI1_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void SPI2_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void USART1_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void USART2_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void USART3_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void EXTI15_10_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void RTC_Alarm_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void TIM8_BRK_TIM12_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void TIM8_UP_TIM13_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void TIM8_TRG_COM_TIM14_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void TIM8_CC_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void DMA1_Stream7_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void FMC_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void SDMMC1_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void TIM5_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void SPI3_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void UART4_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void UART5_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void TIM6_DAC_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void TIM7_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void DMA2_Stream0_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void DMA2_Stream1_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void DMA2_Stream2_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void DMA2_Stream3_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void DMA2_Stream4_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void ETH_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void ETH_WKUP_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void FDCAN_CAL_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void CM7_SEV_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void CM4_SEV_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void DMA2_Stream5_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void DMA2_Stream6_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void DMA2_Stream7_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void USART6_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void I2C3_EV_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void I2C3_ER_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void OTG_HS_EP1_OUT_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void OTG_HS_EP1_IN_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void OTG_HS_WKUP_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void OTG_HS_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void DCMI_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void RNG_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void FPU_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void UART7_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void UART8_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void SPI4_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void SPI5_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void SPI6_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void SAI1_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void LTDC_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void LTDC_ER_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void DMA2D_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void SAI2_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void QUADSPI_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void LPTIM1_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void CEC_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void I2C4_EV_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void I2C4_ER_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void SPDIF_RX_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void OTG_FS_EP1_OUT_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void OTG_FS_EP1_IN_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void OTG_FS_WKUP_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void OTG_FS_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void DMAMUX1_OVR_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void HRTIM1_Master_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void HRTIM1_TIMA_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void HRTIM1_TIMB_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void HRTIM1_TIMC_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void HRTIM1_TIMD_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void HRTIM1_TIME_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void HRTIM1_FLT_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void DFSDM1_FLT0_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void DFSDM1_FLT1_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void DFSDM1_FLT2_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void DFSDM1_FLT3_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void SAI3_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void SWPMI1_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void TIM15_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void TIM16_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void TIM17_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void MDIOS_WKUP_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void MDIOS_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void JPEG_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void MDMA_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void SDMMC2_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void HSEM1_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void HSEM2_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void ADC3_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void DMAMUX2_OVR_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void BDMA_Channel0_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void BDMA_Channel1_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void BDMA_Channel2_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void BDMA_Channel3_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void BDMA_Channel4_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void BDMA_Channel5_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void BDMA_Channel6_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void BDMA_Channel7_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void COMP1_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void LPTIM2_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void LPTIM3_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void LPTIM4_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void LPTIM5_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void LPUART1_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void WWDG_RST_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void CRS_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void ECC_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void SAI4_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void HOLD_CORE_IRQHandler(void)
{
    Default_Handler();
}
__attribute__((weak)) void WAKEUP_PIN_IRQHandler(void)
{
    Default_Handler();
}
