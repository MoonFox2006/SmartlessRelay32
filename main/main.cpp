#include "defs.h"

#include <esp_pm.h>
#include <Arduino.h>
#include <WiFi.h>
#include <Ticker.h>
#ifdef USE_MDNS
#include <ESPmDNS.h>
#endif
#include "global.h"
#include "config.h"
#include "relays.h"
#include "web.h"
#include "ntp.h"
#include "loopdns.h"
#ifdef USE_MQTT
#include "mqtt.h"
#endif
#ifdef USE_TELEGRAM
#include "TeleBot.h"
#endif
#include "buttons.h"
#include "blinks.h"

#define LED_PIN     18
#define LED_LEVEL   0
#define LED_BRIGHT  127 // 50% of 8 bit

#define BTN_PIN     9
#define BTN_LEVEL   0
#define BTN_PULLUP  1

#define CP_WAIT     2000 // 2 sec.
#define CP_SSID     "SmartlessRelay32"
#define CP_PSWD     "1029384756"
#define CP_CHANNEL  1

#define BLINK_CP_CONNECTING     BLINK_2HZ
#define BLINK_CP_CONNECTED      BLINK_BREATH

#define BLINK_WIFI_DISCONNECTED BLINK_OFF
#define BLINK_WIFI_CONNECTING   BLINK_2HZ
#define BLINK_WIFI_CONNECTED    BLINK_BREATH

static const char TAG[] = "SmartlessRelay32";

#ifdef LED_PIN
static blinks_handle_t blinks = NULL;
#endif
static buttons_handle_t buttons = NULL;
static Ticker *wifiTicker = nullptr;
static Ticker *ntpTicker = nullptr;
#ifdef USE_TELEGRAM
static TeleBot *tele = nullptr;
#endif

static void ntpUpdating() {
  if (ntpUpdate(config->ntp_server, config->ntp_tz)) {
    ESP_LOGI(TAG, "NTP refreshed");
    if (config->ntp_interval) {
      ntpTicker->once_ms(config->ntp_interval * 1000, ntpUpdating);
      ESP_LOGI(TAG, "NTP daemon started");
    }
  } else {
    ESP_LOGW(TAG, "NTP retrying...");
    ntpTicker->once_ms(5000, ntpUpdating);
  }
}

static void wifiCallback(arduino_event_id_t event, arduino_event_info_t info) {
  constexpr uint8_t WIFI_RETRY = 5;

  static uint8_t retry = 0;

  switch (event) {
    case ARDUINO_EVENT_WIFI_AP_START:
#ifdef LED_PIN
      blinks_update(blinks, 0, BLINK_CP_CONNECTING, LED_BRIGHT);
#endif
      if (loopdns_start()) {
        ESP_LOGI(TAG, "DNS server started");
      }
      if (webInit()) {
        ESP_LOGI(TAG, "Web server started");
      }
      break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
#ifdef LED_PIN
      blinks_update(blinks, 0, BLINK_OFF, LED_BRIGHT);
#endif
      webDone();
      ESP_LOGI(TAG, "Web server stopped");
      loopdns_stop();
      ESP_LOGI(TAG, "DNS server stopped");
      break;
#ifdef LED_PIN
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      blinks_update(blinks, 0, WiFi.softAPgetStationNum() > 0 ? BLINK_CP_CONNECTED : BLINK_CP_CONNECTING, LED_BRIGHT);
      break;
#endif

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      if (ntpTicker) {
        if (ntpTicker->active()) {
          ntpTicker->detach();
          ESP_LOGI(TAG, "NTP daemon stopped");
        }
      }
#ifdef USE_MQTT
      if (mqttIsActive()) {
        mqttDone();
        ESP_LOGI(TAG, "MQTT service stopped");
      }
#endif
      if (webIsActive()) {
        webDone();
        ESP_LOGI(TAG, "Web server stopped");
      }
#ifdef USE_MDNS
      MDNS.end();
      ESP_LOGI(TAG, "mDNS service stopped");
#endif
#ifdef USE_TELEGRAM
      if (tele) {
        if (tele->isActive()) {
          tele->stop();
          ESP_LOGI(TAG, "Telegram bot stopped");
        }
      }
#endif
      if (++retry >= WIFI_RETRY) {
        WiFi.disconnect();
#ifdef LED_PIN
        blinks_update(blinks, 0, BLINK_WIFI_DISCONNECTED, LED_BRIGHT);
#endif
        ESP_LOGE(TAG, "WiFi disconnected");
        if (wifiTicker) {
          wifiTicker->once_ms(config->wifi_timeout * 1000, [&]() {
            ESP_LOGW(TAG, "WiFi reconnecting...");
            WiFi.begin(config->wifi_ssid, config->wifi_pswd);
#ifdef LED_PIN
            blinks_update(blinks, 0, BLINK_WIFI_CONNECTING, LED_BRIGHT);
#endif
            retry = 0;
          });
        }
      }
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
#ifdef LED_PIN
      blinks_update(blinks, 0, BLINK_WIFI_CONNECTED, LED_BRIGHT);
#endif
      retry = 0;
      {
        uint32_t ip = info.got_ip.ip_info.ip.addr;

        ESP_LOGI(TAG, "WiFi connected with IP: %u.%u.%u.%u", (uint8_t)(ip & 0xFF), (uint8_t)((ip >> 8) & 0xFF), (uint8_t)((ip >> 16) & 0xFF), (uint8_t)((ip >> 24) & 0xFF));
      }
      if (ntpTicker) {
        ntpUpdating();
      }
      if (webInit()) {
        ESP_LOGI(TAG, "Web server started");
      }
#ifdef USE_MDNS
      if (MDNS.begin(config->host_name)) {
        ESP_LOGI(TAG, "mDNS service started");
        MDNS.addService("http", "tcp", 80);
      } else {
        ESP_LOGE(TAG, "mDNS service fail!");
      }
#endif
#ifdef USE_MQTT
      if (*config->mqtt_server && *config->mqtt_client) {
        if (mqttInit()) {
          ESP_LOGI(TAG, "MQTT service started");
        }
      }
#endif
#ifdef USE_TELEGRAM
      if (tele) {
        if (tele->start()) {
          ESP_LOGI(TAG, "Telegram bot started");
        }
      }
#endif
      break;
    default:
      break;
  }
}

