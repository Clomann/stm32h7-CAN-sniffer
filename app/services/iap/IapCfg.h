#pragma once

#ifndef APPLICATION_SECONDARY_START_ADDRESS

extern uint8_t __app_secondary_start__;

#define APPLICATION_SECONDARY_START_ADDRESS                                    \
    ((uint32_t)(uintptr_t) & __app_secondary_start__)

#endif

#ifndef APPLICATION_SECONDARY_SIZE

extern uint8_t __app_secondary_size__;

#define APPLICATION_SECONDARY_SIZE                                             \
    ((uint32_t)(uintptr_t) & __app_secondary_size__)

#endif
