# pragma once

#include <esp_log.h>
#include "CharBuf.h"

#define LOGGER_SIZE     4096
#define LOG_LINE_SIZE   256

class Logger : public CharBuf {
public:
    Logger() : CharBuf(LOGGER_SIZE), _line(nullptr), _standard(nullptr) {
        _this = this;
    }
    ~Logger();

    bool start();

protected:
    static int logger_vprintf(const char *fmt, va_list args);

    static Logger *_this;

    char *_line;
    vprintf_like_t _standard;
};