#ifdef USE_TELEGRAM
static void tele_init() {
  if (*config->tele_key) {
    tele = new TeleBot();
    if (tele) {
      tele->key(config->tele_key);
      tele->allowed(config->tele_sender);
      tele->onUpdate([](const JsonObject &message) {
        char answer[128];
        const char *text;

        text = message["text"].as<const char*>();
        *answer = '\0';
        if (*text == '/') {
          if (strcmp(&text[1], "start") == 0) {
            strlcpy(answer, "Hello!", sizeof(answer));
          } else if (strcmp(&text[1], "help") == 0) {
            strlcpy(answer, "Smartless Relay Bot commands:\n/help - commands list\n/status - relay status\n/relay <N> <0|1> - turns relay #N off|on", sizeof(answer));
          } else if (strcmp(&text[1], "status") == 0) {
            snprintf(answer, sizeof(answer), "Heap: %lu\nUptime: %lu\nRelay #1: %s", esp_get_free_heap_size(), (uint32_t)(millis() / 1000), relay_get(0) ? "ON" : "OFF");
          } else if (strncmp(&text[1], "relay ", 6) == 0) {
            const char *cmd = &text[7];
            int8_t r = -1;

            while (*cmd == ' ') // Skip ' '
              ++cmd;
            while ((*cmd >= '0') && (*cmd <= '9')) {
              if (r == -1)
                r = 0;
              else
                r *= 10;
              r += *cmd - '0';
              ++cmd;
            }
            if ((*cmd == ' ') && (r >= 0) && (r < ARRAY_SIZE(RELAY_PINS))) {
              while (*cmd == ' ') // Skip ' '
                ++cmd;
              if ((cmd[1] == '\0') && ((*cmd == '0') || (*cmd == '1'))) {
                bool on = *cmd == '1';

                relay_set(r, on, " by Bot");
                snprintf(answer, sizeof(answer), "Relay #%d turns o%s", r + 1, on ? "n" : "ff");
              }
            }
          }
        }
        if (*answer) {
          if (! tele->sendMessage(message["chat"]["id"].as<int64_t>(), answer)) {
            ESP_LOGE(TAG, "Telegram bot answer error!");
          }
        } else {
          ESP_LOGW(TAG, "Unhandled telegram bot message: \"%s\"", text);
        }
      });
    } else {
      ESP_LOGE(TAG, "Error creating telegram bot!");
    }
  }
}
#endif

