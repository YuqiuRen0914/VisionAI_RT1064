#include "vision.h"

#include "camera_mt9v03x.h"
#include "event.h"

static vision_result_t vision_result;

ai_status_t vision_module_init(void)
{
    vision_result.line_offset = 0;
    vision_result.target_speed = 0;
    vision_result.valid = 0;

    return CameraMt9v03xInit();
}

void vision_module_tick(void)
{
    CameraMt9v03xFrame frame;
    event_msg_t event;

    if(CameraMt9v03xGetFrame(&frame) != AI_OK)
    {
        return;
    }

    (void)frame;
    vision_result.line_offset = 0;
    vision_result.target_speed = 0;
    vision_result.valid = 1;

    event.id = EVENT_ID_VISION_RESULT;
    event.value0 = vision_result.line_offset;
    event.value1 = vision_result.target_speed;
    (void)event_publish(&event);
}
