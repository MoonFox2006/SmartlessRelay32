#include <Esp.h>
#include "config.h"
#include "private.h"

Parameters<config_t> config;

void config_init(void) {
  config.onClear([](config_t *cfg) {
#ifdef DEF_HOST_NAME
    SET_STR_PARAM(cfg->host_name, DEF_HOST_NAME);
#else
    SET_STR_PARAM(cfg->host_name, "SmartlessRelay32");
#endif
#ifdef DEF_USER_PSWD
    SET_STR_PARAM(cfg->user_pswd, DEF_USER_PSWD);
#endif
#ifdef DEF_ADMIN_PSWD
    SET_STR_PARAM(cfg->admin_pswd, DEF_ADMIN_PSWD);
#endif
#ifdef DEF_WIFI_SSID
    SET_STR_PARAM(cfg->wifi_ssid, DEF_WIFI_SSID);
#endif
#ifdef DEF_WIFI_PSWD
    SET_STR_PARAM(cfg->wifi_pswd, DEF_WIFI_PSWD);
#endif
#ifdef DEF_WIFI_TIMEOUT
    cfg->wifi_timeout = DEF_WIFI_TIMEOUT;
#endif
#ifdef DEF_NTP_SERVER
    SET_STR_PARAM(cfg->ntp_server, DEF_NTP_SERVER);
#endif
#ifdef DEF_NTP_TZ
    cfg->ntp_tz = DEF_NTP_TZ;
#endif
#ifdef DEF_NTP_INTERVAL
    cfg->ntp_interval = DEF_NTP_INTERVAL;
#endif

#ifdef USE_MQTT
#ifdef DEF_MQTT_SERVER
    SET_STR_PARAM(cfg->mqtt_server, DEF_MQTT_SERVER);
#endif
#ifdef DEF_MQTT_PORT
    cfg->mqtt_port = DEF_MQTT_PORT;
#else
    cfg->mqtt_port = 1883;
#endif
#ifdef DEF_MQTT_SSL
    cfg->mqtt_ssl = DEF_MQTT_SSL;
#endif
#ifdef DEF_MQTT_CLIENT
    SET_STR_PARAM(cfg->mqtt_client, DEF_MQTT_CLIENT);
#else
    snprintf(cfg->mqtt_client, sizeof(cfg->mqtt_client), "SmartlessRelay32_%lX", (uint32_t)(ESP.getEfuseMac() >> 24));
#endif
#ifdef DEF_MQTT_USER
    SET_STR_PARAM(cfg->mqtt_user, DEF_MQTT_USER);
#endif
#ifdef DEF_MQTT_PSWD
    SET_STR_PARAM(cfg->mqtt_pswd, DEF_MQTT_PSWD);
#endif
#ifdef DEF_MQTT_SUB
    SET_STR_PARAM(cfg->mqtt_sub, DEF_MQTT_SUB);
#else
    snprintf(cfg->mqtt_sub, sizeof(cfg->mqtt_sub), "/SmartlessRelay32/%lX/exec", (uint32_t)(ESP.getEfuseMac() >> 24));
#endif
#ifdef DEF_MQTT_PUB
    SET_STR_PARAM(cfg->mqtt_pub, DEF_MQTT_PUB);
#else
    snprintf(cfg->mqtt_pub, sizeof(cfg->mqtt_pub), "/SmartlessRelay32/%lX/state", (uint32_t)(ESP.getEfuseMac() >> 24));
#endif
#ifdef DEF_MQTT_QOS
    cfg->mqtt_qos = DEF_MQTT_QOS;
#endif
#ifdef DEF_MQTT_RETAIN
    cfg->mqtt_retain = DEF_MQTT_RETAIN;
#endif
#endif // #ifdef USE_MQTT

#ifdef USE_TELEGRAM
#ifdef DEF_TELE_KEY
    SET_STR_PARAM(cfg->tele_key, DEF_TELE_KEY);
#endif
#ifdef DEF_TELE_SENDER
    SET_STR_PARAM(cfg->tele_sender, DEF_TELE_SENDER);
#endif
#endif // #ifdef USE_TELEGRAM
  });
  ESP_ERROR_CHECK(config.begin());
}
