#pragma once

#include <inttypes.h>
#include "logger.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

extern const char *BOOLS[];

extern Logger logger;
extern volatile bool restarting;
