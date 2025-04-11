#pragma once

#include <stdint.h>

class CharBuf {
public:
  CharBuf(uint16_t capacity);
  CharBuf(const CharBuf &copy);
  ~CharBuf();

  bool check() const {
    return _buf != nullptr;
  }
  uint16_t length() const {
    return _len;
  }
  void clear();
  uint16_t concat(char c) {
    return concat(&c, 1);
  }
  uint16_t concat(const char *str, uint16_t size = 0);
  uint16_t printf(const char *fmt, ...);
  const char *get() const {
    return _buf;
  }

  operator bool() const {
    return check();
  }
  template <typename T>
  void operator +=(T t) {
    concat(t);
  }
  operator const char*() const {
    return get();
  }

protected:
  virtual bool onConcat(const char *str, uint16_t size) {
    return true;
  }
  virtual bool onOverflow(const char *str, uint16_t size);

  char *_buf;
  uint16_t _cap, _len;
};
