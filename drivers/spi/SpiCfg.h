/*
 * SpiCfg.h
 *
 *  Created on: Dec 26, 2024
 *      Author: cbromann
 */

#ifndef CM7_INC_SPICFG_H_
#define CM7_INC_SPICFG_H_

/* Definition for SPIx clock resources */
/* define SPI1 */
#define SPI1_CLK_ENABLE()                __HAL_RCC_SPI1_CLK_ENABLE()
#define DMA1_CLK_ENABLE()                __HAL_RCC_DMA1_CLK_ENABLE()
#define SPI1_SS_GPIO_CLK_ENABLE()        __HAL_RCC_GPIOA_CLK_ENABLE()
#define SPI1_SCK_GPIO_CLK_ENABLE()       __HAL_RCC_GPIOA_CLK_ENABLE()
#define SPI1_MISO_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()
#define SPI1_MOSI_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOB_CLK_ENABLE()

#define SPI1_FORCE_RESET()               __HAL_RCC_SPI1_FORCE_RESET()
#define SPI1_RELEASE_RESET()             __HAL_RCC_SPI1_RELEASE_RESET()

/* Definition for SPIx Pins */
#define SPI1_PWR_PIN                     GPIO_PIN_7
#define SPI1_PWR_GPIO_PORT               GPIOA         
#define SPI1_PWR_AF                      0U
#define SPI1_SS_PIN                      GPIO_PIN_4
#define SPI1_SS_GPIO_PORT                GPIOA
#define SPI1_SS_AF                       GPIO_AF5_SPI1
#define SPI1_SCK_PIN                     GPIO_PIN_5
#define SPI1_SCK_GPIO_PORT               GPIOA
#define SPI1_SCK_AF                      GPIO_AF5_SPI1
#define SPI1_MISO_PIN                    GPIO_PIN_6
#define SPI1_MISO_GPIO_PORT              GPIOA
#define SPI1_MISO_AF                     GPIO_AF5_SPI1
#define SPI1_MOSI_PIN                    GPIO_PIN_5
#define SPI1_MOSI_GPIO_PORT              GPIOB
#define SPI1_MOSI_AF                     GPIO_AF5_SPI1

/* Definition for SPIx's DMA */
#define SPI1_TX_DMA_STREAM               DMA1_Stream3
#define SPI1_RX_DMA_STREAM               DMA1_Stream2

#define SPI1_TX_DMA_REQUEST              DMA_REQUEST_SPI1_TX
#define SPI1_RX_DMA_REQUEST              DMA_REQUEST_SPI1_RX

/* Definition for SPIx's NVIC */
#define SPI1_DMA_TX_IRQn                 DMA1_Stream3_IRQn
#define SPI1_DMA_RX_IRQn                 DMA1_Stream2_IRQn

#define SPI1_DMA_TX_IRQHandler           DMA1_Stream3_IRQHandler
#define SPI1_DMA_RX_IRQHandler           DMA1_Stream2_IRQHandler

#define SPI1_IRQn                        SPI1_IRQn
#define SPI1_IRQHandler                  SPI1_IRQHandler

#define CS_ACTIVE_HIGH 0

#ifndef SPI_USE_RTOS
#define SPI_USE_RTOS 1
#endif

#endif /* CM7_INC_SPICFG_H_ */
