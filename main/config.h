#pragma once

#include <inttypes.h>
#include "defs.h"
#include "Parameters.h"

#define SET_STR_PARAM(p, v) strlcpy(p, v, sizeof(p))

/* Parameter names */
#define PARAM_HOST_NAME     "host_name"
#define PARAM_USER_PSWD     "user_pswd"
#define PARAM_ADMIN_PSWD    "admin_pswd"

#define PARAM_WIFI_SSID     "wifi_ssid"
#define PARAM_WIFI_PSWD     "wifi_pswd"
#define PARAM_WIFI_TIMEOUT  "wifi_timeout"

#define PARAM_NTP_SERVER    "ntp_server"
#define PARAM_NTP_TZ        "ntp_tz"
#define PARAM_NTP_INTERVAL  "ntp_interval"

#ifdef USE_MQTT
#define PARAM_MQTT_SERVER   "mqtt_server"
#define PARAM_MQTT_PORT     "mqtt_port"
#define PARAM_MQTT_SSL      "mqtt_ssl"
#define PARAM_MQTT_CLIENT   "mqtt_client"
#define PARAM_MQTT_USER     "mqtt_user"
#define PARAM_MQTT_PSWD     "mqtt_pswd"
#define PARAM_MQTT_SUB      "mqtt_sub"
#define PARAM_MQTT_PUB      "mqtt_pub"
#define PARAM_MQTT_QOS      "mqtt_qos"
#define PARAM_MQTT_RETAIN   "mqtt_retain"
#endif

#ifdef USE_TELEGRAM
#define PARAM_TELE_KEY      "tele_key"
#define PARAM_TELE_SENDER   "tele_sender"
#endif

struct __attribute__((__packed__)) config_t {
    char host_name[32];
    char user_pswd[32];
    char admin_pswd[32];

    char wifi_ssid[32];
    char wifi_pswd[64];
    uint16_t wifi_timeout;

    char ntp_server[64];
    int8_t ntp_tz;
    uint16_t ntp_interval;

#ifdef USE_MQTT
    char mqtt_server[64];
    uint16_t mqtt_port;
    bool mqtt_ssl;
    char mqtt_client[32];
    char mqtt_user[32];
    char mqtt_pswd[32];
    char mqtt_sub[32];
    char mqtt_pub[32];
    uint8_t mqtt_qos;
    bool mqtt_retain;
#endif

#ifdef USE_TELEGRAM
    char tele_key[48];
    char tele_sender[32];
#endif
};

extern Parameters<config_t> config;

void config_init(void);
