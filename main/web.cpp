#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include "web.h"
#include "HttpResp.h"
#include "global.h"
#include "config.h"
#include "relays.h"
#ifdef USE_MQTT
#include "mqtt.h"
#endif

/* URL */
#define ROOT_URL    "/"
#define CONFIG_URL  "/config"
#define WIFI_URL    "/wifi"
#define TIME_URL    "/time"
#define LOG_URL     "/log"
#define RESTART_URL "/restart"
#define OTA_URL     "/ota"
#define AJAX_URL    "/ajax"
#define SWITCH_URL  "/switch"
#define FAVICON_URL "/favicon.ico"

/* AJAX field names */
#define JSON_WIFI   "wifi"
#define JSON_HEAP   "heap"
#define JSON_UPTIME "uptime"
#define JSON_MQTT   "mqtt"
#define JSON_DATE   "date"
#define JSON_TIME   "time"
#define JSON_RELAY  "relay"

static const char TAG[] = "Web";

static WebServer *http = nullptr;
static TaskHandle_t web_task = nullptr;

template<typename T>
static constexpr T min_of(T a, T b) {
  return a <= b ? a : b;
}

static bool authorize(bool admin = false) {
  if (WiFi.getMode() != WIFI_AP) { // Not captive portal
    if (admin) {
      if (*config->admin_pswd) {
        if (! http->authenticate("admin", config->admin_pswd)) {
          http->requestAuthentication();
          return false;
        }
      }
    } else {
      if (*config->user_pswd) {
        if ((! http->authenticate("user", config->user_pswd)) &&
          ((! *config->admin_pswd) || (! http->authenticate("admin", config->admin_pswd)))) {
          http->requestAuthentication();
          return false;
        }
      }
    }  
  }
  return true;
}