void setup() {
/*
#if CONFIG_XTAL_FREQ == 26
  Serial.begin(74880);
#else
  Serial.begin(115200);
#endif
*/

#if CONFIG_PM_ENABLE
  {
    const esp_pm_config_t pm_config = {
      .max_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
      .min_freq_mhz = CONFIG_XTAL_FREQ,
#if CONFIG_FREERTOS_USE_TICKLESS_IDLE
      .light_sleep_enable = true
#endif
    };

    ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
  }
#endif

  relays_init();
  config_init();
  assert(logger && logger.start());

#ifdef LED_PIN
  blinks = blinks_init();
  assert(blinks);
  blinks_add(blinks, (gpio_num_t)LED_PIN, LED_LEVEL, NULL);
#endif

  const buttons_config_t cfg = BUTTONS_CONFIG_DEFAULT();

  buttons = buttons_init(&cfg);
  assert(buttons);
  buttons_add(buttons, (gpio_num_t)BTN_PIN, BTN_LEVEL, BTN_PULLUP, NULL);
  buttons_start(buttons);

  bool cp = (! *config->host_name) || (! *config->wifi_ssid) || (! *config->wifi_pswd); // Incomplete parameters

  if (! cp) {
    if (esp_reset_reason() == ESP_RST_POWERON) {
      uint32_t t = millis();

#ifdef LED_PIN
      blinks_update(blinks, 0, BLINK_4HZ, LED_BRIGHT);
#endif
      ESP_LOGI(TAG, "Waiting for button long click to activate captive portal...");
      while (! cp) {
        button_event_t evt;

        if (millis() - t >= CP_WAIT)
          break;
        while (buttons_get_event(buttons, &evt, 1)) {
#if MAX_BUTTONS > 1
          if (evt.index == 0) {
            if (evt.state == BTN_LONGCLICK) {
#ifdef LED_PIN
              blinks_update(blinks, 0, BLINK_ON, LED_BRIGHT);
#endif
              ESP_LOGI(TAG, "Release button to activate captive portal");
            } else if (evt.state >= BTN_LONGCLICK) {
              cp = true;
              break;
            }
          }
#else
          if (evt.state == BTN_LONGPRESS) {
#ifdef LED_PIN
            blinks_update(blinks, 0, BLINK_ON, LED_BRIGHT);
#endif
            ESP_LOGI(TAG, "Release button to activate captive portal");
          } else if (evt.state >= BTN_LONGCLICK) {
            cp = true;
            break;
          }
#endif
          t = millis();
        }
      }
#ifdef LED_PIN
      blinks_update(blinks, 0, BLINK_OFF, LED_BRIGHT);
#endif
    }
  }

  if (! cp) {
    if (config->wifi_timeout) {
      wifiTicker = new Ticker();
      assert(wifiTicker);
    }

    if (*config->ntp_server) {
      ntpTicker = new Ticker();
      assert(ntpTicker);
    }

#ifdef USE_TELEGRAM
    tele_init();
#endif
  }

  WiFi.persistent(false);
  WiFi.setHostname(config->host_name);
  WiFi.mode(cp ? WIFI_AP : WIFI_STA);
  WiFi.onEvent(wifiCallback);
  if (cp) {
    WiFi.softAP(CP_SSID, CP_PSWD, CP_CHANNEL);
    ESP_LOGI(TAG, "Creating AP \"%s\" with password \"%s\" and IP %s", CP_SSID, CP_PSWD, WiFi.softAPIP().toString().c_str());
  } else {
    ESP_LOGI(TAG, "Connecting to WiFi...");
    WiFi.begin(config->wifi_ssid, config->wifi_pswd);
#ifdef LED_PIN
    blinks_update(blinks, 0, BLINK_WIFI_CONNECTING, LED_BRIGHT);
#endif
  }

  buttons_clear_events(buttons);
}

void loop() {
  const char *STATES[] = { "pressed", "long pressed", "very long pressed", "clicked", "long clicked", "very long clicked" };

  if (restarting) {
    delay(100);
    ESP_LOGW(TAG, "Restarting...");
    esp_restart();
  }

  button_event_t evt;

  while (buttons_get_event(buttons, &evt, 1)) { // 1 ms.
#if MAX_BUTTONS > 1
    ESP_LOGI(TAG, "Button #%u %s %u time(s)", evt.index, STATES[evt.state], evt.clicks + 1);
#else
    ESP_LOGI(TAG, "Button %s %u time(s)", STATES[evt.state], evt.clicks + 1);
#endif
    if (evt.state == BTN_CLICK) {
      relay_toggle(0, " by button");
    } else if (evt.state == BTN_LONGCLICK) {
      relay_set(0, false, " by button");
    }
  }
}
