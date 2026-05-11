#include "interact.h"

#include "display.h"

ai_status_t interact_module_init(void)
{
    return DisplayInit();
}

void interact_module_tick(void)
{
}