static void handleRoot() {
  if (! authorize())
    return;

  HttpResp resp(http);
  
  if (resp) {
    resp.concat(
      "<!DOCTYPE html>\n"
      "<html>\n"
      "<head>\n"
      "<title>Web Test</title>\n"
      "<meta charset=\"utf-8\">\n"
      "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n"
      "<style>\n"
      "body{background-color:#eee}\n"
      "a{text-decoration:none;color:black;border:1px solid black;border-radius:10px 25px;padding:8px 16px}\n"
      ".switch{vertical-align:top;margin:0 3px 0 0;width:17px;height:17px}\n"
      ".switch+label{cursor:pointer}\n"
      ".switch:not(checked){position:absolute;opacity:0}\n"
      ".switch:not(checked)+label{position:relative;padding:0 0 0 60px}\n"
      ".switch:not(checked)+label:before{content:'';position:absolute;top:-4px;left:0;width:50px;height:26px;border-radius:13px;background:#CDD1DA;box-shadow:inset 0 2px 3px rgba(0,0,0,.2)}\n"
      ".switch:not(checked)+label:after{content:'';position:absolute;top:-2px;left:2px;width:22px;height:22px;border-radius:10px;background:#FFF;box-shadow:0 2px 5px rgba(0,0,0,.3);transition:all .2s}\n"
      ".switch:checked+label:before{background:#9FD468}\n"
      ".switch:checked+label:after{left:26px}\n"
      "</style>\n"
      "<script>\n"
      "function relaySwitch(id,on){\n"
      "let x=new XMLHttpRequest();\n"
      "x.open('GET','" SWITCH_URL "?dummy='+Date.now()+'&id='+id+'&on='+on,true);\n"
      "x.onreadystatechange=function(){\n"
      "if(x.readyState==4){\n"
      "if(x.status!=200){\n"
      "console.log('Error: '+x.responseText);\n"
      "}\n"
      "}\n"
      "}\n"
      "x.send(null);\n"
      "}\n"
      "function refreshData(){\n"
      "let x=new XMLHttpRequest();\n"
      "x.open('GET','" AJAX_URL "?dummy='+Date.now(),true);\n"
      "x.onreadystatechange=function(){\n"
      "if((x.readyState==4)&&(x.status==200)){\n"
      "let d=JSON.parse(x.responseText);\n"
      "document.getElementById('" JSON_HEAP "').innerHTML=d." JSON_HEAP ";\n"
      "document.getElementById('" JSON_UPTIME "').innerHTML=d." JSON_UPTIME ";\n"
#ifdef USE_MQTT
      "document.getElementById('" JSON_MQTT "').innerHTML=d." JSON_MQTT "?'':'not ';\n"
#endif
      "document.getElementById('" JSON_DATE "').innerHTML=d." JSON_DATE ";\n"
      "document.getElementById('" JSON_TIME "').innerHTML=d." JSON_TIME ";\n"
      "for(let i=0;i<d." JSON_RELAY ".length;++i){\n"
      "document.getElementById('" JSON_RELAY "'+i).checked=d." JSON_RELAY "[i];\n"
      "}\n"
      "}\n"
      "}\n"
      "x.send(null);\n"
      "setTimeout(refreshData,1000);\n"
      "}\n"
      "function syncTime(){\n"
      "let x=new XMLHttpRequest();\n"
      "x.open('GET','" TIME_URL "?time='+Math.floor(Date.now()/1000)+'&dummy='+Date.now(),true);\n"
      "x.onreadystatechange=function(){\n"
      "if((x.readyState==4)&&(x.status==200)){\n"
      "document.getElementById('sync_time').style.display='none';\n"
      "}\n"
      "}\n"
      "x.send(null);\n"
      "}\n"
      "</script>\n"
      "</head>\n"
      "<body onload=\"refreshData()\">\n"
      "<h2>Web Test</h2>\n"
      "Free heap size: <span id=\"" JSON_HEAP "\">?</span> bytes<br/>\n"
      "Uptime: <span id=\"" JSON_UPTIME "\">?</span> seconds<br/>\n"
#ifdef USE_MQTT
      "MQTT: <span id=\"" JSON_MQTT "\">not </span>connected<br/>\n"
#endif
      "<div id=\"sync_time\"><button onclick=\"syncTime()\">Sync time with browser</button></div>\n"
      "<span id=\"" JSON_DATE "\"></span> <span id=\"" JSON_TIME "\"></span><br/>\n"
      "<p/>\n");
    for (uint8_t i = 0; i < ARRAY_SIZE(RELAY_PINS); ++i) {
      resp.printf("<input type=\"checkbox\" class=\"switch\" name=\"relay%u\" id=\"relay%u\" onchange=\"relaySwitch(%u,this.checked)\">\n"
        "<label for=\"relay%u\">Relay #%u</label>\n"
        "<p/>\n", i, i, i, i, i + 1);
    }
    resp.concat(
      "<a href=\"" CONFIG_URL "\">Config</a>\n"
      "<a href=\"" LOG_URL "\">Log</a>\n"
      "<a href=\"" RESTART_URL "\" onclick=\"if(!confirm('Are you sure?'))return false\">Restart</a>\n"
      "</body>\n"
      "</html>");
    resp.send();
  } else {
    http->send(500, "text/plain", "Internal server error!");
  }
}

