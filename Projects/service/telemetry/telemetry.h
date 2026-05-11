#ifndef TELEMETRY_H
#define TELEMETRY_H

#include "ai_error.h"

void telemetry_service_init(void);
ai_status_t telemetry_send_text(const char *text);

#endif
