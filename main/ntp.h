#pragma once

#include <inttypes.h>

bool ntpUpdate(const char *server, int8_t tz, uint32_t timeout = 1000, uint8_t repeat = 1);