static void handleConfig() {
  if (! authorize(true)) // Admin level
    return;

  HttpResp resp(http);

  if (resp) {
    if (http->method() == HTTP_GET) {
      resp.concat(
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "<title>Web Test Config</title>\n"
        "<meta charset=\"utf-8\">\n"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n"
        "<style>\n"
        "body{background-color:#eee}\n"
        "td:first-child{text-align:right}\n"
        "</style>\n"
        "<script>\n"
        "function refreshWiFi(btn){\n"
        "let x=new XMLHttpRequest();\n"
        "x.open('GET','" WIFI_URL "?dummy='+Date.now(),true);\n"
        "x.onreadystatechange=function(){\n"
        "if((x.readyState==4)&&(x.status==200)){\n"
        "let d=JSON.parse(x.responseText);\n"
        "if(Array.isArray(d." JSON_WIFI ")){\n"
        "let list=document.getElementById('" JSON_WIFI "');\n"
        "list.innerHTML='';\n"
        "for(let i=0;i<d." JSON_WIFI ".length;++i){\n"
        "let option=document.createElement('option');\n"
        "option.value=d." JSON_WIFI "[i];\n"
        "list.appendChild(option);\n"
        "}\n"
        "}\n"
        "btn.disabled=false;\n"
        "}\n"
        "}\n"
        "x.send(null);\n"
        "btn.disabled=true;\n"
        "}\n"
        "</script>\n"
        "</head>\n"
        "<body>\n"
        "<h2>Web Test Config</h2>\n"
        "<form method=\"POST\">\n"
        "<table>\n"
        "<tr><th colspan=2>General</th></tr>\n"
        "<tr><td>Host name:</td><td><input type=\"text\" name=\"" PARAM_HOST_NAME "\" value=\"");
      resp.concatEscaped(config->host_name);
      resp.printf("\" size=%u maxlength=%u></td></tr>\n", min_of((unsigned int)(sizeof(config_t::host_name) - 1), PARAM_LEN), sizeof(config_t::host_name) - 1);
      resp.concat(
        "<tr><td>User password:</td><td><input type=\"password\" name=\"" PARAM_USER_PSWD "\" value=\"");
      resp.concatEscaped(config->user_pswd);
      resp.printf("\" size=%u maxlength=%u></td></tr>\n", min_of((unsigned int)(sizeof(config_t::user_pswd) - 1), PARAM_LEN), sizeof(config_t::user_pswd) - 1);
      resp.concat(
        "<tr><td>Admin password:</td><td><input type=\"password\" name=\"" PARAM_ADMIN_PSWD "\" value=\"");
      resp.concatEscaped(config->admin_pswd);
      resp.printf("\" size=%u maxlength=%u></td></tr>\n", min_of((unsigned int)(sizeof(config_t::admin_pswd) - 1), PARAM_LEN), sizeof(config_t::admin_pswd) - 1);
      resp.concat(
        "<tr><th colspan=2>WiFi</th></tr>\n"
        "<tr><td>WiFi SSID:</td><td><input type=\"text\" name=\"" PARAM_WIFI_SSID "\" value=\"");
      resp.concatEscaped(config->wifi_ssid);
      resp.printf("\" size=%u maxlength=%u list=\"" JSON_WIFI "\">\n", min_of((unsigned int)(sizeof(config_t::wifi_ssid) - 1), PARAM_LEN), sizeof(config_t::wifi_ssid) - 1);
      resp.concat(
        "<input type=\"button\" value=\"Refresh\" onclick=\"refreshWiFi(this)\">\n"
        "<datalist id=\"" JSON_WIFI "\"></datalist></td></tr>\n"
        "<tr><td>WiFi password:</td><td><input type=\"password\" name=\"" PARAM_WIFI_PSWD "\" value=\"");
      resp.concatEscaped(config->wifi_pswd);
      resp.printf("\" size=%u maxlength=%u></td></tr>\n", min_of((unsigned int)(sizeof(config_t::wifi_pswd) - 1), PARAM_LEN), sizeof(config_t::wifi_pswd) - 1);
      resp.concat(
        "<tr><td>WiFi timeout:</td><td><input type=\"text\" name=\"" PARAM_WIFI_TIMEOUT "\" value=\"");
      resp.printf("%u\" size=%u maxlength=%u> sec.</td></tr>\n", config->wifi_timeout, 5, 5);
      resp.concat(
        "<tr><th colspan=2>SNTP</th></tr>\n"
        "<tr><td>NTP server:</td><td><input type=\"text\" name=\"" PARAM_NTP_SERVER "\" value=\"");
      resp.concatEscaped(config->ntp_server);
      resp.printf("\" size=%u maxlength=%u></td></tr>\n", min_of((unsigned int)(sizeof(config_t::ntp_server) - 1), PARAM_LEN), sizeof(config_t::ntp_server) - 1);
      resp.concat(
        "<tr><td>NTP time zone:</td><td><select name=\"" PARAM_NTP_TZ "\" size=1>\n");
      for (int8_t i = -11; i <= 13; ++i) {
        resp.printf("<option value=\"%d\"%s>GMT%+d</option>\n", i, config->ntp_tz == i ? " selected" : "", i);
      }
      resp.concat(
        "</select></td></tr>\n"
        "<tr><td>NTP interval:</td><td><input type=\"text\" name=\"" PARAM_NTP_INTERVAL "\" value=\"");
      resp.printf("%u\" size=%u maxlength=%u> sec.</td></tr>\n", config->ntp_interval, 5, 5);
      resp.concat(
#ifdef USE_MQTT
        "<tr><th colspan=2>MQTT</th></tr>\n"
        "<tr><td>MQTT broker:</td><td><input type=\"text\" name=\"" PARAM_MQTT_SERVER "\" value=\"");
      resp.concatEscaped(config->mqtt_server);
      resp.printf("\" size=%u maxlength=%u></td></tr>\n", min_of((unsigned int)(sizeof(config_t::mqtt_server) - 1), PARAM_LEN), sizeof(config_t::mqtt_server) - 1);
      resp.concat(
        "<tr><td>MQTT port:</td><td><input type=\"text\" name=\"" PARAM_MQTT_PORT "\" value=\"");
      resp.printf("%u\" size=%u maxlength=%u></td></tr>\n", config->mqtt_port, 5, 5);
      resp.concat(
        "<tr><td>SSL?</td><td><input type=\"checkbox\" name=\"" PARAM_MQTT_SSL "\" value=\"1\"");
      if (config->mqtt_ssl)
        resp.concat(" checked");
      resp.concat("></td></tr>\n"
        "<tr><td>MQTT client:</td><td><input type=\"text\" name=\"" PARAM_MQTT_CLIENT "\" value=\"");
      resp.concatEscaped(config->mqtt_client);
      resp.printf("\" size=%u maxlength=%u></td></tr>\n", min_of((unsigned int)(sizeof(config_t::mqtt_client) - 1), PARAM_LEN), sizeof(config_t::mqtt_client) - 1);
      resp.concat(
        "<tr><td>MQTT user:</td><td><input type=\"text\" name=\"" PARAM_MQTT_USER "\" value=\"");
      resp.concatEscaped(config->mqtt_user);
      resp.printf("\" size=%u maxlength=%u></td></tr>\n", min_of((unsigned int)(sizeof(config_t::mqtt_user) - 1), PARAM_LEN), sizeof(config_t::mqtt_user) - 1);
      resp.concat(
        "<tr><td>MQTT password:</td><td><input type=\"password\" name=\"" PARAM_MQTT_PSWD "\" value=\"");
      resp.concatEscaped(config->mqtt_pswd);
      resp.printf("\" size=%u maxlength=%u></td></tr>\n", min_of((unsigned int)(sizeof(config_t::mqtt_pswd) - 1), PARAM_LEN), sizeof(config_t::mqtt_pswd) - 1);
      resp.concat(
        "<tr><td>Subscribe:</td><td><input type=\"text\" name=\"" PARAM_MQTT_SUB "\" value=\"");
      resp.concatEscaped(config->mqtt_sub);
      resp.printf("\" size=%u maxlength=%u></td></tr>\n", min_of((unsigned int)(sizeof(config_t::mqtt_sub) - 1), PARAM_LEN), sizeof(config_t::mqtt_sub) - 1);
      resp.concat(
        "<tr><td>Publish:</td><td><input type=\"text\" name=\"" PARAM_MQTT_PUB "\" value=\"");
      resp.concatEscaped(config->mqtt_pub);
      resp.printf("\" size=%u maxlength=%u></td></tr>\n", min_of((unsigned int)(sizeof(config_t::mqtt_pub) - 1), PARAM_LEN), sizeof(config_t::mqtt_pub) - 1);
      resp.concat(
        "<tr><td>QOS:</td><td><select name=\"" PARAM_MQTT_QOS "\" size=1>\n");
      for (uint8_t i = 0; i <= 2; ++i) {
        resp.printf("<option value=\"%u\"%s>%u</option>\n", i, config->mqtt_qos == i ? " selected" : "", i);
      }
      resp.concat(
        "</select></td></tr>\n"
        "<tr><td>Retain?</td><td><input type=\"checkbox\" name=\"" PARAM_MQTT_RETAIN "\" value=\"1\"");
      if (config->mqtt_retain)
        resp.concat(" checked");
      resp.concat("></td></tr>\n"
#endif
#ifdef USE_TELEGRAM
        "<tr><th colspan=2>Telegram Bot</th></tr>\n"
        "<tr><td>Bot key:</td><td><input type=\"text\" name=\"" PARAM_TELE_KEY "\" value=\"");
      resp.concatEscaped(config->tele_key);
      resp.printf("\" size=%u maxlength=%u></td></tr>\n", min_of((unsigned int)(sizeof(config_t::tele_key) - 1), PARAM_LEN), sizeof(config_t::tele_key) - 1);
      resp.concat(
        "<tr><td>Allowed sender:</td><td><input type=\"text\" name=\"" PARAM_TELE_SENDER "\" value=\"");
      resp.concatEscaped(config->tele_sender);
      resp.printf("\" size=%u maxlength=%u></td></tr>\n", min_of((unsigned int)(sizeof(config_t::tele_sender) - 1), PARAM_LEN), sizeof(config_t::tele_sender) - 1);
      resp.concat(
#endif
        "</table>\n"
        "<p/>\n"
        "<input type=\"submit\" value=\"OK\">\n"
        "<button onclick=\"location.href='" ROOT_URL "';return false\">Back</button>\n"
        "</form>\n"
        "</body>\n"
        "</html>");
      resp.send();
    } else if (http->method() == HTTP_POST) {
      bool success = true;

      /* Checkbox workarounds */
#ifdef USE_MQTT
      config->mqtt_ssl = false;
      config->mqtt_retain = false;
#endif

      for (auto i = 0; i < http->args(); ++i) {
        const String &name = http->argName(i);
        const String &value = http->arg(i);

        if (name.equals(PARAM_HOST_NAME)) {
          SET_STR_PARAM(config->host_name, value.c_str());
        } else if (name.equals(PARAM_USER_PSWD)) {
          SET_STR_PARAM(config->user_pswd, value.c_str());
        } else if (name.equals(PARAM_ADMIN_PSWD)) {
          SET_STR_PARAM(config->admin_pswd, value.c_str());
        } else if (name.equals(PARAM_WIFI_SSID)) {
          SET_STR_PARAM(config->wifi_ssid, value.c_str());
        } else if (name.equals(PARAM_WIFI_PSWD)) {
          SET_STR_PARAM(config->wifi_pswd, value.c_str());
        } else if (name.equals(PARAM_WIFI_TIMEOUT)) {
          config->wifi_timeout = value.toInt();
        } else if (name.equals(PARAM_NTP_SERVER)) {
          SET_STR_PARAM(config->ntp_server, value.c_str());
        } else if (name.equals(PARAM_NTP_TZ)) {
          config->ntp_tz = value.toInt();
        } else if (name.equals(PARAM_NTP_INTERVAL)) {
          config->ntp_interval = value.toInt();
#ifdef USE_MQTT
        } else if (name.equals(PARAM_MQTT_SERVER)) {
          SET_STR_PARAM(config->mqtt_server, value.c_str());
        } else if (name.equals(PARAM_MQTT_PORT)) {
          config->mqtt_port = value.toInt();
        } else if (name.equals(PARAM_MQTT_SSL)) {
          config->mqtt_ssl = value.equals("1");
        } else if (name.equals(PARAM_MQTT_CLIENT)) {
          SET_STR_PARAM(config->mqtt_client, value.c_str());
        } else if (name.equals(PARAM_MQTT_USER)) {
          SET_STR_PARAM(config->mqtt_user, value.c_str());
        } else if (name.equals(PARAM_MQTT_PSWD)) {
          SET_STR_PARAM(config->mqtt_pswd, value.c_str());
        } else if (name.equals(PARAM_MQTT_SUB)) {
          SET_STR_PARAM(config->mqtt_sub, value.c_str());
        } else if (name.equals(PARAM_MQTT_PUB)) {
          SET_STR_PARAM(config->mqtt_pub, value.c_str());
        } else if (name.equals(PARAM_MQTT_QOS)) {
          config->mqtt_qos = value.toInt();
        } else if (name.equals(PARAM_MQTT_RETAIN)) {
          config->mqtt_retain = value.equals("1");
#endif
#ifdef USE_TELEGRAM
        } else if (name.equals(PARAM_TELE_KEY)) {
          SET_STR_PARAM(config->tele_key, value.c_str());
        } else if (name.equals(PARAM_TELE_SENDER)) {
          SET_STR_PARAM(config->tele_sender, value.c_str());
#endif
        }
      }
      if (! config) {
        success = config.commit() == ESP_OK;
        if (success) {
          ESP_LOGI(TAG, "Configuration successfully stored");
        } else {
          ESP_LOGE(TAG, "Store configuration fail!");
        }
      }
      resp.concat(
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "<title>Web Test Configuration</title>\n"
        "<meta charset=\"utf-8\">\n"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n"
        "<meta http-equiv=\"refresh\" content=\"3;URL=" CONFIG_URL "\">\n"
        "<style>\n"
        "body{background-color:#eee");
      if (! success) {
        resp.concat(";color:red");
      }
      resp.concat(
        "}\n"
        "</style>\n"
        "</head>\n"
        "<body>\n");
      if (success) {
        resp.concat("Configuration sucessfully stored\n");
      } else {
        resp.concat("Store configuration fail!\n");
      }
      resp.concat(
        "</body>\n"
        "</html>");
      resp.send();
    } else {
      http->send(405, "Method not allowed!");
    }
  } else {
    http->send(500, "Internal server error!");
  }
}

