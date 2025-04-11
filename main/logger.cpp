#include <string.h>
#include <stdio.h>
#include "logger.h"

static const char *TAG = "Logger";

Logger::~Logger() {
    if (_standard)
        esp_log_set_vprintf(_standard);
    if (_line)
        free(_line);
}

bool Logger::start() {
    if (! _line) {
        _line = (char*)malloc(LOG_LINE_SIZE);
        if (! _line) {
            ESP_LOGE(TAG, "Not enoung memory!");
            return false;
        }
    }
    if (! _standard)
        _standard = esp_log_set_vprintf(logger_vprintf);
    return true;
}

int Logger::logger_vprintf(const char *fmt, va_list args) {
    int result;

    result = vsnprintf(_this->_line, LOG_LINE_SIZE, fmt, args);
    if (result > 0) {
        _this->concat(_this->_line, result);
        ::printf(_this->_line);
    }
    return result;
}

Logger *Logger::_this = nullptr;
