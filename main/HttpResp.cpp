#include "HttpResp.h"

uint16_t HttpResp::concatEscaped(const char *str, uint16_t size) {
  uint16_t result = 0;

  if (! size)
    size = strlen(str);
  while (size) {
    uint16_t len = 0;

    while (len < size) {
      char c = str[len];

      if ((c == '"') || (c == '&') || (c == '<') || (c == '>'))
        break;
      ++len;
    }
    if (len) {
      result += concat(str, len);
      str += len;
      size -= len;
    }
    if (size) { // Escaped char was encountered
      if (*str == '"') {
        result += concat("&quot;", 6);
      } else if (*str == '&') {
        result += concat("&amp;", 5);
      } else if (*str == '<') {
        result += concat("&lt;", 4);
      } else if (*str == '>') {
        result += concat("&gt;", 4);
      }
      ++str;
      --size;
    }
  }
  return result;
}

void HttpResp::send() {
  if (_chunked) {
    if (_len) {
      _http->sendContent(_buf, _len);
    }
    _http->sendContent(nullptr, 0);
  } else {
    _http->send_P(_code, _type, _buf, _len);
  }
//  clear();
}

bool HttpResp::onOverflow(const char *str, uint16_t size) {
  if (! _chunked) {
    _http->setContentLength(CONTENT_LENGTH_UNKNOWN);
    _http->send(_code, _type, "");
    _chunked = true;
  }
  _http->sendContent(_buf, _len);
  clear();
  concat(str, size);
  return true;
}