static void handleWiFi() {
  HttpResp resp(http, "application/json");

  if (resp) {
    int16_t n;
    bool sta;
  
    sta = (WiFi.getMode() == WIFI_STA) || (WiFi.getMode() == WIFI_AP_STA);
    if (! sta)
      WiFi.enableSTA(true);
    n = WiFi.scanNetworks();
    resp.concat("{\"" JSON_WIFI "\":[");
    if (n > 0) {
      for (auto i = 0; i < n; ++i) {
        if (i)
          resp.concat(',');
        resp.concat('"');
        resp.concatEscaped(WiFi.SSID(i).c_str());
        resp.concat('"');
      }
    }
    WiFi.scanDelete();
    if (! sta)
      WiFi.enableSTA(false);
    resp.concat("]}");
    resp.send();
  } else {
    http->send(500, "text/plain", "Internal server error!");
  }
}

static void handleTime() {
  if (http->hasArg("time")) {
    uint32_t t = http->arg("time").toInt();

    if (t) {
      struct timeval val;

      val.tv_sec = t + config->ntp_tz * 3600;
      val.tv_usec = 0;
      settimeofday(&val, NULL);
      ESP_LOGI(TAG, "Time synchronized with browser");
      http->send(200, "text/plain", "");
      return;
    }
  }
  ESP_LOGW(TAG, "Wrong parameters!");
  http->send(500, "text/plain", "Internal server error!");
}

