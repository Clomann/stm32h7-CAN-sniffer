/*
 * fdcan_cfg.h
 *
 *  Created on: 14.09.2024
 *      Author: Clemens
 */
#pragma once

#define FDCAN_MAX_INSTANCES 2U

#define FDCAN_MESSAGE_RAM_SIZE 0x2800U /* check stm32h7xx_hal_fdcan.c */
#define FDCAN_RAM_RX_SIZE      (FDCAN_MESSAGE_RAM_SIZE - 4UL)
#define FDCAN_RAM_RX_SECTION_SIZE                                              \
    (FDCAN_RAM_RX_SIZE / FDCAN_MAX_INSTANCES / 4UL                             \
    ) /*!< RAM available for each driver to store rx frames */
#define FDCAN_RAM_RX_ELEMENTS (64U)
#define FDCAN_RAM_TX_ELEMENTS (4U)

#ifndef CAN_STATIC_TX_REPLAY_ENABLE
/** @brief Enable the compile-time FDCAN static Tx replay load generator.
 * Dedicated Tx buffers are preloaded once and only re-armed while running. */
#define CAN_STATIC_TX_REPLAY_ENABLE 0U
#endif

#ifndef CAN_STATIC_TX_REPLAY_TX_BUFFERS
/** @brief Number of dedicated hardware Tx buffers preloaded for replay.
 * This is the static frame count per FDCAN instance; STM32H7 supports up to 32. */
#define CAN_STATIC_TX_REPLAY_TX_BUFFERS 32U
#endif

#ifndef CAN_STATIC_TX_REPLAY_ID_COUNT
/** @brief Standard-ID wrap range used only while preloading static buffers.
 * This does not create a runtime sequence or more stored frames than Tx buffers. */
#define CAN_STATIC_TX_REPLAY_ID_COUNT 0xFFU
#endif

#ifndef CAN_STATIC_TX_REPLAY_CHANNEL_MASK
/** @brief Select FDCAN instances used for static replay.
 * Bit 0 enables CAN1 replay and bit 1 enables CAN2 replay. */
#define CAN_STATIC_TX_REPLAY_CHANNEL_MASK 0x3U
#endif

#ifndef CAN_STATIC_TX_REPLAY_CAN1_BASE_ID
/** @brief First standard ID used when preloading CAN1 static replay buffers. */
#define CAN_STATIC_TX_REPLAY_CAN1_BASE_ID 0x000U
#endif

#ifndef CAN_STATIC_TX_REPLAY_CAN2_BASE_ID
/** @brief First standard ID used when preloading CAN2 static replay buffers. */
#define CAN_STATIC_TX_REPLAY_CAN2_BASE_ID 0x000U
#endif

#ifndef CAN_STATIC_TX_REPLAY_FDCAN_MODE
/** @brief HAL FDCAN mode used while static replay is enabled.
 * Intended values are FDCAN_MODE_EXTERNAL_LOOPBACK and FDCAN_MODE_NORMAL. */
#define CAN_STATIC_TX_REPLAY_FDCAN_MODE FDCAN_MODE_EXTERNAL_LOOPBACK
#endif

#if CAN_STATIC_TX_REPLAY_TX_BUFFERS > 32U
#error "CAN_STATIC_TX_REPLAY_TX_BUFFERS must be <= 32"
#endif

#if CAN_STATIC_TX_REPLAY_ID_COUNT > 0x800U
#error                                                                         \
    "CAN_STATIC_TX_REPLAY_ID_COUNT must fit into the 11-bit standard ID space"
#endif

#if CAN_STATIC_TX_REPLAY_ENABLE                                                \
    && (CAN_STATIC_TX_REPLAY_ID_COUNT < CAN_STATIC_TX_REPLAY_TX_BUFFERS)
#error "CAN_STATIC_TX_REPLAY_ID_COUNT must cover all static replay Tx buffers"
#endif

