#include "comm.h"

#include "wireless.h"

ai_status_t comm_module_init(void)
{
    return WirelessInit();
}

void comm_module_tick(void)
{
}
