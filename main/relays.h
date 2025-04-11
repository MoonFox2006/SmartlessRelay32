#pragma once

#include <inttypes.h>

#define RELAY_LEVEL 1

const uint8_t RELAY_PINS[] = { 7 };

void relays_init(void);
bool relay_get(uint8_t index);
void relay_set(uint8_t index, bool on, const char *source);
void relay_toggle(uint8_t index, const char *source);