static void handleLog() {
  if (! authorize(true)) // Admin level
    return;

  HttpResp resp(http);

  if (resp) {
    if (http->method() == HTTP_GET) {
      resp.concat(
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "<title>Web Test Log</title>\n"
        "<meta charset=\"utf-8\">\n"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n"
        "<style>\n"
        "body{background-color:#eee}\n"
        "textarea{resize:none;overflow:auto;width:99%;padding:0.5%}\n"
        "</style>\n"
        "<script>\n"
        "function logScroll(){\n"
        "let l=document.getElementById(\"log\");\n"
        "l.scrollTop=l.scrollHeight;\n"
        "}\n"
        "</script>\n"
        "</head>\n"
        "<body onload=\"logScroll()\">\n"
        "<h2>Log</h2>\n"
        "<textarea id=\"log\" rows=25 readonly>\n");
      resp.concatEscaped(logger.get(), logger.length());
      resp.concat(
        "</textarea>\n"
        "<form method=\"POST\">\n"
        "<button onclick=\"location.reload();return false\">Refresh</button>\n"
        "<input type=\"submit\" value=\"Clear!\" onclick=\"if(!confirm('Are you sure?'))return false\">\n"
        "<button onclick=\"location.href='" ROOT_URL "';return false\">Back</button>\n"
        "</form>\n"
        "</body>\n"
        "</html>");
      resp.send();
    } else if (http->method() == HTTP_POST) {
      logger.clear();
      resp.concat(
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "<title>Web Test Log</title>\n"
        "<meta charset=\"utf-8\">\n"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n"
        "<meta http-equiv=\"refresh\" content=\"3;URL=" LOG_URL "\">\n"
        "<style>\n"
        "body{background-color:#eee}\n"
        "</style>\n"
        "</head>\n"
        "<body>\n"
        "Log sucessfully cleared\n"
        "</body>\n"
        "</html>");
      resp.send();
    } else {
      http->send(405, "Method not allowed!");
    }
  } else {
    http->send(500, "Internal server error!");
  }
}

