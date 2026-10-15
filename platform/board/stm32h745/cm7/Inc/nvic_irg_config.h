#pragma once

#include "FreeRTOSConfig.h"

#define FDCAN_IRQ_PREEMPT_PRIO                                                 \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY - 5)
#define TIMx_IRQ_PREEMPT_PRIO (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY - 4)

#define DEFERRED_IRQ_PREEMPT_PRIO                                              \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1)
#define SPI1_INTERRUPT_PREEMPT_PRIO                                            \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 2)
#define FDCAN_TX_REPLAY_IRQ_PREEMPT_PRIO (FDCAN_IRQ_PREEMPT_PRIO + 1)
#define ETH_IRQ_PREEMPT_PRIO             (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 3)

#if FDCAN_TX_REPLAY_IRQ_PREEMPT_PRIO <= FDCAN_IRQ_PREEMPT_PRIO
#error "FDCAN Tx replay IRQ must stay below the FDCAN Rx IRQ priority"
#endif
