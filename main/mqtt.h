#pragma once

#include "defs.h"

#ifdef USE_MQTT
bool mqttIsActive(void);
bool mqttInit(void);
void mqttDone(void);
bool mqttPublish(const char *payload);
#endif
