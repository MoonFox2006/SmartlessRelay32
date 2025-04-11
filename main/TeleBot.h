#pragma once

#include <functional>
#include <freertos/task.h>
#include <ArduinoJson.h>

class TeleBot {
public:
  typedef std::function<void(const JsonObject &message)> message_cb_t;

  TeleBot(void) : _task(nullptr), _key(nullptr), _allowed(nullptr), _onUpdate(nullptr), _onSent(nullptr), _last_update(0) {}
  ~TeleBot(void) {
    stop();
  }

  void key(const char *value) {
    _key = value;
  }
  void allowed(const char *value) {
    _allowed = value;
  }
  void onUpdate(message_cb_t cb) {
    _onUpdate = cb;
  }
  void onSent(message_cb_t cb) {
    _onSent = cb;
  }
  operator bool() const {
    return _key != nullptr;
  }
  bool isActive(void) const {
    return _task != nullptr;
  }
  bool start(void);
  void stop(void);
  bool getUpdates(uint16_t timeout = 0);
  bool sendMessage(int64_t chat_id, const char *text);

protected:
  static constexpr uint32_t task_stack = 6144;
  static constexpr UBaseType_t task_priority = tskIDLE_PRIORITY + 5;

  static void task_handler(void *arg);

  TaskHandle_t _task;
  const char *_key;
  const char *_allowed;
  message_cb_t _onUpdate;
  message_cb_t _onSent;
  int32_t _last_update;
};
