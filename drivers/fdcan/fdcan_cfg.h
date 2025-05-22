/*
 * fdcan_cfg.h
 *
 *  Created on: 14.09.2024
 *      Author: Clemens
 */

/* User can use this section to tailor FDCANx instance used and associated
   resources */
/* Definition for FDCANx clock resources */
#define FDCANx                      FDCAN1
#define FDCANx_CLK_ENABLE()         __HAL_RCC_FDCAN_CLK_ENABLE()
#define FDCANx_RX_GPIO_CLK_ENABLE() __HAL_RCC_GPIOH_CLK_ENABLE()
#define FDCANx_TX_GPIO_CLK_ENABLE() __HAL_RCC_GPIOH_CLK_ENABLE()

#define FDCANx_FORCE_RESET()   __HAL_RCC_FDCAN_FORCE_RESET()
#define FDCANx_RELEASE_RESET() __HAL_RCC_FDCAN_RELEASE_RESET()

/*!< timestamp resolution in micro seconds */
#define FDCANx_TIMESTAMP_RESOLUTION     (1U)

/* Definition for FDCANx Pins */
#define FDCANx_TX_PIN       GPIO_PIN_9
#define FDCANx_TX_GPIO_PORT GPIOB
#define FDCANx_TX_AF        GPIO_AF9_FDCAN1
#define FDCANx_RX_PIN       GPIO_PIN_8
#define FDCANx_RX_GPIO_PORT GPIOB
#define FDCANx_RX_AF        GPIO_AF9_FDCAN1

/* Definition for FDCANx's NVIC IRQ and IRQ Handlers */
#define FDCANx_IRQn       FDCAN1_IT0_IRQn
#define FDCANx_IRQHandler FDCAN1_IT0_IRQHandler

#define FDCANx_MAX_MSG_COUNT 10u
