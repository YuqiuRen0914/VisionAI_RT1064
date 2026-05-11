#include "telemetry.h"

#include "log.h"

void telemetry_service_init(void)
{
}

ai_status_t telemetry_send_text(const char *text)
{
    return log_write(text);
}
