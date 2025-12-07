/**
 * @file memory_sections.h
 * @brief Memory section attributes for STM32H7 and PC builds
 * 
 * Usage:
 *   DMA_BUFFER uint8_t buffer[1024];
 *   ETH_RX_DESC DMARxDscrTab[ETH_RX_DESC_CNT];
 */
#pragma once

#ifdef TARGET_STM32H7

/* DMA-capable memory (RAM_D2, 32-byte aligned) */
#define DMA_BUFFER       __attribute__((section(".dma_buffer")))
#define DMA_BUFFER_RO    __attribute__((section(".dma_buffer.ro")))

/* Ethernet descriptors (RAM_D2, absolute addresses) */
#define ETH_RX_DESC      __attribute__((section(".RxDecripSection")))
#define ETH_TX_DESC      __attribute__((section(".TxDecripSection")))
#define ETH_RX_POOL      __attribute__((section(".Rx_PoolSection")))

/* Bulk storage (RAM_D2) */
#define RAM_D2_SECTION   __attribute__((section(".ram_d2")))

/* Retention data (RAM_D3, survives reset) */
#define RAM_D3_SECTION   __attribute__((section(".ram_d3")))

#else  /* PC/Test build */

/* All attributes expand to nothing on PC */
#define DMA_BUFFER
#define DMA_BUFFER_RO
#define ETH_RX_DESC
#define ETH_TX_DESC
#define ETH_RX_POOL
#define RAM_D2_SECTION
#define RAM_D3_SECTION

#endif /* TARGET_STM32H7 */
