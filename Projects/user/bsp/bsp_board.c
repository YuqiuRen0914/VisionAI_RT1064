#include "bsp_board.h"

#include "clock_cfg.h"
#include "pinmap.h"
#include "zf_common_debug.h"
#include "zf_driver_gpio.h"

void board_init(void)
{
    clock_cfg_init();
    debug_init();

    gpio_init(BSP_LED_HEARTBEAT_PIN, GPO, GPIO_LOW, GPO_PUSH_PULL);
}

void board_heartbeat_toggle(void)
{
    gpio_toggle_level(BSP_LED_HEARTBEAT_PIN);
}
