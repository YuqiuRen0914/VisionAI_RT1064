#include "zf_common_headfile.h"
#include "vision_app.h"

static uint32 vision_tick_count;

void vision_app_init(void)
{
    vision_tick_count = 0;
}

void vision_app_tick(void)
{
    vision_tick_count++;

    // Add camera capture, image processing, and model inference here.
}
