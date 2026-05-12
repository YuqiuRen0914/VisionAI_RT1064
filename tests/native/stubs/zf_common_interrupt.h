#ifndef TESTS_NATIVE_STUBS_ZF_COMMON_INTERRUPT_H
#define TESTS_NATIVE_STUBS_ZF_COMMON_INTERRUPT_H

#include <stdint.h>

typedef uint32_t uint32;

static inline uint32 interrupt_global_disable(void)
{
    return 0U;
}

static inline void interrupt_global_enable(uint32 primask)
{
    (void)primask;
}

#endif
