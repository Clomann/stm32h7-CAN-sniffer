/* USER CODE BEGIN Header */
/**
 ******************************************************************************
  * File Name          : ethernetif.h
  * Description        : This file provides initialization code for LWIP
  *                      middleWare.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__

#include "lwip/err.h"
#include "lwip/netif.h"
// #include "cmsis_os.h"
#include "os_stub.h"

/* Within 'USER CODE' section, code will be kept by default at each generation */
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* Exported functions ------------------------------------------------------- */
err_t ethernetif_init(struct netif *netif);

void ethernetif_input(void *argument);
void ethernet_link_thread(void *argument);

void ethernet_link_check_state(struct netif *netif);
void send_udp_message(void);

void Error_Handler(void);
u32_t sys_jiffies(void);
u32_t sys_now(void);

/* USER CODE BEGIN 1 */

static inline void AddressAligned_CleanDCache(void *addr, uint32_t len)
{
    uintptr_t raw_addr     = (uintptr_t)addr;
    uintptr_t aligned_addr = raw_addr & ~0x1F;
    uintptr_t aligned_size = ((raw_addr + len + 31) & ~0x1F) - aligned_addr;

    SCB_CleanDCache_by_Addr((uint32_t *)aligned_addr, aligned_size);
}

static inline void AddressAligned_InvalidateDCache(void *addr, uint32_t len)
{
    uintptr_t raw_addr     = (uintptr_t)addr;
    uintptr_t aligned_addr = raw_addr & ~0x1F;
    uintptr_t aligned_size = ((raw_addr + len + 31) & ~0x1F) - aligned_addr;

    SCB_InvalidateDCache_by_Addr((uint32_t *)aligned_addr, aligned_size);
}

/* USER CODE END 1 */
#endif
