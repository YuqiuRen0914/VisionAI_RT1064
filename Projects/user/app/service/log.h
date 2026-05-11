#ifndef LOG_H
#define LOG_H

#include "ai_error.h"

void log_service_init(void);
ai_status_t log_write(const char *text);

#endif
