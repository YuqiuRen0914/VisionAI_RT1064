#ifndef USER_LIBRARIES_WIRELESS_H
#define USER_LIBRARIES_WIRELESS_H

#include "ai_error.h"

ai_status_t WirelessInit(void);
ai_status_t WirelessSend(const uint8_t *data, uint32_t length);

#endif