static void handleRestart() {
  restarting = true;
  http->sendHeader("Connection", "close");
  http->send(200, "text/html",
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head>\n"
    "<title>Web Test Restart</title>\n"
    "<meta charset=\"utf-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n"
    "<meta http-equiv=\"refresh\" content=\"10;URL=" ROOT_URL "\">\n"
    "<style>\n"
    "body{background-color:#eee}\n"
    "</style>\n"
    "</head>\n"
    "<body>\n"
    "Restarting...\n"
    "</body>\n"
    "</html>");
}

static void handleAjax() {
  HttpResp resp(http, "application/json");

  if (resp) {
    time_t now;
    tm timeinfo;
    char dstr[11], tstr[9];

    time(&now);
    localtime_r(&now, &timeinfo);
    if (timeinfo.tm_year >= 2025 - 1900) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
      snprintf(dstr, sizeof(dstr), "%02d.%02d.%d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
      snprintf(tstr, sizeof(tstr), "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
#pragma GCC diagnostic pop
    } else {
      *dstr = '\0';
      *tstr = '\0';
    }
    resp.printf("{\"" JSON_HEAP "\":%lu,\"" JSON_UPTIME "\":%lu,", esp_get_free_heap_size(), (uint32_t)(millis() / 1000));
#ifdef USE_MQTT
    resp.printf("\"" JSON_MQTT "\":%s,", BOOLS[mqttIsActive()]);
#endif
    resp.printf("\"" JSON_DATE "\":\"%s\",\"" JSON_TIME "\":\"%s\",\"" JSON_RELAY "\":[", dstr, tstr);
    for (uint8_t i = 0; i < ARRAY_SIZE(RELAY_PINS); ++i) {
      if (i)
        resp.concat(',');
      resp.concat(BOOLS[relay_get(i)]);
    }
    resp.concat("]}");
    resp.send();
  } else {
    http->send(500, "text/plain", "Internal server error!");
  }
}

static void handleOta() {
  if (! authorize(true)) // Admin level
    return;

  if (http->method() == HTTP_GET) {
    http->send(200, "text/html",
      "<!DOCTYPE html>\n"
      "<html>\n"
      "<head>\n"
      "<title>Web Test Update</title>\n"
      "<meta charset=\"utf-8\">\n"
      "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n"
      "<style>\n"
      "body{background-color:#eee}\n"
      "</style>\n"
      "</head>\n"
      "<body>\n"
      "<form method=\"POST\" action=\"" OTA_URL "\" enctype=\"multipart/form-data\" onsubmit=\"if(document.getElementsByName('update')[0].files.length==0){alert('No firmware file selected!');return false;}\">\n"
      "<input type=\"file\" name=\"update\" accept=\".bin\"><br/>\n"
      "<input type=\"submit\" value=\"Update\">\n"
      "<button onclick=\"location.href='" ROOT_URL "';return false\">Back</button>\n"
      "</form>\n"
      "</body>\n"
      "</html>");  
  } else if (http->method() == HTTP_POST) {
    http->sendHeader("Connection", "close");
    if (Update.hasError()) {
      http->send(500, "text/plain", Update.errorString());
//      ESP_LOGE(TAG, "OTA error: %s", Update.errorString());
    } else {
      restarting = true;
      http->send(200, "text/html",
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "<title>Web Test Restart</title>\n"
        "<meta charset=\"utf-8\">\n"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n"
        "<meta http-equiv=\"refresh\" content=\"10;URL=" ROOT_URL "\">\n"
        "<style>\n"
        "body{background-color:#eee}\n"
        "</style>\n"
        "</head>\n"
        "<body>\n"
        "Firmware successfully updated. Restarting...\n"
        "</body>\n"
        "</html>");
//      ESP_LOGI(TAG, "OTA success");
    }
  } else {
    http->send(405, "Method not allowed!");
  }
}

static void handleOtaing() {
  HTTPUpload &upload = http->upload();

  if (upload.status == UPLOAD_FILE_START) {
    ESP_LOGI(TAG, "Start OTA from file \"%s\"", upload.filename.c_str());
    if (! Update.begin()) { // start with max available size
      ESP_LOGE(TAG, "OTA start error: %s", Update.errorString());
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      ESP_LOGE(TAG, "OTA write error: %s", Update.errorString());
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) { // true to set the size to the current progress
      ESP_LOGI(TAG, "OTA successfully updated (%u bytes)", upload.totalSize);
    } else {
      ESP_LOGE(TAG, "OTA end error: %s", Update.errorString());
    }
  } else {
    Update.abort();
    ESP_LOGE(TAG, "OTA was aborted!");
  }
}

static void handleSwitch() {
  if (http->hasArg("id") && http->hasArg("on")) {
    uint8_t id = http->arg("id").toInt();
    bool on = http->arg("on").equals("true");

    if (id < ARRAY_SIZE(RELAY_PINS)) {
      relay_set(id, on, " by Web");
      http->send(200, "text/plain", "");
      return;
    }
  }
  ESP_LOGW(TAG, "Wrong parameters!");
  http->send(500, "text/plain", "Internal server error!");
}

static void handleFavicon() {
  extern const uint8_t favicon_ico_start[] asm("_binary_favicon_ico_start");
  extern const uint8_t favicon_ico_end[] asm("_binary_favicon_ico_end");

  http->send_P(200, "image/x-icon", (const char*)favicon_ico_start, favicon_ico_end - favicon_ico_start);
}
  
static void handleNotFound() {
  if (WiFi.getMode() == WIFI_AP) { // Captive portal
    if (! http->hostHeader().equals(WiFi.softAPIP().toString())) {
      http->sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
      http->send(302, "text/plain", "Redirect to the captive portal");
      return;
    }
  }

  if (http->uri().equals(FAVICON_URL)) {
    handleFavicon();
  } else {
    http->send(404, "text/plain", "Page not found!");
  }
}

static void webTask(void *arg) {
  WebServer *http = (WebServer*)arg;

  while (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1)) == 0) {
    http->handleClient();
  }
  vTaskDelete(NULL);
}

