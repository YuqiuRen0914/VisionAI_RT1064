#include "zf_common_headfile.h"
#include "vision_app.h"

#define HEARTBEAT_LED_PIN B9

static void board_init(void)
{
    clock_init(SYSTEM_CLOCK_600M);
    debug_init();

    gpio_init(HEARTBEAT_LED_PIN, GPO, 0, GPO_PUSH_PULL);
}

int main(void)
{
    board_init();
    vision_app_init();

    while(1)
    {
        vision_app_tick();
        gpio_toggle_level(HEARTBEAT_LED_PIN);
        system_delay_ms(100);
    }
}
