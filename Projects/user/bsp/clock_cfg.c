#include "clock_cfg.h"

#include "zf_common_clock.h"

void clock_cfg_init(void)
{
    clock_init(SYSTEM_CLOCK_600M);
}
