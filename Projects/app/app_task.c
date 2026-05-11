#include "app_task.h"

#include "ai_config.h"
#include "bsp_board.h"
#include "comm.h"
#include "motion.h"
#include "os_port.h"
#include "task_cfg.h"
#include "vision.h"

static void vision_task(void *argument)
{
    (void)argument;

    while(1)
    {
        vision_module_tick();
        os_port_delay_ms(AI_VISION_PERIOD_MS);
    }
}

static void motion_task(void *argument)
{
    (void)argument;

    while(1)
    {
        motion_module_tick();
        os_port_delay_ms(AI_MOTION_PERIOD_MS);
    }
}

static void comm_task(void *argument)
{
    (void)argument;

    while(1)
    {
        comm_module_tick();
        os_port_delay_ms(AI_COMM_PERIOD_MS);
    }
}

static void heartbeat_task(void *argument)
{
    (void)argument;

    while(1)
    {
        board_heartbeat_toggle();
        os_port_delay_ms(AI_HEARTBEAT_PERIOD_MS);
    }
}

void app_task_create_all(void)
{
    (void)os_port_create_task(vision_task, "vision", TASK_STACK_VISION, NULL, TASK_PRIO_VISION);
    (void)os_port_create_task(motion_task, "motion", TASK_STACK_MOTION, NULL, TASK_PRIO_MOTION);
    (void)os_port_create_task(comm_task, "comm", TASK_STACK_COMM, NULL, TASK_PRIO_COMM);
    (void)os_port_create_task(heartbeat_task, "heartbeat", TASK_STACK_HEARTBEAT, NULL, TASK_PRIO_HEARTBEAT);
}

void app_task_run_cooperative(void)
{
    while(1)
    {
        vision_module_tick();
        motion_module_tick();
        comm_module_tick();
        board_heartbeat_toggle();
        os_port_delay_ms(AI_HEARTBEAT_PERIOD_MS);
    }
}
