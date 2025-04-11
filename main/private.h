#pragma once

#include "defs.h"

/* Parameter default values */
//#define DEF_USER_PSWD       "12345678"
#define DEF_ADMIN_PSWD      "1029384756"

//#define DEF_WIFI_SSID       "ssid"
//#define DEF_WIFI_PSWD       "pswd"
#define DEF_WIFI_TIMEOUT    30 // 30 sec.

#define DEF_NTP_SERVER      "pool.ntp.org"
#define DEF_NTP_TZ          3
#define DEF_NTP_INTERVAL    (3600 * 4) // 4 hrs.

#ifdef USE_MQTT
//#define DEF_MQTT_SERVER     "mqtt"
#define DEF_MQTT_PORT       1883
#define DEF_MQTT_SSL        false
#define DEF_MQTT_QOS        0
#define DEF_MQTT_RETAIN     false
#endif

#ifdef USE_TELEGRAM
//#define DEF_TELE_KEY        "botkey"
//#define DEF_TELE_SENDER     "myself"
#endif
