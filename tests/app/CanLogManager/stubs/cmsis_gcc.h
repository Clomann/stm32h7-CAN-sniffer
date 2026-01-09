#pragma once

#include <stdint.h>

#ifndef   __ASM
#define __ASM __asm
#endif

#ifndef   __INLINE
#define __INLINE __inline
#endif

#ifndef   __STATIC_INLINE
#define __STATIC_INLINE static __inline
#endif

#ifndef   __WEAK
#define __WEAK __attribute__((weak))
#endif

#ifndef   __USED
#define __USED __attribute__((used))
#endif

#ifndef   __PACKED
#define __PACKED __attribute__((packed))
#endif

#ifndef   __ALIGNED
#define __ALIGNED(x) __attribute__((aligned(x)))
#endif

#ifndef   __NO_RETURN
#define __NO_RETURN __attribute__((noreturn))
#endif

#ifndef   __NOP
#define __NOP() __ASM volatile("nop")
#endif
