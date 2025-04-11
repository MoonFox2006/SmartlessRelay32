#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include "CharBuf.h"

static const char *TAG = "CharBuf";

CharBuf::CharBuf(uint16_t capacity) {
  _cap = capacity;
  _buf = (char*)malloc(capacity);
  if (_buf) {
    *_buf = '\0';
    _len = 0;
  } else
    ESP_EARLY_LOGE(TAG, "Not enough memory!");
}

CharBuf::CharBuf(const CharBuf &copy) {
  _cap = copy._cap;
  _buf = (char*)malloc(_cap);
  if (_buf) {
    _len = copy._len;
    memcpy(_buf, copy._buf, _len + 1);
  } else
    ESP_EARLY_LOGE(TAG, "Not enough memory!");
}

CharBuf::~CharBuf() {
  if (_buf) {
    free(_buf);
  }
}

void CharBuf::clear() {
  *_buf = '\0';
  _len = 0;
}

uint16_t CharBuf::concat(const char *str, uint16_t size) {
  if (! size)
    size = strlen(str);
  if (onConcat(str, size)) {
    uint16_t remains = _cap - _len - 1;

    if (size <= remains) {
      memcpy(&_buf[_len], str, size);
      _len += size;
      _buf[_len] = '\0';
    } else { // Overflow
      if (remains > 0) {
        memcpy(&_buf[_len], str, remains);
        _len += remains;
        _buf[_len] = '\0';
      }
      if (! onOverflow(&str[remains], size - remains))
        return remains;
    }
    return size;
  }
  return 0;
}

uint16_t CharBuf::printf(const char *fmt, ...) {
  va_list args1, args2;
  char *str;
  int len;
  uint16_t result = 0;

  va_start(args1, fmt);
  va_copy(args2, args1);
  len = vsnprintf(NULL, 0, fmt, args2);
  va_end(args2);
  if (len > 0) {
    str = (char*)malloc(len + 1);
    if (str) {
      vsnprintf(str, len + 1, fmt, args1);
      result = concat(str, len);
      free(str);
    } else
      ESP_LOGE(TAG, "Not enough memory!");
  }
  va_end(args1);
  return result;
}

bool CharBuf::onOverflow(const char *str, uint16_t size) {
  memmove(_buf, &_buf[size], _cap - size - 1);
  memcpy(&_buf[_cap - size - 1], str, size);
  return true;
}
