#pragma once

#include <WebServer.h>
#include "CharBuf.h"

#define HTTP_PAGE_SIZE  1024

class HttpResp : public CharBuf {
public:
  HttpResp(WebServer *http, const char *type = "text/html", int code = 200) : CharBuf(HTTP_PAGE_SIZE), _http(http), _type(type), _code(code), _chunked(false) {}

  uint16_t concatEscaped(const char *str, uint16_t size = 0);
  void send();

protected:
  bool onOverflow(const char *str, uint16_t size) override;

  WebServer *_http;
  const char *_type;
  int _code;
  bool _chunked;
};
