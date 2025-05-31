/*
 * fdcan_cfg.h
 *
 *  Created on: 14.09.2024
 *      Author: Clemens
 */


#define FDCAN_MAX_INSTANCES 2U

/* User can use this section to tailor FDCANx instance used and associated
   resources */

/* Definition for FDCANx clock resources */
#define FDCANx_CLK_ENABLE()     __HAL_RCC_FDCAN_CLK_ENABLE()
#define FDCANx_FORCE_RESET()    __HAL_RCC_FDCAN_FORCE_RESET()
#define FDCANx_RELEASE_RESET()  __HAL_RCC_FDCAN_RELEASE_RESET()


/* FDCAN 1 ========================================= */
#define FDCAN_1             FDCAN1

#define FDCAN1_RX_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()
#define FDCAN1_TX_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()

/*!< timestamp resolution in micro seconds */
#define FDCAN1_TIMESTAMP_RESOLUTION     (1U)

/* Definition for FDCAN1 Pins */
#define FDCAN1_TX_PIN       GPIO_PIN_9
#define FDCAN1_TX_GPIO_PORT GPIOB
#define FDCAN1_TX_AF        GPIO_AF9_FDCAN1
#define FDCAN1_RX_PIN       GPIO_PIN_8
#define FDCAN1_RX_GPIO_PORT GPIOB
#define FDCAN1_RX_AF        GPIO_AF9_FDCAN1

/* Definition for FDCAN1's NVIC IRQ and IRQ Handlers */
#define FDCAN1_IRQn       FDCAN1_IT0_IRQn
#define FDCAN_1_IRQHandler FDCAN1_IT0_IRQHandler

#define FDCAN1_MAX_MSG_COUNT 10u

/* FDCAN 2 ========================================= */
#define FDCAN_2             FDCAN2

#define FDCAN2_RX_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()
#define FDCAN2_TX_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()

/*!< timestamp resolution in micro seconds */
#define FDCAN2_TIMESTAMP_RESOLUTION     (1U)

/* Definition for FDCAN2 Pins */
#define FDCAN2_TX_PIN       GPIO_PIN_6
#define FDCAN2_TX_GPIO_PORT GPIOB
#define FDCAN2_TX_AF        GPIO_AF9_FDCAN2
#define FDCAN2_RX_PIN       GPIO_PIN_12
#define FDCAN2_RX_GPIO_PORT GPIOB
#define FDCAN2_RX_AF        GPIO_AF9_FDCAN2

/* Definition for FDCAN2's NVIC IRQ and IRQ Handlers */
#define FDCAN2_IRQn       FDCAN2_IT0_IRQn
#define FDCAN_2_IRQHandler FDCAN2_IT0_IRQHandler

#define FDCAN2_MAX_MSG_COUNT 10u