#if CAN_STATIC_TX_REPLAY_ENABLE                                                \
    && ((CAN_STATIC_TX_REPLAY_CAN1_BASE_ID + CAN_STATIC_TX_REPLAY_TX_BUFFERS)  \
        > 0x800U)
#error "CAN_STATIC_TX_REPLAY_CAN1_BASE_ID must not wrap the static replay batch"
#endif

#if CAN_STATIC_TX_REPLAY_ENABLE                                                \
    && ((CAN_STATIC_TX_REPLAY_CAN2_BASE_ID + CAN_STATIC_TX_REPLAY_TX_BUFFERS)  \
        > 0x800U)
#error "CAN_STATIC_TX_REPLAY_CAN2_BASE_ID must not wrap the static replay batch"
#endif

#define FDCAN_IRQ_NOTIFICATION                                                 \
    (FDCAN_IT_RX_FIFO0_WATERMARK | FDCAN_IT_RX_FIFO0_MESSAGE_LOST)
#define FDCAN_IRQ_RX_WATERMARK (54U)

/* User can use this section to tailor FDCANx instance used and associated
   resources */

/* Definition for FDCANx clock resources */
#define FDCANx_CLK_ENABLE()    __HAL_RCC_FDCAN_CLK_ENABLE()
#define FDCANx_FORCE_RESET()   __HAL_RCC_FDCAN_FORCE_RESET()
#define FDCANx_RELEASE_RESET() __HAL_RCC_FDCAN_RELEASE_RESET()

/* FDCAN 1 ========================================= */
#define FDCAN_1 FDCAN1

#define FDCCAN_USE_TIMESTAMP_COUNTER (1U)

/*!< timestamp resolution in micro seconds */
#define FDCAN_1_TIMESTAMP_RESOLUTION (1U)

/* Definition for FDCAN1 Pins */
#define FDCAN_1_TX_PIN       GPIO_PIN_9
#define FDCAN_1_TX_GPIO_PORT GPIOB
#define FDCAN_1_TX_AF        GPIO_AF9_FDCAN1
#define FDCAN_1_RX_PIN       GPIO_PIN_8
#define FDCAN_1_RX_GPIO_PORT GPIOB
#define FDCAN_1_RX_AF        GPIO_AF9_FDCAN1

/* Definition for FDCAN1's NVIC IRQ and IRQ Handlers */
#define FDCAN_1_IRQn          FDCAN1_IT0_IRQn
#define FDCAN_1_IRQHandler    FDCAN1_IT0_IRQHandler
#define FDCAN_1_TX_IRQn       FDCAN1_IT1_IRQn
#define FDCAN_1_TX_IRQHandler FDCAN1_IT1_IRQHandler

#define FDCAN_1_MAX_MSG_COUNT 10u

/* FDCAN 2 ========================================= */
#define FDCAN_2 FDCAN2

/*!< timestamp resolution in micro seconds */
#define FDCAN_2_TIMESTAMP_RESOLUTION (1U)

/* Definition for FDCAN2 Pins */
#define FDCAN_2_TX_PIN       GPIO_PIN_6
#define FDCAN_2_TX_GPIO_PORT GPIOB
#define FDCAN_2_TX_AF        GPIO_AF9_FDCAN2
#define FDCAN_2_RX_PIN       GPIO_PIN_12
#define FDCAN_2_RX_GPIO_PORT GPIOB
#define FDCAN_2_RX_AF        GPIO_AF9_FDCAN2

/* Definition for FDCAN2's NVIC IRQ and IRQ Handlers */
#define FDCAN_2_IRQn          FDCAN2_IT0_IRQn
#define FDCAN_2_IRQHandler    FDCAN2_IT0_IRQHandler
#define FDCAN_2_TX_IRQn       FDCAN2_IT1_IRQn
#define FDCAN_2_TX_IRQHandler FDCAN2_IT1_IRQHandler

#define FDCAN_2_MAX_MSG_COUNT 10u

#define CLK_ENABLE(port_macro) gpio_clk_enable(port_macro)