bool webIsActive(void) {
  return (http != nullptr) && (web_task != nullptr);
}

bool webInit(void) {
  constexpr uint32_t TASK_STACK = 4096;
  constexpr UBaseType_t TASK_PRIORITY = tskIDLE_PRIORITY + 5;

  if ((! http) && (! web_task)) {
    http = new WebServer(80);
    if (http) {
      http->onNotFound(handleNotFound);
      http->on(ROOT_URL, HTTP_GET, handleRoot);
      http->on(CONFIG_URL, handleConfig);
      http->on(WIFI_URL, HTTP_GET, handleWiFi);
      http->on(TIME_URL, HTTP_GET, handleTime);
      http->on(LOG_URL, handleLog);
      http->on(RESTART_URL, HTTP_GET, handleRestart);
      http->on(AJAX_URL, HTTP_GET, handleAjax);
      http->on(OTA_URL, HTTP_ANY, handleOta, handleOtaing);
      http->on(SWITCH_URL, HTTP_GET, handleSwitch);
      http->begin();
      if (xTaskCreate(webTask, "web_task", TASK_STACK, http, TASK_PRIORITY, &web_task) == pdPASS) {
        return true;
      } else {
        web_task = nullptr;
        ESP_LOGE(TAG, "Error creating task!");
      }
      delete http;
      http = nullptr;
    } else {
      ESP_LOGE(TAG, "Error creating WebServer instance!");
    }
  }
  return false;
}

void webDone(void) {
  if (web_task) {
    xTaskNotifyGive(web_task);
    web_task = nullptr;
  }
  if (http) {
    http->stop();
    delete http;
    http = nullptr;
  }
}
