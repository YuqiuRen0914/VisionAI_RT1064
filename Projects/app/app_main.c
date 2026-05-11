#include "app_main.h"

#include "ai_config.h"
#include "app_task.h"
#include "comm.h"
#include "event.h"
#include "log.h"
#include "motion.h"
#include "os_port.h"
#include "param.h"
#include "telemetry.h"
#include "vision.h"

static void app_service_init(void)
{
    event_service_init();
    log_service_init();
    param_service_init();
    telemetry_service_init();
}

static void app_module_init(void)
{
    (void)vision_module_init();
    (void)motion_module_init();
    (void)comm_module_init();
}

void app_main(void)
{
    app_service_init();
    app_module_init();
    app_task_create_all();

#if AI_USE_FREERTOS
    os_port_start_scheduler();
#else
    app_task_run_cooperative();
#endif

    while(1)
    {
    }
}
