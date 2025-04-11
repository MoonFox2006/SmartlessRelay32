#include <HTTPClient.h>
#include "TeleBot.h"
#include "percent.h"

#define TELE_HOST   "api.telegram.org"

static const char *TAG = "TeleBot";

bool TeleBot::start(void) {
  if (! _task) {
    if (xTaskCreate(&task_handler, "TeleBot", task_stack, (void*)this, task_priority, &_task) != pdPASS) {
      _task = nullptr;
      ESP_LOGE(TAG, "Error creating task!");
    }
  }
  return _task != nullptr;
}

void TeleBot::stop(void) {
  if (_task) {
    xTaskNotifyGive(_task);
    _task = nullptr;
  }
}

bool TeleBot::getUpdates(uint16_t timeout) {
  constexpr size_t BUF_SIZE = 256;

  char *buf;
  bool result = false;

  buf = (char*)malloc(BUF_SIZE);
  if (buf) {
    NetworkClientSecure tcp;
    HTTPClient http;

    snprintf(buf, BUF_SIZE, "https://" TELE_HOST "/bot%s/getUpdates?allowed_updates=[\"message\",\"edited_message\"]&limit=1", _key);
    if (timeout) {
      strcat(buf, "&timeout=");
      utoa(timeout, &buf[strlen(buf)], 10);
      http.setTimeout((timeout + 5) * 1000);
    }
    if (_last_update != 0) {
      strcat(buf, "&offset=");
      itoa(_last_update + 1, &buf[strlen(buf)], 10);
    }
    tcp.setInsecure();
    if (http.begin(tcp, buf)) {
      int code;

      free(buf);
      buf = nullptr;
      code = http.GET();
      if (code == HTTP_CODE_OK) {
        JsonDocument doc;
        DeserializationError json_err;

        json_err = deserializeJson(doc, http.getStream());
        http.end();
        if (! json_err) {
          if (doc["ok"].as<bool>() && doc["result"].is<JsonArray>()) {
            result = true;
            for (auto i = 0; i < doc["result"].as<JsonArray>().size(); ++i) {
              JsonObject update, message;

              update = doc["result"].as<JsonArray>()[i].as<JsonObject>();
              _last_update = update["update_id"].as<int32_t>();
              if (update["message"].is<JsonObject>())
                message = update["message"].as<JsonObject>();
              else if (update["edited_message"].is<JsonObject>())
                message = update["edited_message"].as<JsonObject>();
              if (message) {
                if (_allowed && *_allowed) {
                  const char *username = message["from"]["username"].as<const char*>();

                  if (strcasecmp(username, _allowed) != 0) {
                    ESP_LOGW(TAG, "Unallowed sender @%s!", username);
                    continue;
                  }
                }
                if (_onUpdate)
                  _onUpdate(message);
              }
            }
          } else {
            ESP_LOGE(TAG, "Wrong JSON data!");
          }
        } else {
          ESP_LOGE(TAG, "JSON parse error \"%s\"!", json_err.c_str());
        }
      } else {
        ESP_LOGE(TAG, "Wrong status code %d!", code);
        ESP_LOGE(TAG, "%s", http.getString().c_str());
        http.end();
      }
    } else {
      ESP_LOGE(TAG, "Open url error!");
    }
    if (buf)
      free(buf);
  } else {
    ESP_LOGE(TAG, "Not enough memory!");
  }
  return result;
}

bool TeleBot::sendMessage(int64_t chat_id, const char *text) {
  char *buf;
  int size;
  bool result = false;

  size = 31 + percent_encoded_length(text, (size_t)-1) + 1; // "chat_id=-1000000000000000&text=..."
  if (size < 128)
    size = 128;
  buf = (char*)malloc(size);
  if (buf) {
    NetworkClientSecure tcp;
    HTTPClient http;

    snprintf(buf, size, "https://" TELE_HOST "/bot%s/sendMessage", _key);
    tcp.setInsecure();
    if (http.begin(tcp, buf)) {
      snprintf(buf, size, "chat_id=%" PRIi64 "&text=", chat_id);
      percent_encode(&buf[strlen(buf)], text, (size_t)-1);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      size = http.POST((uint8_t*)buf, strlen(buf));
      free(buf);
      buf = nullptr;
      if (size == HTTP_CODE_OK) {
        JsonDocument doc;
        DeserializationError json_err;
    
        json_err = deserializeJson(doc, http.getStream());
        http.end();
        if (! json_err) {
          if (doc["ok"].as<bool>() && doc["result"].is<JsonObject>()) {
            result = true;
            if (_onSent) {
              JsonObject message = doc["result"].as<JsonObject>();

              _onSent(message);
            }
          } else {
            ESP_LOGE(TAG, "Wrong JSON data!");
          }
        } else {
          ESP_LOGE(TAG, "JSON parse error \"%s\"!", json_err.c_str());
        }
      } else {
        ESP_LOGE(TAG, "Wrong status code %d!", size);
        ESP_LOGE(TAG, "%s", http.getString().c_str());
        http.end();
      }
    } else {
      ESP_LOGE(TAG, "Open url error!");
    }
    if (buf)
      free(buf);
  } else {
    ESP_LOGE(TAG, "Not enough memory!");
  }
  return result;
}

void TeleBot::task_handler(void *arg) {
  constexpr uint16_t TELEBOT_TIMEOUT = 15; // 15 sec.
  constexpr uint32_t ERROR_TIMEOUT = 5000; // 5 sec.

  while (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1)) == 0) {
    if (! ((TeleBot*)arg)->getUpdates(TELEBOT_TIMEOUT)) {
      if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(ERROR_TIMEOUT)))
        break;
    }
  }
  vTaskDelete(NULL);
}
